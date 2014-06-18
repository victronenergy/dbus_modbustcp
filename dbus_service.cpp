#include <qstringlist.h>
#include <QEvent>

#include "dbus_service.h"
#include "defines.h"
#include "QsLog.h"

DBusService::DBusService(const QString &serviceName, QObject *parent) :
	QObject(parent),
	mDbusServiceName(serviceName),
	mServiceType(getDeviceType(serviceName)),
	mConnected(true),
	mDeviceInstance(serviceName, "/DeviceInstance", DBUS_CONNECTION)
{
	mDeviceInstance.getValue();
	QLOG_INFO() << "[DBusService] DeviceInstance of " << serviceName << "=" << mDeviceInstance.getValue().toInt() << "with type" << mServiceType;
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
	QLOG_INFO() << "[DBusService] registerObject " << mDbusServiceName << path;
	BusItemCons * busitem = new BusItemCons(mDbusServiceName, path, DBUS_CONNECTION);
	mBusItems.insert(path, busitem);
}

QVariant DBusService::getValue(const QString path)
{
	if (mBusItems.contains(path)) {
		return mBusItems.value(path)->getValue();
	} else
		return QVariant();
}
