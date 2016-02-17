#ifndef MAPPINGS_H
#define MAPPINGS_H

#include <QHash>
#include <QList>
#include <QMap>
#include <QMetaType>
#include <QObject>
#include "mapping_request.h"

class DBusServices;
class VeQItem;
class VeQItemInitMonitor;

class Mappings : public QObject
{
	Q_OBJECT
public:
	Mappings(DBusServices *services, QObject *parent = 0);

	~Mappings();

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
		mb_type_string
	};

	enum Permissions {
		mb_perm_none,
		mb_perm_read,
		mb_perm_write
	};

	struct DBusModbusData {
		QString deviceType;
		QString objectPath;
		double scaleFactor;
		/// Number of registers used for single object. 1 for all uin16 types,
		/// >= 1 for strings.
		int size;
		ModbusTypes modbusType;
		QMetaType::Type dbusType;
		Permissions accessRights;
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
		VeQItem *item() const;
		/// The difference (counted in registers) between the address from the modbus request
		/// and the address associated with the current VeQItem.
		int offset() const;
		/// The address associated with the current VeQItem.
		int address() const;

	private:
		void setError(MappingErrors error, const QString &errorString);
		const Mappings *mMappings;
		QMap<int, DBusModbusData *>::ConstIterator mCurrent;
		int mQuantity;
		int mOffset;
		VeQItem *mServiceRoot;
		QString mErrorString;
		MappingErrors mError;
	};

	friend DataIterator;

	void getValues(MappingRequest *request);
	void setValues(MappingRequest *request);
	void addPendingRequest(MappingRequest *request, const QList<VeQItem *> &pendingItems);
	static quint16 getValue(const QVariant &dbusValue, const ModbusTypes modbusType, int offset,
							double scaleFactor);
	void importCSV(const QString &filename);
	void importUnitIDMapping(const QString &filename);
	template<class rettype> static rettype convertFromDbus(const QVariant &value, double scaleFactor);
	template<class argtype> static QVariant convertToDbus(const QMetaType::Type dbusType, argtype value,
														  double scaleFactor);
	ModbusTypes convertModbusType(const QString &typeString);
	QMetaType::Type convertDbusType(const QString &typeString);
	Permissions convertPermissions(const QString &permissions);
	int convertStringSize(const QString &typeString);

	DBusServices *mServices;
	// modbus register -> unit id -> dbus <-> modbus data
	QMap<int, DBusModbusData *> mDBusModbusMap;

	// Unit ID -> DeviceInstance
	QHash<int, int> mUnitIDMap;

	QMap<VeQItemInitMonitor *, MappingRequest *> mPendingRequests;
};

#endif // MAPPINGS_H
