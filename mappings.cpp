#include <QCoreApplication>
#include <QFile>
#include <qmath.h>
#include <QStringList>
#include <QDateTime>
#include <veutil/qt/ve_qitem.hpp>
#include "dbus_service.h"
#include "dbus_services.h"
#include "mappings.h"
#include "mapping_request.h"
#include "QsLog.h"
#include "ve_qitem_init_monitor.h"

const QString stringType = "string";
const QString reservedType = "reserved";

Mappings::Mappings(DBusServices *services, QObject *parent) :
	QObject(parent),
	mServices(services)
{
}

Mappings::~Mappings()
{
	foreach(DBusModbusData *m, mDBusModbusMap)
		delete m;
}

void Mappings::importCSV(const QString &filename)
{
	QFile file(QCoreApplication::applicationDirPath() + "/" + filename);
	if (!file.open(QIODevice::ReadOnly)) {
		QLOG_ERROR() << "Can not open file" << filename;
		return;
	}
	QTextStream in(&file);
	importCSV(in);
}

void Mappings::importCSV(QTextStream &in)
{
	while (!in.atEnd()) {
		QString line = in.readLine();
		QStringList values = line.split(",");
		if (values.size() >= 8) {
			ModbusTypes modbusType = convertModbusType(values.at(5));
			if (modbusType != mb_type_none) {
				int item_size = 1;
				Operation *operation = &mNopOperation;
				switch (modbusType) {
				case mb_type_string:
					item_size = convertSize(stringType, values.at(5));
					break;
				case mb_type_reserved:
					item_size = convertSize(reservedType, values.at(5));
					operation = new ReservedOperation(item_size);
					break;
				case mb_type_int32:
				case mb_type_uint32:
					item_size = 2;
					break;
				case mb_type_uint64:
					item_size = 4;
					break;
				default:
					item_size = 1;
					break;
				}

				DBusModbusData * item = new DBusModbusData(
					DBusService::getDeviceType(values.at(0)),
					QStringList() << values.at(1), // objectPaths
					values.at(6).toDouble(), // scaleFactor
					item_size,
					modbusType,
					convertDbusType(values.at(2)),
					convertPermissions(values.at(7)),
					operation);

				if (item->dbusType == QMetaType::Void) {
					QLOG_WARN() << "[Mappings] Register" << values.at(4)
								<< ": register has no type";
				}

				int reg = values.at(4).toInt();
				if (mDBusModbusMap.find(reg) != mDBusModbusMap.end()) {
					QLOG_WARN() << "[Mappings] Register" << reg
								<< "reserved more than once. Check attributes file.";
				}
				mDBusModbusMap.insert(reg, item);
				QLOG_TRACE() << "[Mappings] Add" << values;
			}
		}
	}

	// Backwards compatibility registers
	mDBusModbusMap.insert(777, new DBusModbusData(
		"solarcharger",
		QStringList() << "/Yield/Power" << "/Pv/V", // objectPaths
		10, // scaleFactor
		1, // size
		mb_type_int16, // modbus type
		QMetaType::Double, // dbus type
		Mappings::mb_perm_read,
		&mDivOperation)); // Divide power by voltage

	// Date and time
	mDBusModbusMap.insert(830, new DBusModbusData(
		"system", QStringList(), 1, 4, mb_type_uint64, QMetaType::ULongLong,
		Mappings::mb_perm_read, &mTimeOperation));
}

void Mappings::importUnitIDMapping(const QString &filename)
{
	QFile file(QCoreApplication::applicationDirPath() + "/" + filename);
	if (!file.open(QIODevice::ReadOnly)) {
		QLOG_ERROR() << "Can not open file" << filename;
		return;
	}
	QTextStream in(&file);
	importUnitIDMapping(in);
}

void Mappings::importUnitIDMapping(QTextStream &in)
{
	while (!in.atEnd()) {
		QString line = in.readLine();
		QStringList values = line.split(",");
		if (values.size() >= 2) {
			bool isNumber;
			int unitID = values.at(0).toInt(&isNumber);
			if (isNumber) {
				int deviceInstance = values.at(1).toInt(&isNumber);
				// Only add unitID if it is not equal to the device instance, because if a unit ID
				// in a request cannot be found in mUnitIDMap, we will assume
				// unit ID == device instance. Adding these value pairs will only make mUnitIDMap
				// longer without changing the behavior of the application.
				if (isNumber && unitID != deviceInstance) {
					mUnitIDMap.insert(unitID, deviceInstance);
					QLOG_TRACE() << "[Mappings] Add" << values;
				}
			}
		}
	}
}

int Mappings::getUnitId(int deviceInstance) const
{
	for (QHash<int, int>::ConstIterator it = mUnitIDMap.begin(); it != mUnitIDMap.end(); ++it) {
		if (it.value() == deviceInstance)
			return it.key();
	}
	return deviceInstance;
}

void Mappings::handleRequest(MappingRequest *request)
{
	DataIterator it(this, request->address(), request->unitId(), request->quantity());
	QList<VeQItem *> pendingItems;
	for (;!it.atEnd(); it.next()) {
		if (request->type() == WriteValues && it.data()->accessRights != Mappings::mb_perm_write) {
			QString errorString = QString("Cannot write to register %1").arg(it.address());
			request->setError(PermissionError, errorString);
			emit requestCompleted(request);
			return;
		}
		pendingItems.append(it.items());
	}
	if (it.error() != NoError) {
		request->setError(it.error(), it.errorString());
		emit requestCompleted(request);
		return;
	}
	addPendingRequest(request, pendingItems);
}

void Mappings::onItemsInitialized()
{
	VeQItemInitMonitor *monitor = static_cast<VeQItemInitMonitor *>(sender());
	monitor->deleteLater();
	MappingRequest *request = mPendingRequests.value(monitor);
	mPendingRequests.remove(monitor);
	onItemsInitialized(request);
}

void Mappings::onItemsInitialized(MappingRequest *request)
{
	switch (request->type()) {
	case ReadValues:
		getValues(request);
		break;
	case WriteValues:
		setValues(request);
		break;
	default:
		Q_ASSERT(false);
		break;
	}
}

quint16 Mappings::getValue(const QVariant &dbusValue, ModbusTypes modbusType, int offset,
						   double scaleFactor)
{
	if (!dbusValue.isValid())
		return 0;
	switch (modbusType) {
	case mb_type_int16:
		return convertFromDbus<qint16>(dbusValue, scaleFactor);
	case mb_type_uint16:
		return convertFromDbus<quint16>(dbusValue, scaleFactor);
	case mb_type_int32:
	{
		quint32 v = convertFromDbus<qint32>(dbusValue, scaleFactor);
		return offset == 0 ? v >> 16 : v & 0xFFFF;
	}
	case mb_type_uint32:
	{
		quint32 v = convertFromDbus<quint32>(dbusValue, scaleFactor);
		return offset == 0 ? v >> 16 : v & 0xFFFF;
	}
	case mb_type_uint64:
	{
		// For 64-bit, multiplying an int by a double causes errors
		// in the LSB part. Therefore assume scaleFactor is always
		// an integer in this case.
		quint64 v = dbusValue.toULongLong() * int(scaleFactor);
		return (v >> (16 * (3 - offset))) & 0xFFFF;
	}
	case mb_type_string:
	{
		QByteArray b = dbusValue.toString().toLatin1();
		int index = 2 * offset;
		if (b.size() <= index)
			return 0;
		quint16 v = static_cast<quint16>(b[index] << 8);
		++index;
		if (b.size() <= index)
			return v;
		return v | b[index];
	}
	case mb_type_reserved:
		return dbusValue.toInt();
	default:
		return 0;
	}
}

void Mappings::getValues(MappingRequest *request)
{
	DataIterator it(this, request->address(), request->unitId(), request->quantity());
	QByteArray &replyData = request->data();
	replyData.reserve(2 * request->quantity());
	for (;!it.atEnd(); it.next()) {
		Q_ASSERT(it.error() == NoError);
		QList<VeQItem *> items = it.items();

		foreach(VeQItem *item, items) {
			if (item->getState() == VeQItem::Offline) {
				QLOG_TRACE() << "Value not available" << item->uniqueId();
			}
		}

		QVariantList dbusValues;
		foreach(VeQItem *item, items) {
			dbusValues.append(item->getValue());
		}

		// Apply data transformation
		QVariant dbusValue = it.data()->operation->calculate(dbusValues);

		for (int i = 0; i<it.registerCount(); ++i) {
			quint16 value = getValue(dbusValue, it.data()->modbusType, it.offset() + i,
									 it.data()->scaleFactor);
			replyData.append(static_cast<char>(value >> 8));
			replyData.append(static_cast<char>(value));
			QLOG_DEBUG() << "Get dbus value" << it.data()->objectPaths.join(", ")
						 << "offset" << it.offset() << ':' << dbusValue.toString();
		}
	}
	if (it.error() != NoError) {
		request->setError(it.error(), it.errorString());
		emit requestCompleted(request);
		return;
	}
	emit requestCompleted(request);
}

void Mappings::setValues(MappingRequest *request)
{
	DataIterator it(this, request->address(), request->unitId(), request->quantity());
	int j = 0;
	const QByteArray &data = request->data();

	for (;!it.atEnd(); it.next()) {
		Q_ASSERT(it.error() == NoError);
		// Where a register is calculated from multiple items, a WRITE should not be possible,
		// hence we are taking the easy solution of assuming there is only one item.
		VeQItem *item = it.items()[0];
		Q_ASSERT(item->getState() != VeQItem::Requested && item->getState() != VeQItem::Idle);
		quint32 value = 0;
		if (it.registerCount() < it.data()->size || it.offset() > 0) {
			// The write request does not cover all registers mapped to the current item, so we
			// retrieve the current value and overwrite parts of it with data from the request
			// later.
			QVariant dbusValue = item->getValue();
			for (int i=0; i<it.data()->size; ++i) {
				quint16 v = getValue(dbusValue, it.data()->modbusType, i, it.data()->scaleFactor);
				value = (value << 16) | v;
			}
		}
		// Copy the request data to `value`. Take care not to overwrite parts of `value` not
		// covered by the request.
		for (int i=0; i<it.registerCount(); ++i, j+=2) {
			quint16 v = (static_cast<quint8>(data[j]) << 8) | static_cast<quint8>(data[j+1]);
			int shift = 16 * (it.data()->size - i - it.offset() - 1);
			value = (value & ~(0xFFFFu << shift)) | (v << shift);
			value = v;
		}
		QVariant dbusValue;
		switch (it.data()->modbusType) {
		case mb_type_int16:
			dbusValue = convertToDbus(it.data()->dbusType, static_cast<qint16>(value),
									  it.data()->scaleFactor);
			break;
		case mb_type_uint16:
			dbusValue = convertToDbus(it.data()->dbusType, static_cast<quint16>(value),
									  it.data()->scaleFactor);
			break;
		case mb_type_int32:
			dbusValue = convertToDbus(it.data()->dbusType, static_cast<qint32>(value),
									  it.data()->scaleFactor);
			break;
		case mb_type_uint32:
			dbusValue = convertToDbus(it.data()->dbusType, static_cast<quint32>(value),
									  it.data()->scaleFactor);
			break;
		case mb_type_uint64:
			dbusValue = convertToDbus(it.data()->dbusType, static_cast<quint64>(value),
									  it.data()->scaleFactor);
			break;
		case mb_type_string:
			dbusValue = convertToDbus(it.data()->dbusType, QString(data));
		default:
			// Do nothing. dbusValue will remain invalid, which will generate an error below.
			break;
		}
		if (!dbusValue.isValid()) {
			QString errorString = QString("Could not convert value from %1").
					arg(it.data()->objectPaths.join(", "));
			request->setError(ServiceError, errorString);
			emit requestCompleted(request);
			return;
		}
		QLOG_DEBUG() << "Set dbus value" << it.data()->objectPaths.join(", ")
					 << "value to" << dbusValue.toString();
		if (item->setValue(dbusValue) != 0) {
			QString errorString = QString("SetValue failed on %1").
					arg(it.data()->objectPaths.join(", "));
			request->setError(ServiceError, errorString);
			emit requestCompleted(request);
			return;
		}
	}
	if (it.error() != NoError) {
		request->setError(it.error(), it.errorString());
		emit requestCompleted(request);
		return;
	}
	emit requestCompleted(request);
}

void Mappings::addPendingRequest(MappingRequest *request, const QList<VeQItem *> &pendingItems)
{
	if (pendingItems.isEmpty()) {
		onItemsInitialized(request);
		return;
	}
	VeQItemInitMonitor *monitor = new VeQItemInitMonitor(this);
	foreach (VeQItem *item, pendingItems)
		monitor->addItem(item);
	monitor->start();
	if (monitor->checkState()) {
		onItemsInitialized(request);
		delete  monitor;
	} else {
		mPendingRequests[monitor] = request;
		connect(monitor, SIGNAL(initialized()), this, SLOT(onItemsInitialized()));
	}
}

template<class rettype> rettype Mappings::convertFromDbus(const QVariant &value, double scaleFactor)
{
	int variantType = value.userType();

	QMetaType::Type metaType = static_cast<QMetaType::Type>(variantType);
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
		QLOG_WARN() << "[Mappings] convert from dbus type tries to convert an unsupported type:"
					<< value.type() << "(" << value.typeName() << ")";
		return 0;
	}
}

QVariant Mappings::convertToDbus(QMetaType::Type dbusType, QString value)
{
    switch (dbusType) {
    case QMetaType::QString:
        // value is already a string, so just return it
        return QVariant::fromValue(value);
    default:
        QLOG_WARN() << "[Mappings] convert to dbus type tries to convert an unsupported type:"
                    << dbusType;
        return QVariant();
    }
}

template<class argtype> QVariant Mappings::convertToDbus(QMetaType::Type dbusType,
														 argtype value, double scaleFactor)
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
		QLOG_WARN() << "[Mappings] convert to dbus type tries to convert an unsupported type:"
					<< dbusType;
		return QVariant();
	}
}

Mappings::ModbusTypes Mappings::convertModbusType(const QString &typeString)
{
	if (typeString == "int16")
		return mb_type_int16;
	if (typeString == "uint16")
		return mb_type_uint16;
	if (typeString == "int32")
		return mb_type_int32;
	if (typeString == "uint32")
		return mb_type_uint32;
	if (typeString == "uint64")
		return mb_type_uint64;
	if (typeString.startsWith(stringType))
		return mb_type_string;
	if (typeString.startsWith(reservedType))
		return mb_type_reserved;
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
	else if (typeString == "s")
		return QMetaType::QString;
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

int Mappings::convertSize(const QString &identifier, const QString &typeString)
{
	if (typeString.size() <= identifier.size() + 2 ||
		typeString[identifier.size()] != '[' ||
		!typeString.endsWith(']')) {
		return 0;
	}
	int offset = identifier.size() + 1;
	int count = typeString.size() - identifier.size() - 2;
	return typeString.mid(offset, count).toInt();
}

Mappings::DataIterator::DataIterator(const Mappings *mappings, int address, int unitId,
									 int quantity):
	mMappings(mappings),
	mQuantity(quantity),
	mOffset(0),
	mRegisterCount(0),
	mError(NoError)
{
	if (quantity <= 0) {
		mCurrent = mMappings->mDBusModbusMap.end();
		return;
	}

	QHash<int, int>::ConstIterator uIt = mMappings->mUnitIDMap.find(unitId);
	int deviceInstance = 0;
	if (uIt == mMappings->mUnitIDMap.end()) {
		/// If the unit ID is within byte range, and we cannot find it in the mapping, we assume
		/// the unit ID equals  the device instance. This is usefull because device instances
		/// are usually < 256, so we do not have to add all possible device instances to the
		/// mapping.
		if (unitId < 0 || unitId > 255) {
			setError(UnitIdError, QString("Invalid unit ID: %1").arg(unitId));
			return;
		}
		deviceInstance = unitId;
	} else {
		deviceInstance = uIt.value();
	}

	mCurrent = mMappings->mDBusModbusMap.lowerBound(address);
	if (mCurrent == mMappings->mDBusModbusMap.end()) {
		setError(StartAddressError, QString("Modbus address %1 is not registered").arg(address));
		return;
	}
	Q_ASSERT(mCurrent.key() >= address);
	if (mCurrent.key() > address) {
		if (mCurrent == mMappings->mDBusModbusMap.begin()) {
			setError(StartAddressError, QString("Modbus address %1 is not registered").arg(address));
			return;
		}
		// Note that in a QMap (unlike a QHash) all elements are ordered by key,
		// so --mCurrent will move the iterator to the last element before it. This is
		// the last value with key < modbusAddress.
		--mCurrent;
		Q_ASSERT(mCurrent.key() < address);
		if (mCurrent.key() + mCurrent.value()->size <= address) {
			setError(StartAddressError, QString("Modbus address %1 is not registered").arg(address));
			return;
		}
	}
	Q_ASSERT(mCurrent.key() <= address);
	Q_ASSERT(mCurrent.key() + mCurrent.value()->size > address);
	mOffset = address - mCurrent.key();
	Q_ASSERT(mOffset >= 0);
	mRegisterCount = qMin(mCurrent.key() + mCurrent.value()->size - address, quantity);
	Q_ASSERT(mRegisterCount > 0);
	Q_ASSERT(mRegisterCount <= mCurrent.value()->size);
	Q_ASSERT(mRegisterCount <= quantity);

	// Get service from the first modbus address. The service must be the same
	// for the complete address range therefore the service pointer has to be
	// fetched and checked only once
	mService = mMappings->mServices->getService(mCurrent.value()->deviceType, deviceInstance);
	if (mService == 0 || !mService->getConnected()) {
		QString msg = QString("Error finding service with device type %1 at device instance %2").
				arg(mCurrent.value()->deviceType).
				arg(deviceInstance);
		setError(ServiceError, msg);
		return;
	}
}

MappingErrors Mappings::DataIterator::error() const
{
	return mError;
}

QString Mappings::DataIterator::errorString() const
{
	return mErrorString;
}

void Mappings::DataIterator::next()
{
	if (mCurrent == mMappings->mDBusModbusMap.end())
		return;
	mQuantity -= mRegisterCount;
	Q_ASSERT(mQuantity >= 0);
	if (mQuantity == 0) {
		mCurrent = mMappings->mDBusModbusMap.end();
		return;
	}
	DBusModbusData *d = mCurrent.value();
	int oldAddress = mCurrent.key();
	++mCurrent;
	int newAddress = oldAddress + d->size;
	if (mCurrent == mMappings->mDBusModbusMap.end() || mCurrent.key() != newAddress) {
		setError(AddressError, QString("Modbus address %1 is not registered").arg(newAddress));
		return;
	}
	mOffset = 0;
	mRegisterCount = qMin(mCurrent.value()->size, mQuantity);
}

bool Mappings::DataIterator::atEnd() const
{
	return mCurrent == mMappings->mDBusModbusMap.end();
}

const Mappings::DBusModbusData *Mappings::DataIterator::data() const
{
	if (mOffset < 0)
		return 0;
	return mCurrent == mMappings->mDBusModbusMap.end() ? 0 : *mCurrent;
}

QList<VeQItem *> Mappings::DataIterator::items() const
{
	if (mCurrent == mMappings->mDBusModbusMap.end())
		return QList<VeQItem *>();

	QList<VeQItem *> l;
	foreach(QString p, mCurrent.value()->objectPaths) {
		l.append(mService->getItem(p));
	}
	return l;
}

int Mappings::DataIterator::offset() const
{
	return mOffset;
}

int Mappings::DataIterator::address() const
{
	return mCurrent == mMappings->mDBusModbusMap.end() ? -1 : mCurrent.key() + mOffset;
}

int Mappings::DataIterator::registerCount() const
{
	return mRegisterCount;
}

void Mappings::DataIterator::setError(MappingErrors error, const QString &errorString)
{
	mCurrent = mMappings->mDBusModbusMap.end();
	mError = error;
	mErrorString = errorString;
}

Mappings::DBusModbusData::DBusModbusData(QString _deviceType, QStringList _objectPaths,
	double _scaleFactor, int _size, ModbusTypes _modbusType, QMetaType::Type _dbusType,
	Permissions _accessRights, Operation *_operation) :
	deviceType(_deviceType), objectPaths(_objectPaths), size(_size),
	modbusType(_modbusType), dbusType(_dbusType), operation(_operation)
{
	if (_scaleFactor == 0)
		_scaleFactor = 1;
	scaleFactor = _scaleFactor;

	accessRights = _accessRights;
}

// Machinery for doing simple operations between registers
// Oversimplified: Only supports two values for now. Extend if ever required.
Mappings::Operation::~Operation()
{
}

QVariant Mappings::DivOperation::calculate(QList<QVariant> args)
{
	return QVariant(args[0].toDouble() / args[1].toDouble());
}

QVariant Mappings::NopOperation::calculate(QList<QVariant> args)
{
	return args[0];
}

Mappings::ReservedOperation::ReservedOperation(int size) : mSize(size)
{
}

QVariant Mappings::ReservedOperation::calculate(QList<QVariant> args)
{
	Q_UNUSED(args);
	return QVariant(0xFFFF);
}

QVariant Mappings::TimeOperation::calculate(QList<QVariant> args)
{
	Q_UNUSED(args);
	return QVariant(
		QDateTime::currentDateTimeUtc().currentMSecsSinceEpoch()/1000);
}
