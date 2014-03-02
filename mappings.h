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

signals:

private slots:
	void dbusServiceFound(DBusService *service);

private:
	enum ModbusValueTypes { mb_type_none, mb_type_uint16, mb_type_int16 } ;

	struct DBusModbusData {
		DBusService::DbusServiceType deviceType;
		QString objectPath;
		double scaleFactor;
		ModbusValueTypes valueType;
	};

	bool getValue(const int modbusAddress, const int unitID, quint16 &value);
	void importCSV(const QString &filename);
	void importUnitIDMapping(const QString &filename);
	quint16 convertToUInt16(QVariant value, float scaleFactor);
	qint16 convertToInt16(QVariant value, float scaleFactor);
	ModbusValueTypes convertType(const QString &typeString);

	DBusServices *mServices;
	// modbus register -> unit id -> dbus<->modbus data
	QHash< int, DBusModbusData* > mDBusModbusMap;

	// Unit ID -> /DeviceInstance
	QHash< int, int> mUnitIDMap;
};


#endif // MAPPINGS_H
