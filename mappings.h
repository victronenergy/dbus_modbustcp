#ifndef MAPPINGS_H
#define MAPPINGS_H

#include <QMap>
#include <QObject>
#include <QHash>
#include "dbus_services.h"
#include "dbus_service.h"

class Mappings : public QObject
{
	Q_OBJECT
public:
	Mappings(DBusServices *services, QObject *parent = 0);

	enum MappingErrors { NoError, QuantityError, StartAddressError, AddressError, UnitIdError, ServiceError, PermissionError};

public slots:
	void getValues(const int modbusAddress, const int unitID, const int quantity, QByteArray &replyData, Mappings::MappingErrors &error) const;
	void setValues(const int modbusAddress, const int unitID, const int quantity, QByteArray &data, Mappings::MappingErrors &error) ;

signals:

private slots:
	void dbusServiceFound(DBusService *service);

private:
	enum ModbusTypes { mb_type_none, mb_type_uint16, mb_type_int16, mb_type_uint32, mb_type_int32, mb_type_string };
	enum Permissions { mb_perm_none, mb_perm_read, mb_perm_write };

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

	quint16 getValue(const DBusService *service, const QString & objectPath, const ModbusTypes modbusType, const int offset, const double scaleFactor, Mappings::MappingErrors &error) const;
	int getDeviceInstance(int unitID) const;

	bool setValue(DBusService * const service, const QString &objectPath, const ModbusTypes modbusType, const QMetaType::Type dbusType, const double scaleFactor, const quint16 value);
	void importCSV(const QString &filename);
	void importUnitIDMapping(const QString &filename);
	template<class rettype> rettype convertFromDbus(const QVariant &value, const float scaleFactor) const;
	template<class argtype> QVariant convertToDbus(const QMetaType::Type dbusType, const argtype value, const float scaleFactor) const;
	ModbusTypes convertModbusType(const QString &typeString);
	QMetaType::Type convertDbusType(const QString &typeString);
	Permissions convertPermissions(const QString &permissions);
	int convertStringSize(const QString &typeString);
	int findBaseAddress(int modbusAddress) const;

	DBusServices *mServices;
	// modbus register -> unit id -> dbus<->modbus data
	QMap< int, DBusModbusData* > mDBusModbusMap;

	// Unit ID -> /DeviceInstance
	QHash< int, int> mUnitIDMap;
};


#endif // MAPPINGS_H
