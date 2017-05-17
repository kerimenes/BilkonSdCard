#ifndef APPLICATIONSETTINGS_H
#define APPLICATIONSETTINGS_H

#include <QObject>
#include <QVariant>
#include <QIODevice>

#include <QJsonObject>

#define strcontains(__str) setting.contains(__str, Qt::CaseInsensitive)
#define equals(__str) !setting.compare(__str, Qt::CaseInsensitive)
#define starts(__str) setting.startsWith(__str, Qt::CaseInsensitive)
#define ends(__str) setting.endsWith(__str, Qt::CaseInsensitive)

class ApplicationSettingsPriv;

class AppSettingHandlerInterface
{
public:
	virtual QVariant getSetting(const QString &setting) = 0;
	virtual int setSetting(const QString &setting, const QVariant &value) = 0;
};

class ApplicationSettings : QObject
{
	Q_OBJECT
public:
	static ApplicationSettings * instance();
	static ApplicationSettings * create(const QString &filename, QIODevice::OpenModeFlag openMode);
	static QString getFileHash(const QString &filename);

	int load(const QString &filename, QIODevice::OpenModeFlag openMode);
	QVariant get(const QString &setting);
	int set(const QString &setting, const QVariant &value);
	const QStringList getMembers(const QString &base);
	int getArraySize(const QString &base);
	int appendToArray(const QString &base);
	int removeFromArray(const QString &base);
	QStringList getArray(const QString &base, const QString &member);

	int addHandler(const QString &base, AppSettingHandlerInterface *h);
	QVariant getCache(const QString &setting);
	void enableHandlers(bool value, bool recursive = false);

	void addMapping(const QString &group, ApplicationSettings *s);
	void addMapping(const QString &filename);
	ApplicationSettings * getMapping(const QString &setting);
	QVariant getm(const QString &setting);
	int setm(const QString &setting, const QVariant &value);

	void readIncrementalSettings(const QString &group = "");

	int setupRedirection(const QString &from, const QString &to);

	QVariant getJson(const QString &setting);
	int setJson(const QString &setting, const QVariant &value);

	QJsonValue getValueList(const QString &key);
signals:

public slots:
	void saveAll();
protected:
	explicit ApplicationSettings(QObject *parent = 0);
	int save(const QString &filename);
	ApplicationSettingsPriv *p;
	QJsonObject myobj;

	static ApplicationSettings *inst;

};

#endif // APPLICATIONSETTINGS_H
