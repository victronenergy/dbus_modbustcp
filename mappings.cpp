#include "mappings.h"

//#define QS_LOG_DISABLE
#include "QsLog.h"

const QString attributesFile = "/data/modbustcp/attributes.csv";
const QString unitIDFile = "/data/modbustcp/unitid2di.csv";

Mappings::Mappings(DBusServices *services, QObject *parent) :
	QObject(parent),
	mServices(services)
{
	importCSV(attributesFile);
	importUnitIDMapping(unitIDFile);
	connect(services, SIGNAL(dbusServiceFound(DBusService *)), SLOT(dbusServiceFound(DBusService *)));
}

void Mappings::dbusServiceFound(DBusService * service)
{
	QLOG_TRACE() << "[Mappings] service found " << service->getServiceName();
	DBusService::DbusServiceType type = DBusService::getDeviceType(service->getServiceName());
	foreach (const DBusModbusData * item, mDBusModbusMap ) {
		if (item->deviceType == type)
			service->registerObject(item->objectPath);
	}
}

bool Mappings::getValue(const int modbusAddress, const int unitID, quint16 &value)
{
	if (mDBusModbusMap.contains(modbusAddress) && mUnitIDMap.contains(unitID)) {
		DBusModbusData * itemProperties = mDBusModbusMap.value(modbusAddress);
		DBusService * service = mServices->getService(itemProperties->deviceType, mUnitIDMap[unitID]);
		if (service) {
			QVariant dbusValue = service->getValue(itemProperties->objectPath);
			if (dbusValue.isValid()) {
				switch (itemProperties->valueType) {
				case mb_type_int16:
					value = convertInt16(dbusValue, itemProperties->scaleFactor);
					return true;
				case mb_type_uint16:
					value = convertUInt16(dbusValue, itemProperties->scaleFactor);
					return true;
				default:
					break;
				}
			}
			QLOG_DEBUG() << "[Mappings] getValue (" << service->getServiceName() << "," << itemProperties->objectPath << ") got" << dbusValue.toString() << "converted to" << value;
		}else
			QLOG_WARN() << "[Mappings] service not found for modbus address " << modbusAddress << " and unit ID " << unitID;
	}
	return false; //retVal = true;
}

void Mappings::getValues(const int modbusAddress, const int unitID, const int quantity, QByteArray &replyData)
{
	quint16 value;
	int j = 0;
	for (int i = 0; i < quantity; i++) {
		if (getValue(modbusAddress+i, unitID, value)) {
			replyData[j++] = (quint8)(value >> 8);
			replyData[j++] = (quint8)value;
		} else {
			replyData.clear();
			return;
		}
	}
	QLOG_TRACE() << "[Mappings] reply data" << replyData.toHex().toUpper();
}

quint16 Mappings::convertInt16(const QVariant &value, const float scaleFactor)
{
	QLOG_TRACE() << "[Mappings] convert to int16: value = " << value.toString() << " scale factor = " << scaleFactor;
	switch (value.type()) {
	case QVariant::Double:
		return static_cast<qint16>(round(value.toDouble() * scaleFactor));
	case QVariant::Int:
		return static_cast<qint16>(round(value.toInt() * scaleFactor));
	case QVariant::UInt:
		return static_cast<qint16>(round(value.toUInt() * scaleFactor));
	case QVariant::Bool:
		return static_cast<qint16>(value.toBool());
		break;
	default:
		return 0;
	}
	return 0;
}

quint16 Mappings::convertUInt16(const QVariant &value, const float scaleFactor)
{
	QLOG_TRACE() << "[Mappings] convert to uint16: value = " << value.toString() << " scale factor = " << scaleFactor;
	switch (value.type()) {
	case QVariant::Double:
		return static_cast<quint16>(round(value.toDouble() * scaleFactor));
	case QVariant::Int:
		return static_cast<quint16>(round(value.toInt() * scaleFactor));
	case QVariant::UInt:
		return static_cast<quint16>(round(value.toUInt() * scaleFactor));
	case QVariant::Bool:
		return static_cast<quint16>(value.toBool());
		break;
	default:
		return 0;
	}
	return 0;
}

Mappings::ModbusValueTypes Mappings::convertType(const QString &typeString)
{
	if (typeString == "int16")
		return mb_type_int16;
	if (typeString == "uint16")
		return mb_type_uint16;
	return mb_type_none;
}

void Mappings::importCSV(const QString &filename)
{
	QString string;
	QFile file(filename);

	if (file.open(QIODevice::ReadOnly)) {
		QTextStream in(&file);
		while ( !in.atEnd() ) {
			DBusModbusData * item = new DBusModbusData();
			QString line = in.readLine();
			QStringList values = line.split(",");
			if (values.size() >= 4) {
				item->deviceType = DBusService::getDeviceType(values.at(0));
				item->objectPath = values.at(1);
				item->valueType = convertType(values.at(4));
				if (item->valueType != mb_type_none) {
					item->scaleFactor = values.at(5).toDouble();
					mDBusModbusMap.insert(values.at(3).toInt(), item);
					QLOG_INFO() << "[Mappings] Add" << values;
				}
			}
		}
		file.close();
	}
	return;
}

void Mappings::importUnitIDMapping(const QString &filename)
{
	QString string;
	QFile file(filename);

	if (file.open(QIODevice::ReadOnly)) {
		QTextStream in(&file);
		while ( !in.atEnd() ) {
			QString line = in.readLine();
			QStringList values = line.split(",");
			if (values.size() >= 2) {
				bool isNumber;
				const int unitID = values.at(0).toInt(&isNumber);
				if (isNumber) {
					const int deviceInstance = values.at(1).toInt(&isNumber);
					if (isNumber) {
						mUnitIDMap.insert(unitID, deviceInstance);
						QLOG_INFO() << "[Mappings] Add" << values;
					}
				}
			}
		}
		file.close();
	}
	return;
}

