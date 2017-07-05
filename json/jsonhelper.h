#ifndef JSONHELPER_H
#define JSONHELPER_H

#include "qjsonmodel.h"

class JsonHelper : public QObject
{
	Q_OBJECT
public:
	JsonHelper(const QString &file);
	int save();
	QString value(QString key);
	QJsonObject valueObject(QString key);
	int insert(const QString &key, const QString &value);
protected:
	QString replaceVariable(QString str);
	QJsonObject jsonRead(const QString &filename);
protected slots:
	void saveAll();
private:
	QJsonObject obj;
	QString filename;
	QTimer *timer;
};

#endif // JSONHELPER_H
