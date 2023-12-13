#include <QsLog.h>
#include <veutil/qt/ve_qitem.hpp>
#include "dbus_service.h"
#include "dbus_services.h"

DBusServices::DBusServices(VeQItem *root, QObject *parent) :
	QObject(parent),
	mRoot(root)
{
}

DBusServices::~DBusServices()
{
	foreach (const QList<DBusService *> &services, mServiceByType) {
		foreach (DBusService *service, services)
			delete service;
	}
}

void DBusServices::initialScan()
{
	for (int i=0;;++i) {
		VeQItem *item = mRoot->itemChild(i);
		if (item == 0)
			break;
		onServiceAdded(item);
	}
	connect(mRoot, SIGNAL(childAdded(VeQItem *)),
			this, SLOT(onServiceAdded(VeQItem *)));
}

DBusService *DBusServices::getService(QString deviceType, int deviceInstance)
{
	QList<DBusService *> services = mServiceByType.value(deviceType);
	if (services.isEmpty())
		return 0;
	DBusService *last = 0;
	DBusService *lastConnected = 0;
	foreach (DBusService *service, services) {
		VeQItem *serviceInstance = service->getDeviceInstance();
		QVariant v = serviceInstance->getValue();
		if (v.toInt() == deviceInstance) {
			switch (serviceInstance->getState()) {
			case VeQItem::Synchronized:
				lastConnected = service;
				// Fall through
			default:
				last = service;
				break;
			}
		}
	}
	return lastConnected == 0 ? last : lastConnected;
}

void DBusServices::onServiceAdded(VeQItem *item)
{
	QString name = item->id();

	// Skip our own service
	if (name == "com.victronenergy.modbustcp")
		return;

	DBusService *service = mServicesByName.value(name);
	if (service != 0)
		return;

	QLOG_TRACE() << "[DBusServices] Add new service " << name;
	service = new DBusService(item);
	mServicesByName.insert(name, service);

	QString deviceType = service->getDeviceType(name);
	QList<DBusService *> &device = mServiceByType[deviceType];
	device.append(service);
	emit dbusServiceFound(service);
}
