#ifndef DIAGNOSTICS_SERVICE_TEST_H
#define DIAGNOSTICS_SERVICE_TEST_H

#include <dbus_services.h>
#include <diagnostics_service.h>
#include <mappings.h>
#include <QObject>
#include <QScopedPointer>
#include <QTest>
#include <veutil/qt/ve_qitem.hpp>

class DiagnosticsServiceTest : public QObject
{
	Q_OBJECT

private slots:
	void init()
	{
		// qRegisterMetaType<MappingRequest *>();
		mItemProducer.reset(new VeQItemProducer(VeQItems::getRoot(), "test"));
		mServices.reset(new DBusServices(mItemProducer->services()));
		mMappings.reset(new Mappings(mServices.data()));
		QTextStream unitIdMapping(QByteArray(
			"Unit ID, /DeviceInstance,Remark\n"
			"246,257,CCGX VE.Bus port (ttyO1)\n"
			"0,0,VE.Can device instance 0 and system service\n"
			"1,1,VE.Can device instance 1\n"
			"100,0,Workaround for clients that do not support unit ID 0\n"
			"239,288,VE.Direct via USB (ttyUSB0)\n"));
		mMappings->importUnitIDMapping(unitIdMapping);
		mDiagnosticsRoot =
			mItemProducer->services()->itemGetOrCreate("com.victronenergy.modbustcp");
		mDiagnosticsService.reset(
			new DiagnosticsService(mServices.data(), mMappings.data(), mDiagnosticsRoot));
		mServices->initialScan();
	}

	void cleanup()
	{
		mDiagnosticsService.reset();
		mMappings.reset();
		mServices.reset();
		mItemProducer.reset();
	}

	void createService()
	{
		VeQItem *service = createService("com.victronenergy.grid.ttyUSB0", 30);
		QCOMPARE(getServiceName(0), service->id());
		QCOMPARE(getUnitId(0), 30);
		QCOMPARE(isActive(0), 1);
		QCOMPARE(getServiceEntryCount(), 1);
	}

	void createMultipleServices()
	{
		VeQItem *service = createService("com.victronenergy.grid.ttyUSB0", 30);
		VeQItem *service2 = createService("com.victronenergy.system", 0);

		QCOMPARE(getServiceName(0), service->id());
		QCOMPARE(getUnitId(0), 30);
		QCOMPARE(isActive(0), 1);

		QCOMPARE(getServiceName(1), service2->id());
		QCOMPARE(getUnitId(1), 100);
		QCOMPARE(isActive(1), 1);

		QCOMPARE(getServiceEntryCount(), 2);
	}

	void unitIdMapping()
	{
		VeQItem *service = createService("com.victronenergy.solarcharger.ttyO1", 257);
		QCOMPARE(getServiceName(0), service->id());
		QCOMPARE(getUnitId(0), 246);
		QCOMPARE(isActive(0), 1);
		QCOMPARE(getServiceEntryCount(), 1);
	}

	void removeService()
	{
		VeQItem *service = createService("com.victronenergy.solarcharger.ttyO1", 257);
		service->produceValue(QVariant(), VeQItem::Offline);
		QCOMPARE(getServiceName(0), service->id());
		QCOMPARE(getUnitId(0), 246);
		QCOMPARE(isActive(0), 0);
		QCOMPARE(getServiceEntryCount(), 1);
	}

	void replaceService()
	{
		VeQItem *service = createService("com.victronenergy.grid.ttyUSB0", 30);
		QCOMPARE(isActive(0), 1);
		service->produceValue(QVariant(), VeQItem::Offline);
		QCOMPARE(isActive(0), 0);
		VeQItem *service2 = createService("com.victronenergy.grid.ttyUSB1", 30);
		QCOMPARE(getServiceName(0), service2->id());
		QCOMPARE(isActive(0), 1);
		service2->produceValue(QVariant(), VeQItem::Offline);
		QCOMPARE(getServiceName(0), service2->id());
		QCOMPARE(isActive(0), 0);
		service->produceValue(QVariant(), VeQItem::Synchronized);
		QCOMPARE(getServiceName(0), service->id());
		QCOMPARE(isActive(0), 1);
	}

	void slowCreation()
	{
		VeQItem *service = mItemProducer->services()->itemGetOrCreate("com.victronenergy.grid.ttyUSB0", false);
		QCOMPARE(getUnitId(0), -1);
		QCOMPARE(getServiceEntryCount(), 0);
		VeQItem *deviceInstance = service->itemGetOrCreate("DeviceInstance");
		deviceInstance->produceValue(39);
		QCOMPARE(getServiceName(0), service->id());
		QCOMPARE(getUnitId(0), 39);
		QCOMPARE(isActive(0), 1);
		QCOMPARE(getServiceEntryCount(), 1);
	}

	void removeDeviceInstanceFirst()
	{
		VeQItem *service = mItemProducer->services()->itemGetOrCreate("com.victronenergy.grid.ttyUSB0", false);
		VeQItem *deviceInstance = service->itemGetOrCreate("DeviceInstance");
		deviceInstance->produceValue(39);
		QCOMPARE(getServiceName(0), service->id());
		QCOMPARE(isActive(0), 1);
		deviceInstance->produceValue(QVariant());
		service->produceValue(QVariant(), VeQItem::Offline);
		QCOMPARE(getServiceName(0), service->id());
		QCOMPARE(isActive(0), 0);
	}

private:
	VeQItem *createService(const QString &name, int deviceInstance)
	{
		VeQItem *di =
			mItemProducer->services()->itemGetOrCreate(QString("%1/DeviceInstance").arg(name));
		di->produceValue(deviceInstance);
		VeQItem *service = di->itemParent();
		service->produceValue(QVariant(), VeQItem::Synchronized);
		return service;
	}

	VeQItem *getServiceEntry(int index)
	{
		return mDiagnosticsRoot->itemGet(QString("Services/%1").arg(index));
	}

	QString getServiceName(int index)
	{
		VeQItem *service = getServiceEntry(index);
		if (service == 0)
			return QString();
		VeQItem *serviceName = service->itemGet("ServiceName");
		if (serviceName == 0)
			return QString();
		return serviceName->getValue().toString();
	}

	int getUnitId(int index)
	{
		VeQItem *service = getServiceEntry(index);
		if (service == 0)
			return -1;
		VeQItem *unitId = service->itemGet("UnitId");
		if (unitId == 0)
			return -1;
		return unitId->getValue().toInt();
	}

	int isActive(int index)
	{
		VeQItem *service = getServiceEntry(index);
		if (service == 0)
			return -1;
		VeQItem *isActive = service->itemGet("IsActive");
		if (isActive == 0)
			return -1;
		return isActive->getValue().toInt();
	}

	int getServiceEntryCount()
	{
		VeQItem *count = mDiagnosticsRoot->itemGet("Services/Count");
		if (count == 0)
			return -1;
		return count->getValue().toInt();
	}

	QScopedPointer<VeQItemProducer> mItemProducer;
	QScopedPointer<DBusServices> mServices;
	QScopedPointer<Mappings> mMappings;
	QScopedPointer<DiagnosticsService> mDiagnosticsService;
	VeQItem *mDiagnosticsRoot;
};

#endif // DIAGNOSTICS_SERVICE_TEST_H
