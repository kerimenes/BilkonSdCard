#ifndef JSONHELPER_H
#define JSONHELPER_H

#include "qjsonmodel.h"

class JsonHelper
{
public:
	JsonHelper(const QString &file);
	int save();
	QString value(QString key);
	QJsonObject valueObject(QString key);
	int insert(const QString &key, const QString &value);
protected:
	QString replaceVariable(QString str);
	QJsonObject jsonRead(const QString &filename);
private:
	QJsonObject obj;
	QString filename;
};

#endif // JSONHELPER_H
