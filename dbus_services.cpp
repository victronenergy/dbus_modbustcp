#include <QsLog.h>
#include <velib/qt/ve_qitem.hpp>
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
	QMap<QString, QList<DBusService *> >::iterator it = mServiceByType.find(deviceType);
	if (it == mServiceByType.end())
		return 0;
	DBusService *last = 0;
	DBusService *lastConnected = 0;
	foreach (DBusService *service, *it) {
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
	QMap<QString, DBusService *>::Iterator it = mServicesByName.find(name);
	if (it != mServicesByName.end())
		return;

	QLOG_TRACE() << "[DBusServices] Add new service " << name;
	DBusService *service = new DBusService(item);
	mServicesByName.insert(name, service);

	QString deviceType = service->getDeviceType(name);
	QList<DBusService *> &device = mServiceByType[deviceType];
	device.append(service);
	emit dbusServiceFound(service);
}
