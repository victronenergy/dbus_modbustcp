#include <qstringlist.h>
#include <QEvent>
#include <velib/qt/ve_qitem.hpp>
#include "dbus_service.h"
#include "defines.h"
#include "QsLog.h"

DBusService::DBusService(VeQItem *serviceRoot, QObject *parent) :
	QObject(parent),
	mServiceRoot(serviceRoot),
	mServiceType(getDeviceType(serviceRoot->id())),
	mConnected(true),
	mDeviceInstance(serviceRoot->itemGetOrCreate("/DeviceInstance"))
{
	QLOG_INFO() << "[DBusService] DeviceInstance of" << getServiceName()
				<< "=" << mDeviceInstance->getValue().toInt()
				<< "with type" << mServiceType;
}

QString DBusService::getServiceName() const
{
	return mServiceRoot->id();
}

QString DBusService::getDeviceType(const QString &name)
{
	QStringList elements = name.split(".");
	if (elements.count() < 3)
		return "Unkown";

	return elements[2];
}

void DBusService::setDeviceType(const QString &name)
{
	mServiceType = getDeviceType(name);
}

void DBusService::registerObjects(const QStringList &pathList)
{
	foreach(const QString &path, pathList)
		registerObject(path);
}

void DBusService::registerObject(const QString &path)
{
	Q_ASSERT(path.startsWith('/'));
	QLOG_INFO() << "[DBusService] registerObject " << getServiceName() << path;
	VeQItem *item = mServiceRoot->itemGetOrCreate(path);
	item->getValue();
}

QPair<QVariant, bool> DBusService::getValue(const QString &path) const
{
	VeQItem *item = mServiceRoot->itemGet(path);
	Q_ASSERT(item != 0);
	if (item == 0 || item->getState() == VeQItem::Requested)
		return QPair<QVariant, bool>(QVariant(), false);
	return QPair<QVariant, bool>(item->getValue(), true);
}

bool DBusService::setValue(const QString &path, const QVariant &value)
{
	VeQItem *item = mServiceRoot->itemGet(path);
	if (item == 0)
		return false;
	return item->setValue(value) == 0;
}
