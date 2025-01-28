#ifndef MAPPINGS_H
#define MAPPINGS_H

#include <QHash>
#include <QList>
#include <QStringList>
#include <QMap>
#include <QMetaType>
#include <QObject>
#include "mapping_request.h"

class DBusService;
class DBusServices;
class QTextStream;
class VeQItem;
class VeQItemInitMonitor;

class Mappings : public QObject
{
	Q_OBJECT
public:
	Mappings(DBusServices *services, QObject *parent = 0);

	~Mappings();

	void importCSV(const QString &filename);

	void importCSV(QTextStream &filename);

	void importUnitIDMapping(const QString &filename);

	void importUnitIDMapping(QTextStream &filename);

	int getUnitId(int deviceInstance) const;

public slots:
	void handleRequest(MappingRequest *request);

signals:
	void requestCompleted(MappingRequest *request);

private slots:
	void onItemsInitialized();

	void onItemsInitialized(MappingRequest *request);

private:
	enum ModbusTypes {
		mb_type_none,
		mb_type_uint16,
		mb_type_int16,
		mb_type_uint32,
		mb_type_int32,
		mb_type_uint64,
		mb_type_string,
		mb_type_reserved
	};

	enum Permissions {
		mb_perm_none,
		mb_perm_read,
		mb_perm_write
	};

	class Operation;
	struct DBusModbusData {
		DBusModbusData(QString _deviceType, QStringList _objectPaths,
			double _scaleFactor, int _size, ModbusTypes _modbusType, QMetaType::Type _dbusType,
			Permissions _accessRights, Operation *_operation);
		QString deviceType;
		QStringList objectPaths;
		double scaleFactor;
		/// Number of registers used for single object. 1 for all uin16 types,
		/// >= 1 for strings.
		int size;
		ModbusTypes modbusType;
		QMetaType::Type dbusType;
		Permissions accessRights;
		Operation *operation;
	};

	/// Iterates over all VeQItems involved in a single modbus request.
	/// In each step of the iteration, it contains data on how to extract data from the value
	/// of the VeQItem (offset and byte count).
	/// Example: suppose the request asks for 3 registers starting with address 802. The mapping
	/// defines a D-Bus object at register 800 and a size of 6 registers (a 12 character string).
	/// The first step in the iteration would show offset=2 registerCount=3 and address=800.
	/// If this is a read request, the value of the current VeQItem should be converted to
	/// binary data, and byte 4 (=2*offset) through 10 (=2*(offset + registerCount)) should be
	/// copied from it to the modbus request data.
	class DataIterator {
	public:
		DataIterator(const Mappings *mappings, int address, int unitId, int quantity);

		MappingErrors error() const;
		QString errorString() const;
		void next();
		bool atEnd() const;
		const DBusModbusData *data() const;
		QList<VeQItem *> items() const;
		/// The difference (counted in registers) between the address from the modbus request
		/// and the address associated with the current VeQItem.
		int offset() const;
		/// The address associated with the current VeQItem.
		int address() const;
		/// The number of registers to mapped to the data from the current VeQItem.
		int registerCount() const;

	private:
		void setError(MappingErrors error, const QString &errorString);
		const Mappings *mMappings;
		QMap<int, DBusModbusData *>::ConstIterator mCurrent;
		int mQuantity;
		int mOffset;
		int mRegisterCount;
		DBusService *mService;
		QString mErrorString;
		MappingErrors mError;
	};

	friend DataIterator;

	void getValues(MappingRequest *request);
	void setValues(MappingRequest *request);
	void addPendingRequest(MappingRequest *request, const QList<VeQItem *> &pendingItems);
	static quint16 getValue(const QVariant &dbusValue, const ModbusTypes modbusType, int offset,
							double scaleFactor);
	template<class rettype> static rettype convertFromDbus(const QVariant &value, double scaleFactor);
	template<class argtype> static QVariant convertToDbus(const QMetaType::Type dbusType, argtype value,
														  double scaleFactor);
	QVariant convertToDbus(QMetaType::Type dbusType, QString value);
	ModbusTypes convertModbusType(const QString &typeString);
	QMetaType::Type convertDbusType(const QString &typeString);
	Permissions convertPermissions(const QString &permissions);
	int convertSize(const QString &identifier, const QString &typeString);
	DBusServices *mServices;
	// modbus register -> unit id -> dbus <-> modbus data
	QMap<int, DBusModbusData *> mDBusModbusMap;

	// Unit ID -> DeviceInstance
	QHash<int, int> mUnitIDMap;

	QMap<VeQItemInitMonitor *, MappingRequest *> mPendingRequests;

	// Simple operations that can be done to values
	class Operation {
	public:
		virtual QVariant calculate(QList<QVariant> args) = 0;
		virtual ~Operation();
	};

	class DivOperation : public Operation { QVariant calculate(QList<QVariant> args); };
	class NopOperation : public Operation { QVariant calculate(QList<QVariant> args); };
	class TimeOperation : public Operation { QVariant calculate(QList<QVariant> args); };

	// To mark reserved ranges
	class ReservedOperation : public Operation {
	public:
		ReservedOperation(int size);
		QVariant calculate(QList<QVariant> args);
	private:
		int mSize;
	};

	// class level instances of operations
	DivOperation mDivOperation;
	NopOperation mNopOperation;
	TimeOperation mTimeOperation;
};

#endif // MAPPINGS_H
