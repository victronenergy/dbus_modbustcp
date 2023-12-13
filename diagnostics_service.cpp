#include <QDateTime>
#include <QTimer>
#include <veutil/qt/ve_qitem.hpp>
#include "dbus_service.h"
#include "dbus_services.h"
#include "diagnostics_service.h"
#include "mappings.h"

DiagnosticsService::DiagnosticsService(DBusServices *services, Mappings *mappings, VeQItem *root,
									   QObject *parent):
	QObject(parent),
	mLastErrorTimer(new QTimer(this)),
	mServices(services),
	mMappings(mappings),
	mRoot(root),
	mLastError(root->itemGetOrCreate("LastError/Message")),
	mLastErrorTimeStamp(root->itemGetOrCreate("LastError/Timestamp")),
	mServiceCount(root->itemGetOrCreate("Services/Count"))
{
	QsLogging::Logger &logger = QsLogging::Logger::instance();
	QsLogging::DestinationPtr dest(new DiagnosticsDestination(this));
	logger.addDestination(dest);
	mServiceCount->produceValue(0);
	mServiceCount->produceText("0");
	mRoot->produceValue(QVariant(), VeQItem::Synchronized);

	mLastErrorTimer->setInterval(1000);
	mLastErrorTimer->setSingleShot(true);

	connect(services, SIGNAL(dbusServiceFound(DBusService *)),
			this, SLOT(onServiceFound(DBusService *)));
	connect(mLastErrorTimer, SIGNAL(timeout()), this, SLOT(onLastErrorTimer()));
}

void DiagnosticsService::setError(const QString &error)
{
	mLastErrorText = error;
	mLastErrorTime = QDateTime::currentDateTimeUtc();
	if (!mLastErrorTimer->isActive())
		mLastErrorTimer->start();
}

void DiagnosticsService::onServiceFound(DBusService *service)
{
	connect(service->getDeviceInstance(), SIGNAL(valueChanged(QVariant)),
			this, SLOT(onDeviceInstanceChanged()));
	connect(service->getServiceRoot(), SIGNAL(stateChanged(VeQItem::State)),
			this, SLOT(onServiceStateChanged()));
}

void DiagnosticsService::onDeviceInstanceChanged()
{
	VeQItem *deviceInstanceItem = static_cast<VeQItem *>(sender());
	VeQItem *serviceRoot = deviceInstanceItem->itemParent();
	VeQItem *serviceEntry = getServiceItem(serviceRoot, deviceInstanceItem);
	if (serviceEntry == 0)
		serviceEntry = createServiceItem(deviceInstanceItem);
	if (serviceEntry == 0)
		return;
	updateService(serviceEntry, serviceRoot);
}

void DiagnosticsService::onServiceStateChanged()
{
	VeQItem *item = static_cast<VeQItem *>(sender());
	VeQItem *serviceEntry = 0;
	if (item->getState() == VeQItem::Offline) {
		serviceEntry = getServiceItem(item);
	} else {
		VeQItem *deviceInstanceItem = item->itemGetOrCreate("DeviceInstance");
		serviceEntry = getServiceItem(item, deviceInstanceItem);
		if (serviceEntry == 0)
			serviceEntry = createServiceItem(deviceInstanceItem);
	}
	if (serviceEntry == 0)
		return;
	updateService(serviceEntry, item);
}

void DiagnosticsService::onLastErrorTimer()
{
	mLastError->produceValue(mLastErrorText);
	mLastError->produceText(mLastErrorText);

	mLastErrorTimeStamp->produceValue(mLastErrorTime.toTime_t());
	mLastErrorTimeStamp->produceText(mLastErrorTime.toLocalTime().toString());
}

VeQItem *DiagnosticsService::getServiceItem(VeQItem *serviceRoot, VeQItem *deviceInstance)
{
	QVariant v = deviceInstance->getValue();
	if (!v.isValid())
		return 0;
	int unitId = mMappings->getUnitId(v.toInt());
	QString id = serviceRoot->id();

	// EV Check for a service with the same type and the same device instance.
	// If present, replace that service.
	QString serviceType = DBusService::getDeviceType(id);
	int serviceCount = mServiceCount->getValue().toInt();
	for (int i=0; i<serviceCount; ++i) {
		VeQItem *serviceEntry = mRoot->itemGetOrCreate(QString("Services/%1").arg(i), false);
		VeQItem *serviceName = serviceEntry->itemGetOrCreate("ServiceName");
		QString st = DBusService::getDeviceType(serviceName->getValue().toString());
		if (st == serviceType) {
			VeQItem *di = serviceEntry->itemGetOrCreate("UnitId");
			if (di->getValue().toInt() == unitId)
				return serviceEntry;
		}
	}

	return 0;
}

VeQItem *DiagnosticsService::getServiceItem(VeQItem *serviceRoot)
{
	QString serviceName = serviceRoot->id();
	int serviceCount = mServiceCount->getValue().toInt();
	for (int i=0; i<serviceCount; ++i) {
		VeQItem *se = mRoot->itemGetOrCreate(QString("Services/%1").arg(i), false);
		VeQItem *sn = se->itemGetOrCreate("ServiceName");
		if (serviceName == sn->getValue().toString())
			return se;
	}
	return 0;
}

VeQItem *DiagnosticsService::createServiceItem(VeQItem *deviceInstance)
{
	QVariant v = deviceInstance->getValue();
	if (!v.isValid())
		return 0;
	int unitId = mMappings->getUnitId(v.toInt());

	int serviceCount = mServiceCount->getValue().toInt();
	VeQItem *serviceEntry = mRoot->itemGetOrCreate(QString("Services/%1").arg(serviceCount), false);

	VeQItem *unitIdItem = serviceEntry->itemGetOrCreate("UnitId");
	unitIdItem->produceValue(unitId);
	unitIdItem->produceText(QString::number(unitId));

	mServiceCount->setValue(serviceCount + 1);

	return serviceEntry;
}

void DiagnosticsService::updateService(VeQItem *serviceEntry, VeQItem *serviceRoot)
{
	QString id = serviceRoot->id();
	VeQItem *serviceName = serviceEntry->itemGetOrCreate("ServiceName");
	serviceName->produceValue(id);
	serviceName->produceText(id);

	VeQItem *isActive = serviceEntry->itemGetOrCreate("IsActive");
	int a = serviceRoot->getState() == VeQItem::Offline ? 0 : 1;
	isActive->produceValue(a);
	isActive->produceText(QString::number(a));
}
