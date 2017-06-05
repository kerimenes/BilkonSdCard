#include "jsonhelper.h"

#include <QFile>
#include <QDebug>
#include <unistd.h>
#include <QJsonArray>

JsonHelper::JsonHelper(const QString &file)
{
	filename = file;
	obj = jsonRead(filename);
	if (obj.isEmpty())
		return;
}

QJsonObject JsonHelper::jsonRead(const QString &filename)
{
	QFile f(filename);
	if (!f.open(QIODevice::ReadWrite | QIODevice::Text))
		return QJsonObject();
	const QByteArray &json = f.readAll();
	f.close();

	QJsonDocument doc(QJsonDocument::fromJson(json));
	if(!doc.isObject())
		return QJsonObject();
	QJsonObject root = doc.object();
	//	qDebug() << root.value("SDK_path").toString();
	return root;
}

int JsonHelper::save()
{
	QString tmpname = QString(filename).replace(".json", ".tmp");
	QFile f(tmpname);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QFile::Truncate))
		return -2;
	f.write(QJsonDocument(obj).toJson());
	fsync(f.handle());
	f.close();
	rename(qPrintable(tmpname), qPrintable(filename));
	/* w/o sync() jffs2 fails */
	::sync();
	return 0;
}

QString JsonHelper::value(QString key)
{
	QString str;
	if(key.contains(".")) {
		QStringList flds = key.split(".");
		QJsonValue obj2 = obj.value(flds.at(0));
		for (int i = 1; i < flds.size(); i++) {
			obj2 = obj2.toObject().value(flds.at(i));
			str = obj2.toString();
		}
		str = replaceVariable(str);
	} else {
		str = obj.value(key).toString();
		str = replaceVariable(str);
	}
	return str;
}

QJsonObject JsonHelper::valueObject(QString key)
{
	return obj.value(key).toObject();
}

int JsonHelper::insert(const QString &key, const QString &value)
{
	if (key.contains(".")) {
		QStringList flds = key.split(".");
		QJsonObject stats_obj;
		stats_obj = obj.value(flds.at(0)).toObject();
		stats_obj[flds.at(1)] = value;
		obj.insert(flds.at(0), stats_obj);
	} else {
		if (key.isEmpty())
			return -1;
		obj.insert(key, value);
		return 0;
	}
}

QString JsonHelper::replaceVariable(QString str)
{
	while (str.contains("$")) {
		int start = str.indexOf("$");
		int end = str.indexOf(QRegExp("\\W"), start + 1);
		QString var = str.mid(start + 1, end - start - 1);
		if (end < 0)
			end = str.size();
		str = str.replace(start, end - start, obj.value(var).toString());
	}
	return str;
}
