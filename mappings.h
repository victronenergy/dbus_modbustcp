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
		QString deviceType;
		QString objectPath;
		double scaleFactor;
		ModbusValueTypes valueType;
	};

	bool getValue(const int modbusAddress, const int unitID, quint16 &value);
	void importCSV(const QString &filename);
	void importUnitIDMapping(const QString &filename);
	quint16 convertUInt16(const QVariant &value, const float scaleFactor);
	quint16 convertInt16(const QVariant &value, const float scaleFactor);
	ModbusValueTypes convertType(const QString &typeString);

	DBusServices *mServices;
	// modbus register -> unit id -> dbus<->modbus data
	QHash< int, DBusModbusData* > mDBusModbusMap;

	// Unit ID -> /DeviceInstance
	QHash< int, int> mUnitIDMap;
};


#endif // MAPPINGS_H
