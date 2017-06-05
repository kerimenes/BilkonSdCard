/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2011 SCHUTZ Sacha
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "qjsonmodel.h"
#include <QFile>
#include <QDebug>
#include <QFont>

#if __cplusplus < 201103
#error You cannot use this module without c++11
#endif

QJsonTreeItem::QJsonTreeItem(QJsonTreeItem *parent)
{
	mParent = parent;
}

QJsonTreeItem::~QJsonTreeItem()
{
	qDeleteAll(mChilds);
}

void QJsonTreeItem::appendChild(QJsonTreeItem *item)
{
	mChilds.append(item);
}

QJsonTreeItem *QJsonTreeItem::child(int row)
{
	return mChilds.value(row);
}

QJsonTreeItem *QJsonTreeItem::parent()
{
	return mParent;
}

int QJsonTreeItem::childCount() const
{
	return mChilds.count();
}

int QJsonTreeItem::row() const
{
	if (mParent)
		return mParent->mChilds.indexOf(const_cast<QJsonTreeItem*>(this));

	return 0;
}

void QJsonTreeItem::setKey(const QString &key)
{
	mKey = key;
}

void QJsonTreeItem::setValue(const QVariant &value)
{
	mValue = value;
}

void QJsonTreeItem::setType(const QJsonValue::Type &type)
{
	mType = type;
}

QString QJsonTreeItem::key() const
{
	return mKey;
}

QVariant QJsonTreeItem::value() const
{
	return mValue;
}

QJsonValue::Type QJsonTreeItem::type() const
{
	return mType;
}

QJsonTreeItem* QJsonTreeItem::load(const QJsonValue& value, QJsonTreeItem* parent)
{
	QJsonTreeItem * rootItem = new QJsonTreeItem(parent);
	rootItem->setKey("root");

	if ( value.isObject())
	{
		//Get all QJsonValue childs
		for (QString key : value.toObject().keys()){
			QJsonValue v = value.toObject().value(key);
			QJsonTreeItem * child = load(v,rootItem);
			child->setKey(key);
			child->setType(v.type());
			rootItem->appendChild(child);

		}

	}

	else if ( value.isArray())
	{
		//Get all QJsonValue childs
		int index = 0;
		for (QJsonValue v : value.toArray()){
			QJsonTreeItem * child = load(v,rootItem);
			child->setKey(QString::number(index));
			child->setType(v.type());
			rootItem->appendChild(child);
			++index;
		}
	}
	else
	{
		rootItem->setValue(value.toVariant());
		rootItem->setType(value.type());
	}

	return rootItem;
}

//=========================================================================

QJsonModel::QJsonModel(QObject *parent) :
	QAbstractItemModel(parent)
{
	mRootItem = new QJsonTreeItem;
	mHeaders.append("key");
	mHeaders.append("value");


}

bool QJsonModel::load(const QString &fileName)
{
	QFile file(fileName);
	bool success = false;
	if (file.open(QIODevice::ReadOnly)) {
		success = load(&file);
		file.close();
	}
	else success = false;

	return success;
}

bool QJsonModel::load(QIODevice *device)
{
	return loadJson(device->readAll());
}

bool QJsonModel::loadJson(const QByteArray &json)
{
	mDocument = QJsonDocument::fromJson(json);

	if (!mDocument.isNull())
	{
		beginResetModel();
		if (mDocument.isArray()) {
			mRootItem = QJsonTreeItem::load(QJsonValue(mDocument.array()));
			mRootItem->setType(QJsonValue::Array);
		} else {
			mRootItem = QJsonTreeItem::load(QJsonValue(mDocument.object()));
			mRootItem->setType(QJsonValue::Object);
		}
		endResetModel();
		return true;
	}

	qDebug()<<Q_FUNC_INFO<<"cannot load json";
	return false;
}

QJsonDocument QJsonModel::toJson()
{
	if (mRootItem->type() == QJsonValue::Object) {
		QJsonObject obj;
		toJson(mRootItem, obj);
		return QJsonDocument(obj);
	} else if (mRootItem->type() == QJsonValue::Array) {
		QJsonArray arr;
		toJson(mRootItem, arr);
		return QJsonDocument(arr);
	}
	return QJsonDocument();
}


QVariant QJsonModel::data(const QModelIndex &index, int role) const
{

	if (!index.isValid())
		return QVariant();


	QJsonTreeItem *item = static_cast<QJsonTreeItem*>(index.internalPointer());


	if (role == Qt::DisplayRole) {

		if (index.column() == 0)
			return QString("%1").arg(item->key());

		if (index.column() == 1)
			return item->value();
	}



	return QVariant();

}

bool QJsonModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid())
		return false;

	QJsonTreeItem *item = static_cast<QJsonTreeItem*>(index.internalPointer());
	if (role == Qt::EditRole) {
		QVariant var = value;
		var.convert(item->value().type());
		if (index.column() == 1)
			item->setValue(var);
	}

	return true;
}

Qt::ItemFlags QJsonModel::flags(const QModelIndex &index) const
{
	if (index.column())
		return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	return QAbstractItemModel::flags(index);
}

QVariant QJsonModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal) {

		return mHeaders.value(section);
	}
	else
		return QVariant();
}

QModelIndex QJsonModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	QJsonTreeItem *parentItem;

	if (!parent.isValid())
		parentItem = mRootItem;
	else
		parentItem = static_cast<QJsonTreeItem*>(parent.internalPointer());

	QJsonTreeItem *childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);
	else
		return QModelIndex();
}

QModelIndex QJsonModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	QJsonTreeItem *childItem = static_cast<QJsonTreeItem*>(index.internalPointer());
	QJsonTreeItem *parentItem = childItem->parent();

	if (parentItem == mRootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int QJsonModel::rowCount(const QModelIndex &parent) const
{
	QJsonTreeItem *parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		parentItem = mRootItem;
	else
		parentItem = static_cast<QJsonTreeItem*>(parent.internalPointer());

	return parentItem->childCount();
}

int QJsonModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return 2;
}

void QJsonModel::toJson(QJsonTreeItem *item, QJsonObject &obj)
{
	for (int i = 0; i < item->childCount(); i++) {
		QJsonTreeItem *child = item->child(i);
		if (child->type() == QJsonValue::Object) {
			QJsonObject cobj;
			toJson(child, cobj);
			obj.insert(child->key(), cobj);
		} else if (child->type() == QJsonValue::Array) {
			QJsonArray carr;
			toJson(child, carr);
			obj.insert(child->key(), carr);
		} else
			obj.insert(child->key(), QJsonValue::fromVariant(child->value()));
	}
}

void QJsonModel::toJson(QJsonTreeItem *item, QJsonArray &arr)
{
	for (int i = 0; i < item->childCount(); i++) {
		QJsonTreeItem *child = item->child(i);
		if (child->type() == QJsonValue::Object) {
			QJsonObject cobj;
			toJson(child, cobj);
			arr.append(cobj);
		} else if (child->type() == QJsonValue::Array) {
			QJsonArray carr;
			toJson(child, carr);
			arr.append(carr);
		} else
			arr.append(QJsonValue::fromVariant(child->value()));
	}
}

