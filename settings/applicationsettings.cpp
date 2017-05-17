#include "applicationsettings.h"

#include <settings/debug.h>
#include <QFile>
#include <QHash>
#include <QString>
#include <QByteArray>
#include <QStringList>
#include <QTimer>

#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QCryptographicHash>

#include <errno.h>
#include <unistd.h>

/**
	\class ApplicationSettings

	\brief This class provides mechanisims to manage application settings.
	It is designed to be a singleton so that it can be interfaced in a flexible way.

	ApplicationSettings class is based on JSON parsing, however, it keeps settings
	using a custom implementation. This is primaryly due to reasons that Qt's JSON
	implementation is not designed to allow changes (efficiently) in the underlying
	JSON file but designed to provide read-only access. We need read-write access.
	However, loading and saving operations use fully JSON compatiple input/output.

	ApplicationSettings class allows linearized access to your JSON settings through
	the use of get() and set() functions. Let's suppose we have the following JSON
	file as our source:

		{
			"GroupName": "MainProcesses",
			"ManageProcesses": true,
			"Cgroup": {
				"Root": "/sys/fs/cgroup",
				"Path": "deepvms/main"
			}
		}

	Then, we can access settings as following:

		...
		ApplicationSettings::instance()->get("GroupName");
		ApplicationSettings::instance()->get("Cgroup.Root");
		ApplicationSettings::instance()->get("Cgroup.Path");
		...

	In addition to linearization, ApplicationSettings supports array based indexing.
	For the following JSON:

		{
			"GroupName": "MainProcesses",
			"ManageProcesses": true,
			"Cgroup": {
				"Root": "/sys/fs/cgroup",
				"Path": "deepvms/main"
			},
			"Processes": [
				{
					"Name": "Dashboard",
					"Path": "Dashboard",
					"AutoStart": false
				},
				{
					"Name": "EncoderManager",
					"Path": "EncoderManager",
					"AutoStart": false
				}
			]
		}

	We use following accessors:

		...
		ApplicationSettings::instance()->get("Processes.0.Name");
		ApplicationSettings::instance()->get("Processes.1.AutoStart");
		ApplicationSettings::instance()->set("Processes.1.AutoStart", true);
		...

	\section Defining Custom Handlers

	ApplicationSettings class also support defining your custom handlers and hooking
	those handlers to specific JSON objects. For the above JSON files we can add our
	own handler for Cgroup object as follows:

		ApplicationSettings::instance()->addHandler("Cgroup", this);

	Of course this to work, our class should provide AppSettingHandlerInterface. It is
	also possible to add different hooks to different array elements:

		s->addHandler("Processes.0", new TestHandler);
		s->addHandler("Processes.1", new TestHandler);

	\section Managing Cache

	If no handler is ever installed on a given object, ApplicationSettings class uses
	values present in its local cache. If you write a setting, you change the value present
	in cache, if you read a setting you get the one present in cache. On the contrary,
	when you install a handler most of the time you read/write using some other mechanism.
	In some cases you may need both functionality, this is perfectly possible. You return
	a invalid, or empty, QVariant() in your getSetting() implementation for the settings
	you don't want to manage. Likewise, you return '0' in your setSetting() implementation
	for the settings you don't manage. If this is not what you find you just want the value
	in cache you can use getCache() function.

	\section Handler Management

	Handler management can become complicated in some cases. Actually we have 2 types of settings,
	ones residing in file backend and ones residing in handlers. This implementation doesn't
	de-couple these two which is the sole reason behind this complication. Let's suppose we
	have some camera settings which should be updated in real-time using the camera module.
	On the other hand, we have some settings which should be written to camera module hardware
	at the very beginning. This is a controversy we should address. ApplicationSettings resolves
	this controversy by disabling custom handlers at the very beginning. Handlers are registered
	but they are not activated until they are enabled explicitly using enableHandlers() function.
*/

ApplicationSettings * ApplicationSettings::inst = NULL;

class AppSetNodeArray;
class AppSetNodeObject;

static AppSetNodeObject * json2app(const QJsonObject &obj);

class AppSetNode
{
public:
	QHash<QString, AppSetNodeObject *> objects;
	QHash<QString, AppSetNodeArray *> arrays;
};

class AppSetNodeObject : public AppSetNode
{
public:
	AppSetNodeObject()
	{
		handler = NULL;
		redirect = NULL;
	}

	QHash<QString, QVariant> nodes;
	AppSettingHandlerInterface *handler;
	AppSetNodeObject *redirect;
};

class AppSetNodeArray
{
public:
	QList<AppSetNodeObject *> list;
};

class ApplicationSettingsPriv {
public:
	AppSetNodeObject *root;
	bool handlersEnabled;
	QHash<QString, ApplicationSettings *> mappings;
	int numaratorTimerKick;
	QTimer *timerAutoSave;
	QString loadedFilename;

	QIODevice::OpenModeFlag openMode;
};

static QJsonObject app2json(const AppSetNodeObject *a)
{
	QJsonObject obj;
	QHashIterator<QString, QVariant> hi(a->nodes);
	while (hi.hasNext()) {
		hi.next();
		obj.insert(hi.key(), QJsonValue::fromVariant(hi.value()));
	}
	QHashIterator<QString, AppSetNodeObject *> oi(a->objects);
	while (oi.hasNext()) {
		oi.next();
		obj.insert(oi.key(), QJsonValue(app2json(oi.value())));
	}
	QHashIterator<QString, AppSetNodeArray *> ai(a->arrays);
	while (ai.hasNext()) {
		ai.next();
		const AppSetNodeArray *arr = ai.value();
		QJsonArray jarr;
		for (int i = 0; i < arr->list.size(); i++)
			jarr.append(app2json(arr->list[i]));
		obj.insert(ai.key(), jarr);
	}
	return obj;
}

static AppSetNodeArray * json2arr(const QJsonArray &arr)
{
	AppSetNodeArray *a = new AppSetNodeArray;
	for (int i = 0; i < arr.size(); i++) {
		if (arr.at(i).isObject())
			a->list << json2app(arr[i].toObject());
	}
	return a;
}

static AppSetNodeObject * json2app(const QJsonObject &obj)
{
	AppSetNodeObject *s = new AppSetNodeObject;
	const QStringList keys = obj.keys();
	foreach (const QString &key, keys) {
		const QJsonValue &val = obj.value(key);
		if (val.isObject()) {
			s->objects.insert(key, json2app(val.toObject()));
		} else if (val.isArray()) {
			s->arrays.insert(key, json2arr(val.toArray()));
		} else
			s->nodes.insert(key, val.toVariant());
	}
	return s;
}

QJsonValue ApplicationSettings::getValueList(const QString &key)
{
	if (!myobj.isEmpty())
		return myobj.value(key);
}

static void dump(const AppSetNodeObject *obj)
{
	ffDebug() << "nodes" << obj->nodes;
	QHashIterator<QString, AppSetNodeObject *> oi(obj->objects);
	while (oi.hasNext()) {
		oi.next();
		ffDebug() << "=============== obj ===============" << oi.key();
		dump(oi.value());
	}
	QHashIterator<QString, AppSetNodeArray *> ai(obj->arrays);
	while (ai.hasNext()) {
		ai.next();
		for (int i = 0; i < ai.value()->list.size(); i++)
			dump(ai.value()->list[i]);
	}
}

static AppSetNodeObject * getObject(AppSetNodeObject *parent, const QStringList &list)
{
	AppSetNodeObject *t = parent;
	for (int i = 0; i < list.size() - 1; i++) {
		bool ok;
		int ind = list[i + 1].toInt(&ok);
		if (ok) {
			/*
			 * next element is an array index, so this element
			 * should be an array
			 */
			if (!t->arrays.contains(list[i]))
				return NULL;
			AppSetNodeArray *arr = t->arrays[list[i]];
			if (arr->list.size() <= ind)
				return NULL;
			t = arr->list[ind];
			i++;
		} else {
			/* this is not an array index */
			if (!t->objects.contains(list[i]))
				return NULL;
			t = t->objects[list[i]];
		}
	}
	return t;
}

static AppSetNodeObject * cloneObject(const AppSetNodeObject *ref)
{
	AppSetNodeObject *obj = new AppSetNodeObject;
	QHashIterator<QString, QVariant> hin(ref->nodes);
	while (hin.hasNext()) {
		hin.next();
		obj->nodes.insert(hin.key(), hin.value());
	}
	QHashIterator<QString, AppSetNodeObject *> hio(ref->objects);
	while (hio.hasNext()) {
		hio.next();
		obj->objects.insert(hio.key(), cloneObject(hio.value()));
	}
	QHashIterator<QString, AppSetNodeArray *> hia(ref->arrays);
	while (hia.hasNext()) {
		hia.next();
		const AppSetNodeArray *arr = hia.value();
		AppSetNodeArray *arrn = new AppSetNodeArray;
		for (int i = 0; i < arr->list.size(); i++) {
			arrn->list << cloneObject(arr->list[i]);
		}
		obj->arrays.insert(hia.key(), arrn);
	}
	return obj;
}

static QString getHash(const QJsonObject &obj);
static QString getHash(const QJsonArray &arr);
static QString getHash(const QJsonValue &val)
{
	if (val.isArray())
		return getHash(val.toArray());
	if (val.isObject())
		return getHash(val.toObject());
	return "";
}

static QString getHash(const QJsonObject &obj)
{
	QString final;
	foreach (const QString &key, obj.keys()) {
		QString hash = getHash(obj.value(key));
		if (hash.isEmpty())
			hash = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex();
		final += hash;
	}

	return QCryptographicHash::hash(final.toUtf8(), QCryptographicHash::Md5).toHex();
}

static QString getHash(const QJsonArray &arr)
{
	/*
	 * we are interested only in one elements hash value
	 * as all array elements should have identical hashes.
	 * We just need to make sure that all elements
	 * are identical
	 */
	QStringList hashAll;
	foreach (const QJsonValue &val, arr) {
		QString hash = getHash(val);
		hashAll << hash;
	}
	QStringList filtered = hashAll.filter(hashAll.first());
	if (filtered != hashAll)
		return QCryptographicHash::hash(filtered.join(" ").toUtf8(), QCryptographicHash::Md5).toHex();
	if (hashAll.first().isEmpty())
		return "";
	return QCryptographicHash::hash(hashAll.first().toUtf8(), QCryptographicHash::Md5).toHex();
}

static QString getHash(const QJsonDocument &doc)
{
	QString hash;
	if (doc.isArray())
		hash = getHash(doc.array());
	else if (doc.isObject())
		hash = getHash(doc.object());
	return hash;
}

ApplicationSettings::ApplicationSettings(QObject *parent) :
	QObject(parent)
{
	p = new ApplicationSettingsPriv;
	p->root = NULL;
	p->handlersEnabled = false;
	p->numaratorTimerKick =  0;
	p->timerAutoSave = new QTimer(this);
	connect(p->timerAutoSave, SIGNAL(timeout()), this, SLOT(saveAll()));
}

QVariant ApplicationSettings::get(const QString &setting)
{
	if (!p->root)
		return QVariant();
	const QStringList &list = setting.split(".");
	AppSetNodeObject *t = p->root;
	for (int i = 0; i < list.size() - 1; i++) {
		bool ok;
		int ind = list[i + 1].toInt(&ok);
		if (ok) {
			/*
			 * next element is an array index, so this element
			 * should be an array
			 */
			if (!t->arrays.contains(list[i]))
				return QVariant();
			AppSetNodeArray *arr = t->arrays[list[i]];
			if (arr->list.size() <= ind)
				return QVariant();
			t = arr->list[ind];
			i++;
		} else {
			/* this is not an array index */
			if (!t->objects.contains(list[i]))
				return QVariant();
			t = t->objects[list[i]];
		}
	}

	/* redirect node */
	if (t->redirect)
		t = t->redirect;

	const QString &val = list[list.size() - 1];
	QVariant var;
	if (p->handlersEnabled && t->handler)
		var = t->handler->getSetting(setting);
	if (var.isValid())
		return var;
	if (!t->nodes.contains(val))
		return QVariant();
	return t->nodes[val];
}

int ApplicationSettings::set(const QString &setting, const QVariant &value)
{
	if (!p->root)
		return -ESRCH;
	const QStringList &list = setting.split(".");
	AppSetNodeObject *t = p->root;
	for (int i = 0; i < list.size() - 1; i++) {
		bool ok;
		int ind = list[i + 1].toInt(&ok);
		if (ok) {
			/*
			 * next element is an array index, so this element
			 * should be an array
			 */
			if (!t->arrays.contains(list[i]))
				return -EINVAL;
			AppSetNodeArray *arr = t->arrays[list[i]];
			if (arr->list.size() <= ind)
				return -ENOENT;
			t = arr->list[ind];
			i++;
		} else {
			/* this is not an array index */
			if (!t->objects.contains(list[i]))
				return -EINVAL;
			t = t->objects[list[i]];
		}
	}

	/* redirect node */
	if (t->redirect)
		t = t->redirect;

	const QString &val = list[list.size() - 1];
	int err = 0;
	if (p->handlersEnabled && t->handler)
		err = t->handler->setSetting(setting, value);
	if (err == -EROFS)
		return 0;
	if (err)
		return err;

	QVariant::Type type = t->nodes[val].type();
	QVariant var = value;
	if (value.canConvert(type) && (value.type() != type))
		var.convert(type);
	/* we don't need to check existence of a node here? */
	t->nodes.insert(val, var);

	/* perform delayed save but don't let delaying to last forever */
	if (p->numaratorTimerKick < 10) {
		p->numaratorTimerKick++;
		p->timerAutoSave->start(10000);
	}

	return 0;
}

const QStringList ApplicationSettings::getMembers(const QString &base)
{
	const QStringList &list = base.split(".");
	QStringList keys;
	AppSetNodeObject *obj = getObject(p->root, list);
	if (!obj)
		return keys;
	keys.append(obj->arrays.keys());
	keys.append(obj->objects.keys());
	keys.append(obj->nodes.keys());
	keys.sort();
	return keys;
}

/**
 * @brief ApplicationSettings::getArraySize
 * @param base
 * @return
 *
 * This function accepts an array node element, neither an object node nor an element index.
 * For an json like:
 *
 * "user_management": {
		"users": [
			{
				"username": "operator",
				"passwd": ""
			}
		]
	}

	you can get size of users array as getArraySize("user_management.users").
 */
int ApplicationSettings::getArraySize(const QString &base)
{
	QStringList list = base.split(".");
	QString arrayName = list.last();
	AppSetNodeObject *obj = getObject(p->root, list);
	if (!obj)
		return -ENOENT;
	if (!obj->arrays.contains(arrayName))
		return -ENOENT;
	return obj->arrays[arrayName]->list.size();
}

/**
 * @brief ApplicationSettings::appendToArray
 * @param base
 * @return
 *
 * Likewise getArraySize() function, this function also accepts an array node element.
 */
int ApplicationSettings::appendToArray(const QString &base)
{
	if (!p->root)
		return -ESRCH;
	const QStringList &list = base.split(".");
	QString arrayName = list.last();
	AppSetNodeObject *obj = getObject(p->root, list);
	if (!obj)
		return -ENOENT;
	if (!obj->arrays.contains(arrayName))
		return -ENOENT;
	AppSetNodeArray *arr = obj->arrays[arrayName];
	if (!arr->list.size())
		return -EINVAL;
	arr->list << cloneObject(arr->list.first());
	return 0;
}

/**
 * @brief ApplicationSettings::removeFromArray
 * @param base
 * @return
 *
 * Unlike appendToArray() function, this function accepts an element index. For example,
 * for the example json give in getArraySize() function description you remove an item like
 * this:
 *
 * removeFromArray("user_management.users.1"). If you pass an invalid index, you get an error.
 */
int ApplicationSettings::removeFromArray(const QString &base)
{
	if (!p->root)
		return -ESRCH;
	QStringList list = base.split(".");
	bool ok;
	int arrayIndex = list.last().toInt(&ok);
	if (!ok)
		return -EINVAL;
	list.removeLast();
	QString arrayName = list.last();
	AppSetNodeObject *obj = getObject(p->root, list);
	if (!obj)
		return -ENOENT;
	if (obj->arrays[arrayName]->list.size() <= arrayIndex)
		return -EINVAL;
	AppSetNodeObject *objd = obj->arrays[arrayName]->list.takeAt(arrayIndex);
	delete objd;
	return 0;
}

QStringList ApplicationSettings::getArray(const QString &base, const QString &member)
{
	QStringList l;
	const QStringList keyParse = base.split(".");
	const QString &arrayName = keyParse.last();
	AppSetNodeObject *obj = getObject(p->root, keyParse);

	if (!obj)
		return l;

	/* TODO: we should re-implement the following, 'member' is lost along-the-way  */
	if (p->handlersEnabled && obj->handler)
		return obj->handler->getSetting(base).toStringList();

	if (!obj->arrays.contains(arrayName))
		return l;

	const QList<AppSetNodeObject *> &appList =  obj->arrays[arrayName]->list;
	if (appList.size() && !appList.first()->nodes.contains(member))
		return l;
	for (int i = 0; i < appList.size() ; i++)
		l.append(appList[i]->nodes[member].toString());

	return l;
}

int ApplicationSettings::addHandler(const QString &base, AppSettingHandlerInterface *h)
{
	if (!p->root)
		return -ESRCH;
	QStringList list = base.split(".");
	if (!base.endsWith("."))
		list << "";
	AppSetNodeObject *obj = getObject(p->root, list);
	if (!obj)
		return -ENOENT;
	obj->handler = h;

	if (obj->objects.size())
		foreach (QString key, obj->objects.keys())
			addHandler(QString("%1.%2").arg(base).arg(key), h);

	if (obj->arrays.size()) {
		foreach (QString key, obj->arrays.keys()) {
			AppSetNodeArray *arr = obj->arrays[key];
			for (int i = 0; i < arr->list.size() ; i++)
				addHandler(QString("%1.%2.%3").arg(base).arg(key).arg(i), h);
		}
	}

	return 0;
}

QVariant ApplicationSettings::getCache(const QString &setting)
{
	if (!p->root)
		return -ESRCH;
	const QStringList &list = setting.split(".");
	AppSetNodeObject *t = getObject(p->root, list);
	if (!t)
		return QVariant();
	const QString &val = list[list.size() - 1];
	if (!t->nodes.contains(val))
		return QVariant();
	return t->nodes[val];
}

void ApplicationSettings::enableHandlers(bool value, bool recursive)
{
	p->handlersEnabled = value;
	if (!recursive)
		return;
	QHashIterator<QString, ApplicationSettings *> hi(p->mappings);
	while (hi.hasNext()) {
		hi.next();
		hi.value()->p->handlersEnabled = value;
	}
}

void ApplicationSettings::addMapping(const QString &group, ApplicationSettings *s)
{
	p->mappings.insert(group, s);
}

void ApplicationSettings::addMapping(const QString &filename)
{
	ApplicationSettings *sets = ApplicationSettings::create(filename, p->openMode);
	const QStringList &keys = sets->getMembers("");
	foreach (const QString &key, keys)
		if (key != "config")
			addMapping(key, sets);
}

ApplicationSettings * ApplicationSettings::getMapping(const QString &setting)
{
	const QStringList &vals = setting.split(".");
	if (p->mappings.contains(vals.first()))
		return p->mappings[vals.first()];
	return ApplicationSettings::instance();
}

QVariant ApplicationSettings::getm(const QString &setting)
{
	return getMapping(setting)->get(setting);
}

int ApplicationSettings::setm(const QString &setting, const QVariant &value)
{
	return getMapping(setting)->set(setting, value);
}

void ApplicationSettings::readIncrementalSettings(const QString &group)
{

}

int ApplicationSettings::setupRedirection(const QString &from, const QString &to)
{
	QStringList keyFrom = from.split(".");
	QStringList keyTo = to.split(".");
	getObject(p->root, keyFrom)->redirect = getObject(p->root, keyTo);
	return 0;
}

QVariant ApplicationSettings::getJson(const QString &setting)
{
	if (!p->root)
		return QVariant();
	const QStringList &list = setting.split(".");
	AppSetNodeObject *t = p->root;
	for (int i = 0; i < list.size() - 1; i++) {
		bool ok;
		int ind = list[i + 1].toInt(&ok);
		if (ok) {
			/*
			 * next element is an array index, so this element
			 * should be an array
			 */
			if (!t->arrays.contains(list[i]))
				return QVariant();
			AppSetNodeArray *arr = t->arrays[list[i]];
			if (arr->list.size() <= ind)
				return QVariant();
			t = arr->list[ind];
			i++;
		} else {
			/* this is not an array index */
			if (!t->objects.contains(list[i]))
				return QVariant();
			t = t->objects[list[i]];
		}
	}

	/* redirect node */
	if (t->redirect)
		t = t->redirect;

	const QString &val = list[list.size() - 1];
	if (!t->nodes.contains(val))
		return QVariant();
	return t->nodes[val];
}

int ApplicationSettings::setJson(const QString &setting, const QVariant &value)
{
	if (!p->root)
		return -ESRCH;
	const QStringList &list = setting.split(".");
	AppSetNodeObject *t = p->root;
	for (int i = 0; i < list.size() - 1; i++) {
		bool ok;
		int ind = list[i + 1].toInt(&ok);
		if (ok) {
			/*
			 * next element is an array index, so this element
			 * should be an array
			 */
			if (!t->arrays.contains(list[i]))
				return -EINVAL;
			AppSetNodeArray *arr = t->arrays[list[i]];
			if (arr->list.size() <= ind)
				return -ENOENT;
			t = arr->list[ind];
			i++;
		} else {
			/* this is not an array index */
			if (!t->objects.contains(list[i]))
				return -EINVAL;
			t = t->objects[list[i]];
		}
	}

	/* redirect node */
	if (t->redirect)
		t = t->redirect;

	const QString &val = list[list.size() - 1];

	QVariant::Type type = t->nodes[val].type();
	QVariant var = value;
	if (value.canConvert(type) && (value.type() != type))
		var.convert(type);
	/* we don't need to check existence of a node here? */
	t->nodes.insert(val, var);

	/* perform delayed save but don't let delaying to last forever */
	if (p->numaratorTimerKick < 10) {
		p->numaratorTimerKick++;
		p->timerAutoSave->start(10000);
	}

	return 0;
}

void ApplicationSettings::saveAll()
{
	if(p->timerAutoSave->isActive())
		p->timerAutoSave->stop();
	p->numaratorTimerKick = 0;
	save(p->loadedFilename);
}

ApplicationSettings * ApplicationSettings::instance()
{
	if (!inst)
		inst = new ApplicationSettings;
	return inst;
}

ApplicationSettings * ApplicationSettings::create(const QString &filename, QIODevice::OpenModeFlag openMode)
{
	ApplicationSettings *s = new ApplicationSettings;
	s->load(filename, openMode);
	return s;
}

QString ApplicationSettings::getFileHash(const QString &filename)
{
	QFile f(filename);
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		return "";
	const QByteArray &json = f.readAll();
	f.close();
	const QJsonDocument &doc = QJsonDocument::fromJson(json);
	return getHash(doc);
}

int ApplicationSettings::load(const QString &filename, QIODevice::OpenModeFlag openMode)
{
	QFile f(filename);
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		return -ENOENT;
	const QByteArray &json = f.readAll();
	f.close();

	QJsonDocument doc(QJsonDocument::fromJson(json));
	if (!doc.isObject())
		return -EINVAL;
	QJsonObject root = doc.object();
	myobj = root;
	p->root = json2app(root);
	p->loadedFilename = filename;
	p->openMode = openMode;
	//dump(p->root);

	return 0;
}

int ApplicationSettings::save(const QString &filename)
{
	if ((p->openMode & QIODevice::WriteOnly) == 0)
		return -EACCES;
	QString tmpname = QString(filename).replace(".json", ".tmp");
	QFile f(tmpname);
	if (!f.open(QIODevice::WriteOnly))
		return -EPERM;
	f.write(QJsonDocument(app2json(p->root)).toJson());
	fsync(f.handle());
	f.close();
	rename(qPrintable(tmpname), qPrintable(filename));
	/* w/o sync() jffs2 fails */
	::sync();
	return 0;
}
