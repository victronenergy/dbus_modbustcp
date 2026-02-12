#ifndef MAPPING_TEST_H
#define MAPPING_TEST_H

#include <dbus_services.h>
#include <mappings.h>
#include <mapping_request.h>
#include <veutil/qt/ve_qitem.hpp>
#include <QObject>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QTest>

class DBusServices;
class Mappings;
class VeQItemProducer;

Q_DECLARE_METATYPE(MappingRequest *)

class MappingTest : public QObject
{
	Q_OBJECT

private slots:
	void init()
	{
		qRegisterMetaType<MappingRequest *>();
		mItemProducer.reset(new VeQItemProducer(VeQItems::getRoot(), "test"));
		mServices.reset(new DBusServices(mItemProducer->services()));
		mMappings.reset(new Mappings(mServices.data()));
		QTextStream attributes(QByteArray(
			"com.victronenergy.vebus,/Ac/Out/L1/V,d,V AC,15,uint16,10,R\n"
			"com.victronenergy.vebus,/Ac/ActiveIn/CurrentLimit,d,A,22,int16,10,W\n"
			"com.victronenergy.system,/Serial,s,,800,string[6],1,R\n"
			"com.victronenergy.system,/CustomName,s,,810,string[6],1,W\n"
			"com.victronenergy.pvinverter,/Ac/L1/Power,i,W,1029,uint16,1,R\n"
			"com.victronenergy.gps,/Position/Longitude,d,Decimal degrees,2802,int32,10000000,R\n"
			"com.victronenergy.gps,/Position/Latitude,d,Decimal degrees,2800,int32,10000000,R\n"
			"com.victronenergy.settings,/Settings/CGwacs/AcPowerSetPoint,d,W,2700,int16,1,W\n"
			"com.victronenergy.settings,/Settings/CGwacs/AcPowerSetPoint,d,W,2703,int32,100,W\n"
			"com.victronenergy.settings,/Settings/CGwacs/MaxChargePercentage,d,%,2701,uint16,1,W\n"
			"com.victronenergy.settings,/Settings/CGwacs/MaxDischargePercentage,d,%,2702,uint16,1,W\n"));
		mMappings->importCSV(attributes);
		QTextStream unitIdMapping(QByteArray(
			"Unit ID, /DeviceInstance,Remark\n"
			"246,257,CCGX VE.Bus port (ttyO1)\n"
			"0,0,VE.Can device instance 0 and system service\n"
			"1,1,VE.Can device instance 1\n"
			"100,0,Workaround for clients that do not support unit ID 0\n"
			"239,288,VE.Direct via USB (ttyUSB0)\n"));
		mMappings->importUnitIDMapping(unitIdMapping);
	}

	void cleanup()
	{
		mMappings.reset();
		mServices.reset();
		mItemProducer.reset();
	}

	void readSingleRegisterTest()
	{
		VeQItem *hub4 = createService("com.victronenergy.settings", 0);
		VeQItem *hub4GridSetpoint = hub4->itemGetOrCreate("/Settings/CGwacs/AcPowerSetPoint");
		hub4GridSetpoint->setValue(32123);

		mServices->initialScan();

		MappingRequest request(ReadValues, 2700, 0, 1);
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 2);
		QCOMPARE(getInt16Value(request.data(), 0), 32123);
	}

	void readSingleRegisterUnitId100Test()
	{
		VeQItem *hub4 = createService("com.victronenergy.settings", 0);
		VeQItem *hub4GridSetpoint = hub4->itemGetOrCreate("/Settings/CGwacs/AcPowerSetPoint");
		hub4GridSetpoint->setValue(32123);

		mServices->initialScan();

		// Unit ID 100 is an alternative for devices with instance 0. Not all Modbus TCP
		// implementations support unit ID 0.
		MappingRequest request(ReadValues, 2700, 100, 1);
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 2);
		QCOMPARE(getInt16Value(request.data(), 0), 32123);
	}

	void readInvalidSingleRegisterTest()
	{
		VeQItem *hub4 = createService("com.victronenergy.settings", 0);
		VeQItem *hub4GridSetpoint = hub4->itemGetOrCreate("/Settings/CGwacs/AcPowerSetPoint");
		hub4GridSetpoint->setValue(32123);

		mServices->initialScan();

		// Register does not exist
		MappingRequest request(ReadValues, 2639, 0, 1);
		handleRequest(&request);

		QCOMPARE(request.error(), StartAddressError);
		QVERIFY(!request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 0);
	}

	void zeroQuantityTest()
	{
		VeQItem *hub4 = createService("com.victronenergy.settings", 0);
		VeQItem *hub4GridSetpoint = hub4->itemGetOrCreate("/Settings/CGwacs/AcPowerSetPoint");
		hub4GridSetpoint->setValue(32123);

		mServices->initialScan();

		// Register read zero bytes, should return an error
		MappingRequest request(ReadValues, 2700, 0, 0);
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 0);
	}

	void largeQuantityTest()
	{
		VeQItem *hub4 = createService("com.victronenergy.settings", 0);
		VeQItem *hub4GridSetpoint = hub4->itemGetOrCreate("/Settings/CGwacs/AcPowerSetPoint");
		hub4GridSetpoint->setValue(32123);

		mServices->initialScan();

		// Register read zero bytes, should return an error
		MappingRequest request(ReadValues, 2700, 0, 0xFFFF);
		handleRequest(&request);

		QCOMPARE(request.error(), AddressError);
		QVERIFY(!request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 0);
	}

	void readSingleRegisterUnDocumentedUnitIdTest()
	{
		VeQItem *pvinverter = createService("com.victronenergy.pvinverter", 213);
		VeQItem *acL1Power = pvinverter->itemGetOrCreate("/Ac/L1/Power");
		acL1Power->setValue(517);

		mServices->initialScan();

		// Unit ID 213 is not documented in the unit ID list, so the device instance should be
		// used as unit ID.
		MappingRequest request(ReadValues, 1029, 213, 1);
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 2);
		QCOMPARE(getInt16Value(request.data(), 0), 517);
	}

	void readSingleRegisterNonExistingUnitIdTest()
	{
		VeQItem *pvinverter = createService("com.victronenergy.pvinverter", 20);
		VeQItem *acL1Power = pvinverter->itemGetOrCreate("/Ac/L1/Power");
		acL1Power->setValue(517);

		mServices->initialScan();

		// There is no device with unit ID 213, so we expect an error
		MappingRequest request(ReadValues, 1029, 213, 1);
		handleRequest(&request);

		QCOMPARE(request.error(), ServiceError);
		QVERIFY(!request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 0);
	}

	void readMultipleRegistersTest()
	{
		VeQItem *hub4 = createService("com.victronenergy.settings", 0);
		VeQItem *hub4GridSetpoint = hub4->itemGetOrCreate("/Settings/CGwacs/AcPowerSetPoint");
		hub4GridSetpoint->setValue(5014);
		VeQItem *maxChargePercentage = hub4->itemGetOrCreate("/Settings/CGwacs/MaxChargePercentage");
		maxChargePercentage->setValue(100);
		VeQItem *maxDischargePercentage = hub4->itemGetOrCreate("/Settings/CGwacs/MaxDischargePercentage");
		maxDischargePercentage->setValue(34.5);

		mServices->initialScan();

		MappingRequest request(ReadValues, 2700, 0, 4);
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 8);
		QCOMPARE(getInt16Value(request.data(), 0), 5014);
		QCOMPARE(getInt16Value(request.data(), 1), 100);
		QCOMPARE(getInt16Value(request.data(), 2), 35);
	}

	void readMultipleRegistersInvalidTest()
	{
		VeQItem *hub4 = createService("com.victronenergy.settings", 0);
		VeQItem *hub4GridSetpoint = hub4->itemGetOrCreate("/Settings/CGwacs/AcPowerSetPoint");
		hub4GridSetpoint->setValue(5014);
		VeQItem *maxChargePercentage = hub4->itemGetOrCreate("/Settings/CGwacs/MaxChargePercentage");
		maxChargePercentage->setValue(100);
		VeQItem *maxDischargePercentage = hub4->itemGetOrCreate("/Settings/CGwacs/MaxDischargePercentage");
		maxDischargePercentage->setValue(34.5);

		mServices->initialScan();

		MappingRequest request(ReadValues, 2700, 0, 30);
		handleRequest(&request);

		QCOMPARE(request.error(), AddressError);
		QVERIFY(!request.errorString().isEmpty());
		QVERIFY(request.data().isEmpty());
	}

	void readInt32Test()
	{
		VeQItem *gps = createService("com.victronenergy.gps", 0);
		VeQItem *latitude = gps->itemGetOrCreate("/Position/Latitude");
		latitude->setValue(83.123456789);

		mServices->initialScan();

		MappingRequest request(ReadValues, 2800, 0, 2);
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 4);
		QCOMPARE(getInt32Value(request.data(), 0), 831234568);
	}

	void readPartialTest()
	{
		VeQItem *gps = createService("com.victronenergy.gps", 0);
		VeQItem *latitude = gps->itemGetOrCreate("/Position/Latitude");
		VeQItem *longitude = gps->itemGetOrCreate("/Position/Longitude");

		latitude->setValue(47.987654321);
		longitude->setValue(83.123456789);

		mServices->initialScan();

		MappingRequest request(ReadValues, 2801, 0, 2);
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 4);
		QCOMPARE(getInt16Value(request.data(), 0), 479876543 & 0x0000FFFF);
		QCOMPARE(getInt16Value(request.data(), 1), 831234568 >> 16);
	}

	void readString()
	{
		VeQItem *system = createService("com.victronenergy.system", 0);
		VeQItem *serial = system->itemGetOrCreate("/Serial");
		serial->setValue("a1b2c3e4f5a6");

		MappingRequest request(ReadValues, 800, 100, 6);
		mServices->initialScan();
		handleRequest(&request);

		QCOMPARE(request.data().size(), 12);
		QCOMPARE(getRegisterString(request.data(), 0, 6), QString("a1b2c3e4f5a6"));
	}

	void readShortString()
	{
		VeQItem *system = createService("com.victronenergy.system", 0);
		VeQItem *serial = system->itemGetOrCreate("/Serial");
		serial->setValue("a1b2c3e4");

		MappingRequest request(ReadValues, 800, 100, 6);
		mServices->initialScan();
		handleRequest(&request);

		QCOMPARE(request.data().size(), 12);
		QCOMPARE(getRegisterString(request.data(), 0, 6), QString("a1b2c3e4"));
	}

	void readPartialString()
	{
		VeQItem *system = createService("com.victronenergy.system", 0);
		VeQItem *serial = system->itemGetOrCreate("/Serial");
		serial->setValue("a1b2c3e4f5a6");

		MappingRequest request(ReadValues, 802, 100, 2);
		mServices->initialScan();
		handleRequest(&request);

		QCOMPARE(request.data().size(), 4);
		QCOMPARE(getRegisterString(request.data(), 0, 2), QString("c3e4"));
	}

	void writeSingleRegisterTest()
	{
		VeQItem *vebus = createService("com.victronenergy.vebus.ttyO1", 257);
		VeQItem *currentLimit = vebus->itemGetOrCreate("/Ac/ActiveIn/CurrentLimit");
		currentLimit->setValue(75.4);

		mServices->initialScan();

		MappingRequest request(WriteValues, 22, 246, 1);
		request.data().append(0x02);
		request.data().append(0x43);
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 2);
		QCOMPARE(getInt16Value(request.data(), 0), 0x0243);
		QCOMPARE(currentLimit->getValue().toDouble(), 57.9);
	}

	void writeUnwritableRegisterTest()
	{
		VeQItem *vebus = createService("com.victronenergy.vebus.ttyO1", 257);
		VeQItem *currentLimit = vebus->itemGetOrCreate("/Ac/Out/L1/V");
		currentLimit->setValue(75.4);

		mServices->initialScan();

		// We are not allowed to write to this reqister, expect an error.
		MappingRequest request(WriteValues, 15, 246, 1);
		request.data().append(0x02);
		request.data().append(0x43);
		handleRequest(&request);

		QCOMPARE(request.error(), PermissionError);
		QVERIFY(!request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 2);
		QCOMPARE(getInt16Value(request.data(), 0), 0x0243);
		QCOMPARE(currentLimit->getValue().toDouble(), 75.4);
	}

	void writeMultipleRegisterTest()
	{
		VeQItem *hub4 = createService("com.victronenergy.settings", 0);
		VeQItem *hub4GridSetpoint = hub4->itemGetOrCreate("/Settings/CGwacs/AcPowerSetPoint");
		hub4GridSetpoint->setValue(5014);
		VeQItem *maxChargePercentage = hub4->itemGetOrCreate("/Settings/CGwacs/MaxChargePercentage");
		maxChargePercentage->setValue(100);
		VeQItem *maxDischargePercentage = hub4->itemGetOrCreate("/Settings/CGwacs/MaxDischargePercentage");
		maxDischargePercentage->setValue(34.5);

		mServices->initialScan();

		// Write all 3 value defined above in one request.
		MappingRequest request(WriteValues, 2700, 100, 3);
		QByteArray &data = request.data();
		data.append(static_cast<char>(0x12));
		data.append(static_cast<char>(0xD7));
		data.append(static_cast<char>(0x00));
		data.append(static_cast<char>(0x43));
		data.append(static_cast<char>(0x00));
		data.append(static_cast<char>(0x12));
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 6);
		QCOMPARE(hub4GridSetpoint->getValue().toInt(), 4823);
		QCOMPARE(maxChargePercentage->getValue().toInt(), 67);
		QCOMPARE(maxDischargePercentage->getValue().toInt(), 18);
	}

	void write32bitRegister()
	{
		VeQItem *hub4 = createService("com.victronenergy.settings", 0);
		VeQItem *hub4GridSetpoint = hub4->itemGetOrCreate("/Settings/CGwacs/AcPowerSetPoint");
		hub4GridSetpoint->setValue(5014);

		mServices->initialScan();

		MappingRequest request(WriteValues, 2703, 100, 2);
		QByteArray &data = request.data();
		data.append(static_cast<char>(0x12));
		data.append(static_cast<char>(0xD7));
		data.append(static_cast<char>(0x00));
		data.append(static_cast<char>(0x43));
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 4);
		QCOMPARE(hub4GridSetpoint->getValue().toDouble(), 3160801.95);
	}

	void write32bitRegisterPartial1()
	{
		VeQItem *hub4 = createService("com.victronenergy.settings", 0);
		VeQItem *maxDischargePercentage = hub4->itemGetOrCreate("/Settings/CGwacs/MaxDischargePercentage");
		maxDischargePercentage->setValue(34.5);
		VeQItem *hub4GridSetpoint = hub4->itemGetOrCreate("/Settings/CGwacs/AcPowerSetPoint");
		hub4GridSetpoint->setValue(5014);

		mServices->initialScan();

		MappingRequest request(WriteValues, 2702, 100, 2);
		QByteArray &data = request.data();
		data.append(static_cast<char>(0x00));
		data.append(static_cast<char>(0x63));
		data.append(static_cast<char>(0x12));
		data.append(static_cast<char>(0xD7));
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 4);
		QCOMPARE(hub4GridSetpoint->getValue().toDouble(), 3161227.76);
		QCOMPARE(maxDischargePercentage->getValue().toDouble(), 99.0);
	}

	void write32bitRegisterPartial2()
	{
		VeQItem *hub4 = createService("com.victronenergy.settings", 0);
		VeQItem *hub4GridSetpoint = hub4->itemGetOrCreate("/Settings/CGwacs/AcPowerSetPoint");
		hub4GridSetpoint->setValue(5014);

		mServices->initialScan();

		MappingRequest request(WriteValues, 2704, 100, 1);
		QByteArray &data = request.data();
		data.append(static_cast<char>(0x00));
		data.append(static_cast<char>(0x43));
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 2);
		QCOMPARE(hub4GridSetpoint->getValue().toDouble(), 4588.19);
	}

	void writeMultipleRegisterTestInvalid()
	{
		VeQItem *hub4 = createService("com.victronenergy.settings", 0);
		VeQItem *hub4GridSetpoint = hub4->itemGetOrCreate("/Settings/CGwacs/AcPowerSetPoint");
		hub4GridSetpoint->setValue(5014);
		VeQItem *maxChargePercentage = hub4->itemGetOrCreate("/Settings/CGwacs/MaxChargePercentage");
		maxChargePercentage->setValue(100);
		VeQItem *maxDischargePercentage = hub4->itemGetOrCreate("/Settings/CGwacs/MaxDischargePercentage");
		maxDischargePercentage->setValue(34.5);

		mServices->initialScan();

		// Send an extra write request, this should cause an error
		MappingRequest request(WriteValues, 2700, 100, 6);
		QByteArray &data = request.data();
		data.append(0x12);
		data.append(0x47);
		data.append(static_cast<char>(0x00));
		data.append(0x43);
		data.append(static_cast<char>(0x00));
		data.append(0x12);
		data.append(0x13);
		data.append(0x14);
		data.append(0x15);
		data.append(0x16);
		data.append(0x17);
		data.append(0x18);
		handleRequest(&request);

		QCOMPARE(request.error(), AddressError);
		QVERIFY(!request.errorString().isEmpty());
		QCOMPARE(request.data().size(), 12);
		QCOMPARE(hub4GridSetpoint->getValue().toInt(), 5014);
		QCOMPARE(maxChargePercentage->getValue().toInt(), 100);
		QCOMPARE(maxDischargePercentage->getValue().toInt(), 35);
	}

	void writeSingleRegisterTestTwice()
	{
		// During testing a problem occured we writing the same value to a register twice:
		// the second write would not cause a SetValue on the D-Bus because the VeQItem was
		// left in Storing state, where a check is performed on the value to be sent. If it is
		// equal to the previous value that has been set, it will not initiate a SetValue. The
		// Storing state will not be reset until a properties changed signal is received, which
		// does not happen if the value has not changed on the service.
		VeQItem *vebus = createService("com.victronenergy.vebus.ttyO1", 257);
		VeQItem *currentLimit = vebus->itemGetOrCreate("/Ac/ActiveIn/CurrentLimit");
		currentLimit->setValue(75.4);

		mServices->initialScan();

		MappingRequest request(WriteValues, 22, 246, 1);
		request.data().append(0x02);
		request.data().append(0x43);
		handleRequest(&request);

		QCOMPARE(currentLimit->getValue().toDouble(), 57.9);
		QCOMPARE(currentLimit->getState(), VeQItem::Synchronized);

		handleRequest(&request);
		QCOMPARE(currentLimit->getState(), VeQItem::Synchronized);

		MappingRequest request2(WriteValues, 22, 246, 1);
		request2.data().append(0x03);
		request2.data().append(0x43);
		handleRequest(&request2);

		QCOMPARE(currentLimit->getValue().toDouble(), 83.5);
		QCOMPARE(currentLimit->getState(), VeQItem::Synchronized);
	}

	void writeString()
	{
		VeQItem *system = createService("com.victronenergy.system", 0);
		VeQItem *customName = system->itemGetOrCreate("/CustomName");
		customName->setValue("OldValue1234");

		mServices->initialScan();

		// Write "Hello World!" (12 bytes = 6 registers)
		MappingRequest request(WriteValues, 810, 100, 6);
		QByteArray strBytes("Hello World!");
		request.data().append(strBytes);
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(customName->getValue().toString(), QString("Hello World!"));
	}

	void writePartialString()
	{
		VeQItem *system = createService("com.victronenergy.system", 0);
		VeQItem *customName = system->itemGetOrCreate("/CustomName");
		customName->setValue("abcdefghijkl");

		mServices->initialScan();

		// Write to registers 812-813 (offset 2 into the string), replacing bytes 4-7
		MappingRequest request(WriteValues, 812, 100, 2);
		request.data().append('X');
		request.data().append('Y');
		request.data().append('Z');
		request.data().append('W');
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(customName->getValue().toString(), QString("abcdXYZWijkl"));
	}

	void writeShortString()
	{
		VeQItem *system = createService("com.victronenergy.system", 0);
		VeQItem *customName = system->itemGetOrCreate("/CustomName");
		customName->setValue("OldValue1234");

		mServices->initialScan();

		// Write "Hi" followed by null bytes (full 6-register write)
		MappingRequest request(WriteValues, 810, 100, 6);
		request.data().append('H');
		request.data().append('i');
		for (int i = 0; i < 10; ++i)
			request.data().append(static_cast<char>(0));
		handleRequest(&request);

		QCOMPARE(request.error(), NoError);
		QVERIFY(request.errorString().isEmpty());
		QCOMPARE(customName->getValue().toString(), QString("Hi"));
	}

private:
	VeQItem *createService(const QString &service, int deviceInstance)
	{
		VeQItem *sv = mItemProducer->services()->itemGetOrCreate(service, false);
		VeQItem *di = sv->itemGetOrCreate("/DeviceInstance");
		di->setValue(deviceInstance);
		return  sv;
	}

	void handleRequest(MappingRequest *r)
	{
		QSignalSpy spy(mMappings.data(), SIGNAL(requestCompleted(MappingRequest *)));
		mMappings->handleRequest(r);
		QCOMPARE(spy.size(), 1);
	}

	static int getInt16Value(const QByteArray &data, int offset)
	{
		// Caution: elements in a QByteArray are (signed) chars, but they are to be interpreted
		// as (unsigned) bytes.
		return
			(static_cast<quint8>(data[2 * offset]) << 8) |
			static_cast<quint8>(data[2 * offset + 1]);
	}

	static int getInt32Value(const QByteArray &data, int offset)
	{
		return
			(static_cast<quint16>(getInt16Value(data, offset)) << 16) |
			static_cast<quint16>(getInt16Value(data, offset + 1));
	}

	QString getRegisterString(const QByteArray &data, int offset, int regCount)
	{
		QString s;
		for (int i=0; i<regCount;++i) {
			int v = getInt16Value(data, offset + i);
			char c0 = static_cast<char>(v >> 8);
			if (c0 == 0)
				return s;
			s += c0;
			char c1 = static_cast<char>(v & 0xFF);
			if (c1 == 0)
				return s;
			s += c1;
		}
		return s;
	}

	QScopedPointer<VeQItemProducer> mItemProducer;
	QScopedPointer<DBusServices> mServices;
	QScopedPointer<Mappings> mMappings;
};

#endif // MAPPING_TEST_H
