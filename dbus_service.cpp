#include <qstringlist.h>
#include <QEvent>

#include "dbus_service.h"
#include "QsLog.h"

DBusService::DBusService(const QDBusConnection &dbus, const QString &serviceName, QObject *parent) :
	QObject(parent),
	mDbusServiceName(serviceName),
	mServiceType(getDeviceType(serviceName)),
	mConnected(true),
	mDeviceInstance(serviceName, "/DeviceInstance", dbus),
	mDBus(dbus)
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
	QHash<QString, BusItemCons *>::Iterator it = mBusItems.find(path);
	if (it == mBusItems.end()) {
		QLOG_DEBUG() << "[DBusService] registerObject " << mDbusServiceName << path;
		BusItemCons * busitem = new BusItemCons(mDbusServiceName, path, mDBus);
		mBusItems.insert(path, busitem);
	} else {
		it.value()->getValue(true);
	}
}

QVariant DBusService::getValue(const QString path) const
{
	if (mBusItems.contains(path)) {
		return mBusItems.value(path)->getValue();
	} else
		return QVariant();
}

bool DBusService::setValue(const QString path, const QVariant value)
{
	if (mBusItems.contains(path))
		return mBusItems.value(path)->setValue(value) == 0 ? true : false;
	else
		return false;
}
