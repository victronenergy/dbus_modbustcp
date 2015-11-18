#include <QsLog.h>
#include <velib/qt/ve_qitem.hpp>
#include "dbus_service.h"
#include "dbus_services.h"
#include "defines.h"

DBusServices::DBusServices(VeQItem *root, QObject *parent) :
	QObject(parent),
	mRoot(root)
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
	for (int i=0;;++i) {
		VeQItem *item = mRoot->itemChild(i);
		if (item == 0)
			break;
		onServiceAdded(item);
	}
	connect(mRoot, SIGNAL(childAdded(VeQItem *)),
			this, SLOT(onServiceAdded(VeQItem *)));
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

void DBusServices::onServiceAdded(VeQItem *service)
{
	VeQItem *item = service->itemGetOrCreate("/DeviceInstance");
	connect(item, SIGNAL(stateChanged(VeQItem *, State)),
			this, SLOT(onDeviceInstanceFound(VeQItem *)));
	item->getValue();
	if (item->getState() == VeQItem::Synchronized)
		addService(service);
}

void DBusServices::onDeviceInstanceFound(VeQItem *item)
{
	switch (item->getState()) {
	case VeQItem::Idle:
		item->getValue();
		break;
	case VeQItem::Synchronized:
		addService(item->itemParent());
		break;
	case VeQItem::Offline:
		// There are 3 scenario's to get here:
		// 1. D-Bus service found earlier and disappeared. Device instance
		//    state will change from Synchronized to Offline. In this case
		//    service will already exist, so addService is a no-op.
		// 2. D-Bus service was just added and disappears before device
		//    instance was retrieved. State will change from Requested to
		//    offline. We will add the service and disable it directly.
		//    This behavior is similar to (1).
		// 3. D-Bus service was just added and has no /DeviceInstance path.
		//    state will change from Requested to offline. In this case we
		//    also create a disabled service which can be used as long as
		//    no other D-Bus service turns up with a /DeviceInstance path.
		//    We do this for backward compatibility with older version of this
		//    service.
		addService(item->itemParent());
		removeService(item->itemParent());
		break;
	default:
		break;
	}
}

void DBusServices::addService(VeQItem *item)
{
	QString name = item->id();
	QMap<QString, DBusService *>::Iterator it = mServicesByName.find(name);
	if (it != mServicesByName.end()) {
		if (!it.value()->getConnected()) {
			QLOG_TRACE() << "[DBusServices] connect " << name;
			it.value()->setConnected(true);
		}
		return;
	}

	QLOG_TRACE() << "[DBusServices] Add new service " << name;
	DBusService *service = new DBusService(item);
	mServicesByName.insert(name, service);

	QString deviceType = service->getDeviceType(name);
	QMultiMap<int, DBusService *> &device = mServiceByType[deviceType];
	device.insert(service->getDeviceInstance(), service);
	emit dbusServiceFound(service);
}

void DBusServices::removeService(VeQItem *item)
{
	QString name = item->id();
	QMap<QString, DBusService *>::Iterator it = mServicesByName.find(name);
	if (it == mServicesByName.end() || !it.value()->getConnected())
		return;

	QLOG_TRACE() << "[DBusServices] Remove old service " << name;
	it.value()->setConnected(false);
}
