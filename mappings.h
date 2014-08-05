#ifndef MAPPINGS_H
#define MAPPINGS_H

#include <QObject>
#include <QHash>
#include "dbus_services.h"
#include "dbus_service.h"

class Mappings : public QObject
{
	Q_OBJECT
public:
	Mappings(DBusServices *services, QObject *parent = 0);

public slots:
	void getValues(const int modbusAddress, const int unitID, const int quantity, QByteArray &replyData);
	void setValues(const int modbusAddress, const int unitID, const int quantity, QByteArray &data);

signals:

private slots:
	void dbusServiceFound(DBusService *service);

private:
	enum ModbusTypes { mb_type_none, mb_type_uint16, mb_type_int16 };
	enum Permissions { mb_perm_none, mb_perm_read, mb_perm_write };

	struct DBusModbusData {
		QString deviceType;
		QString objectPath;
		double scaleFactor;
		ModbusTypes modbusType;
		QMetaType::Type dbusType;
		Permissions accessRights;
	};

	bool getValue(const int modbusAddress, const int unitID, quint16 &value);
	bool setValue(const int modbusAddress, const int unitID, quint16 value);
	void importCSV(const QString &filename);
	void importUnitIDMapping(const QString &filename);
	template<class rettype> rettype convertFromDbus(const QVariant &value, const float scaleFactor);
	template<class argtype> QVariant convertToDbus(const QMetaType::Type dbusType, const argtype value, const float scaleFactor);
	ModbusTypes convertModbusType(const QString &typeString);
	QMetaType::Type convertDbusType(const QString &typeString);
	Permissions convertPermissions(const QString &permissions);

	DBusServices *mServices;
	// modbus register -> unit id -> dbus<->modbus data
	QHash< int, DBusModbusData* > mDBusModbusMap;

	// Unit ID -> /DeviceInstance
	QHash< int, int> mUnitIDMap;
};


#endif // MAPPINGS_H
