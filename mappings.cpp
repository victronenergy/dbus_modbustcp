#include "mappings.h"

//#define QS_LOG_DISABLE
#include "QsLog.h"

const QString attributesFile = "attributes.csv";
const QString unitIDFile = "unitid2di.csv";

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
	QString type = DBusService::getDeviceType(service->getServiceName());
	foreach (const DBusModbusData * item, mDBusModbusMap ) {
		if (item->deviceType == type)
			service->registerObject(item->objectPath);
	}
}

quint16 Mappings::getValue(const DBusService * service, const QString & objectPath, const ModbusTypes modbusType, const double scaleFactor) const
{
	QVariant dbusValue = service->getValue(objectPath);
	QLOG_TRACE() << "Get value from dbus object" << objectPath;
	if (dbusValue.isValid()) {
		switch (modbusType) {
		case mb_type_int16:
			return convertFromDbus<qint16>(dbusValue, scaleFactor);
		case mb_type_uint16:
			return convertFromDbus<quint16>(dbusValue, scaleFactor);
		default:
			break;
		}
	}
	return 0;
}

void Mappings::getValues(const int modbusAddress, const int unitID, const int quantity, QByteArray &replyData, Mappings::MappingErrors &error) const
{
	if (!mUnitIDMap.contains(unitID)) {
		error = UnitIdError;
		return;
	}
	if (!mDBusModbusMap.contains(modbusAddress)) {
		error = StartAddressError;
		return;
	}

	/*
	 * Get service from the first modbus address. The service must be the same for the complete address range
	 * therefore the service pointer has to be fetched and checked only once
	 */
	DBusModbusData * itemProperties = mDBusModbusMap.value(modbusAddress);
	DBusService * service = mServices->getService(itemProperties->deviceType, mUnitIDMap[unitID]);
	if (!service) {
		error = ServiceError;
		return;
	}

	error = NoError;
	quint16 value;
	int j = 0;
	for (int i = 0; i < quantity; i++) {
		if (mDBusModbusMap.contains(modbusAddress+i)) {
			itemProperties = mDBusModbusMap.value(modbusAddress+i);
			value = getValue(service, itemProperties->objectPath, itemProperties->modbusType, itemProperties->scaleFactor);
			replyData[j++] = (quint8)(value >> 8);
			replyData[j++] = (quint8)value;
		} else {
			error = AddressError;
			return;
		}
	}
}

bool Mappings::setValue(DBusService * const service, const QString & objectPath, const ModbusTypes modbusType, const QMetaType::Type dbusType, const double scaleFactor, const quint16 value)
{
	QVariant dbusValue;
	QLOG_TRACE() << "Set dbus object" << objectPath << "value to" << value;
	switch (modbusType) {
	case mb_type_int16:
	{
		dbusValue = convertToDbus(dbusType, static_cast<qint16>(value), scaleFactor);
		break;
	}
	case mb_type_uint16:
	{
		dbusValue = convertToDbus(dbusType, static_cast<quint16>(value), scaleFactor);
		break;
	}
	default:
		break;
	}
	if (dbusValue.isValid()) {
		if (service->setValue(objectPath, dbusValue))
			return true;
		else
			QLOG_ERROR() << "[Mappings] Set value failed.";
	}
	return false;
}

void Mappings::setValues(const int modbusAddress, const int unitID, const int quantity, QByteArray &data, Mappings::MappingErrors &error)
{
	if (!mUnitIDMap.contains(unitID)) {
		error = UnitIdError;
		return;
	}
	if (!mDBusModbusMap.contains(modbusAddress)) {
		error = StartAddressError;
		return;
	}

	/*
	 * Get service from the first modbus address. The service must be the same for the complete address range
	 * therefore the service pointer has to be fetched and checked only once
	 */
	DBusModbusData * itemProperties = mDBusModbusMap.value(modbusAddress);
	DBusService * service = mServices->getService(itemProperties->deviceType, mUnitIDMap[unitID]);
	if (!service) {
		error = ServiceError;
		return;
	}

	error = NoError;
	quint16 value;
	int j = 0;
	for (int i = 0; i < quantity; i++) {
		if (mDBusModbusMap.contains(modbusAddress+i)) {
			itemProperties = mDBusModbusMap.value(modbusAddress+i);
			if (itemProperties->accessRights==Mappings::mb_perm_write) {
				value = (data[j++] << 8);
				value |= (quint8)data[j++];
				if (!setValue(service, itemProperties->objectPath, itemProperties->modbusType, itemProperties->dbusType,itemProperties->scaleFactor, value)) {
					error = ServiceError;
				}
			} else {
				error = PermissionError;
				return;
			}
		} else {
			error = AddressError;
			return;
		}
	}
}

template<class rettype> rettype Mappings::convertFromDbus(const QVariant &value, const float scaleFactor) const
{
	const int variantType = value.userType();
	if (qMetaTypeId<QDBusArgument>() == variantType)
		return 0;

	const QMetaType::Type metaType = static_cast<QMetaType::Type>(variantType);
	switch (metaType) {
	case QMetaType::Float:
	case QMetaType::Double:
		return static_cast<rettype>(round(value.toDouble() * scaleFactor));
	case QMetaType::Char:
	case QMetaType::Short:
	case QMetaType::Int:
	case QMetaType::Long:
	case QMetaType::LongLong:
		return static_cast<rettype>(round(value.toInt() * scaleFactor));
	case QMetaType::UChar:
	case QMetaType::UShort:
	case QMetaType::UInt:
	case QMetaType::ULong:
	case QMetaType::ULongLong:
		return static_cast<rettype>(round(value.toUInt() * scaleFactor));
	case QMetaType::Bool:
		return static_cast<rettype>(value.toBool());
	default:
		QLOG_WARN() << "[Mappings] convert from dbus type tries to convert an unsupported type:" << value.type() << "(" << value.typeName() << ")";
		return 0;
	}
}

template<class argtype> QVariant Mappings::convertToDbus(const QMetaType::Type dbusType, const argtype value, const float scaleFactor) const
{
	switch (dbusType) {
	case QMetaType::Float:
	case QMetaType::Double:
		return QVariant::fromValue(static_cast<double>(value/scaleFactor));
	case QMetaType::Char:
	case QMetaType::Short:
	case QMetaType::Int:
	case QMetaType::Long:
	case QMetaType::LongLong:
		return QVariant::fromValue(static_cast<int>(round(value/scaleFactor)));
	case QMetaType::UChar:
	case QMetaType::UShort:
	case QMetaType::UInt:
	case QMetaType::ULong:
	case QMetaType::ULongLong:
		return QVariant::fromValue(static_cast<unsigned int>(round(value/scaleFactor)));
	case QMetaType::Bool:
		return QVariant::fromValue(static_cast<int>(value));
	default:
		QLOG_WARN() << "[Mappings] convert to dbus type tries to convert an unsupported type:" << dbusType;
	}
	return QVariant();
}

Mappings::ModbusTypes Mappings::convertModbusType(const QString &typeString)
{
	if (typeString == "int16")
		return mb_type_int16;
	if (typeString == "uint16")
		return mb_type_uint16;
	return mb_type_none;
}

QMetaType::Type Mappings::convertDbusType(const QString &typeString)
{
	if (typeString == "y")
		return QMetaType::UChar;
	else if (typeString == "b")
		return QMetaType::Bool;
	else if (typeString == "n")
		return QMetaType::Short;
	else if (typeString == "q")
		return QMetaType::UShort;
	else if (typeString == "i")
		return QMetaType::Int;
	else if (typeString == "u")
		return QMetaType::UInt;
	else if (typeString == "x")
		return QMetaType::Long;
	else if (typeString == "t")
		return QMetaType::ULong;
	else if (typeString == "d")
		return QMetaType::Double;
	return QMetaType::Void;
}

Mappings::Permissions Mappings::convertPermissions(const QString &permissions)
{
	if (permissions == "R")
		return Mappings::mb_perm_read;
	else if (permissions == "W")
		return Mappings::mb_perm_write;
	return Mappings::mb_perm_none;
}

void Mappings::importCSV(const QString &filename)
{
	QString string;
	QFile file(QCoreApplication::applicationDirPath() + "/" + filename);

	if (file.open(QIODevice::ReadOnly)) {
		QTextStream in(&file);
		while ( !in.atEnd() ) {
			DBusModbusData * item = new DBusModbusData();
			QString line = in.readLine();
			QStringList values = line.split(",");
			if (values.size() >= 8) {
				item->deviceType = DBusService::getDeviceType(values.at(0));
				item->objectPath = values.at(1);
				item->modbusType = convertModbusType(values.at(5));
				if (item->modbusType != mb_type_none) {
					item->scaleFactor = values.at(6).toDouble();
					if (item->scaleFactor == 0) item->scaleFactor = 1;
					item->dbusType = convertDbusType(values.at(2));
					item->accessRights = convertPermissions(values.at(7));
					mDBusModbusMap.insert(values.at(4).toInt(), item);
					QLOG_INFO() << "[Mappings] Add" << values;
				}
			}
		}
		file.close();
	} else
		QLOG_ERROR() << "Can not open file" << filename;
	return;
}

void Mappings::importUnitIDMapping(const QString &filename)
{
	QString string;
	QFile file(QCoreApplication::applicationDirPath() + "/" + filename);

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
	} else
		QLOG_ERROR() << "Can not open file" << filename;
	return;
}

