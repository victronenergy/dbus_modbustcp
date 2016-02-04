#include "dbus_service.h"
#include "dbus_services.h"
#include "defines.h"

//#define QS_LOG_DISABLE
#include "QsLog.h"

DBusServices::DBusServices(QObject *parent) :
	QObject(parent),
	mDBus(DBUS_CONNECTION)
{
}

DBusServices::~DBusServices()
{
	for (QMap<QString, QMultiMap<int, DBusService *> >::iterator it = mServiceByType.begin();
		 it != mServiceByType.end(); ++it) {
		for (QMultiMap<int, DBusService *>::iterator it2 = it->begin(); it2 != it->end(); ++it2) {
			delete it2.value();
		}
	}
}

void DBusServices::initialScan()
{
	QDBusConnectionInterface *interface = mDBus.interface();
	QStringList serviceNames = interface->registeredServiceNames();
	connect(interface, SIGNAL(serviceOwnerChanged(QString,QString,QString)), SLOT(serviceOwnerChanged(QString,QString,QString)));

	foreach (const QString &name, serviceNames)
		addService(name);
}

DBusService * DBusServices::getService(QString deviceType, int deviceInstance)
{
	QMap<QString, QMultiMap<int, DBusService *> >::iterator it =
		mServiceByType.find(deviceType);
	if (it == mServiceByType.end())
		return 0;
	DBusService *last = 0;
	DBusService *lastConnected = 0;
	for (QMultiMap<int, DBusService *>::Iterator it2 = it->find(deviceInstance);
		 it2 != it->end() && it2.key() == deviceInstance; ++it2) {
		last = it2.value();
		if (last->getConnected())
			lastConnected = last;
	}
	return lastConnected == 0 ? last : lastConnected;
}

void DBusServices::addService(const QString &name)
{
	if (!name.startsWith("com.victronenergy."))
		return;

	if (mServicesByName.contains(name)) {
		QLOG_TRACE() << "[DBusServices] connect " << name;
		mServicesByName.value(name)->setConnected(true);
		return;
	}

	QLOG_TRACE() << "[DBusServices] Add new service " << name;
	DBusService *service = new DBusService(name);
	mServicesByName.insert(name, service);

	QString deviceType = service->getDeviceType(name);
	QMultiMap<int, DBusService *> &device = mServiceByType[deviceType];
	device.insert(service->getDeviceInstance(), service);
	emit dbusServiceFound(service);
}

void DBusServices::removeService(const QString &name)
{
	if (!mServicesByName.contains(name))
		return;

	QLOG_TRACE() << "[DBusServices] Remove old service " << name;
	mServicesByName.value(name)->setConnected(false);
}

void DBusServices::serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
	Q_UNUSED(oldOwner);
	if (!newOwner.isEmpty() ) {
		// new owner > service add on dbus
		addService(name);
	} else {
		// no new owner > service removed from dbus
		removeService(name);
	}
}
