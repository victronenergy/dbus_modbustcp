#include <qstringlist.h>
#include <QEvent>

#include "dbus_service.h"
#include "defines.h"
#include "QsLog.h"

DBusService::DBusService(const QString &serviceName, QObject *parent) :
	QObject(parent),
	mDbusServiceName(serviceName),
	mServiceType(DBUS_SERVICE_UNKNOWN),
	mConnected(true),
	mDeviceInstance(serviceName, "/DeviceInstance", DBUS_CONNECTION)
{
	mDeviceInstance.getValue();
	setDeviceType(serviceName);
	QLOG_INFO() << "[DBusService] DeviceInstance of " << serviceName << "=" << mDeviceInstance.getValue().toInt();
}

DBusService::DbusServiceType DBusService::getDeviceType(const QString &name)
{
	QStringList elements = name.split(".");
	if (elements.count() < 3)
		return DBUS_SERVICE_UNKNOWN;

	QString type = elements[2];

	if (type == "battery")
		return DBUS_SERVICE_BATTERY;
	if (type == "vebus")
		return DBUS_SERVICE_MULTI;
	if (type == "solarcharger")
		return DBUS_SERVICE_SOLAR_CHARGER;
	if (type == "lynxion")
		return DBUS_SERVICE_LYNX_ION;
	if (type == "pvinverter")
		return DBUS_SERVICE_PV_INVERTER;
	if (type == "logger")
		return DBUS_SERVICE_LOGGER;
	if (type == "settings")
		return DBUS_SERVICE_SETTINGS;
	if (type == "gps")
		return DBUS_SERVICE_GPS;

	return DBUS_SERVICE_UNKNOWN;
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
