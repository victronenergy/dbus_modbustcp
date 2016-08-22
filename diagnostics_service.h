#ifndef DIAGNOSTICSSERVICE_H
#define DIAGNOSTICSSERVICE_H

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QsLog.h>

class DBusService;
class DBusServices;
class Mappings;
class QTimer;
class VeQItem;

/// Publishes a D-Bus serivce (com.victronenergy.modbustcp) which contains diagnostics information
/// on the modbus TCP services.
/// D-Bus paths:
/// /LastError/Message				''		   <- text of latest error message.
/// /LastError/Timestamp			1485350774 <- unix timestamp of the latest error (or invalid)
/// /Services/Count					3		   <- number of services which can be queried.
/// /Services/0/ServiceName			com.victronenergy.battery.ttyO2
/// /Services/0/UnitId				245
/// /Services/0/IsActive			1		   <- 0 When service has existed in the past, but has
///												    disappeared. Old values are still accessible.
/// /Service/1/ServiceName ...
class DiagnosticsService : public QObject
{
	Q_OBJECT
public:
	DiagnosticsService(DBusServices *services, Mappings *mappings, VeQItem *root, QObject *parent = 0);

	void setError(const QString &error);

private slots:
	void onServiceFound(DBusService *service);

	void onDeviceInstanceChanged(VeQItem *item);

	void onServiceStateChanged(VeQItem *item);

	void onLastErrorTimer();

private:
	VeQItem *getServiceItem(VeQItem *serviceRoot, VeQItem *deviceInstance);

	VeQItem *getServiceItem(VeQItem *serviceRoot);

	VeQItem *createServiceItem(VeQItem *deviceInstance);

	void updateService(VeQItem *serviceEntry, VeQItem *serviceRoot);

	QString mLastErrorText;
	QDateTime mLastErrorTime;
	/// This timer is used to ensure that mLastError is not updated more than once per second.
	QTimer *mLastErrorTimer;
	DBusServices *mServices;
	Mappings *mMappings;
	VeQItem *mRoot;
	VeQItem *mLastError;
	VeQItem *mLastErrorTimeStamp;
	VeQItem *mServiceCount;
};

class DiagnosticsDestination : public QsLogging::Destination
{
public:
	DiagnosticsDestination(DiagnosticsService *service):
		mService(service)
	{
	}

	virtual void write(const QString& message, QsLogging::Level level)
	{
		if (mService && level == QsLogging::ErrorLevel)
			mService->setError(message);
	}

	virtual bool isValid()
	{
		return mService;
	}

private:
	QPointer<DiagnosticsService> mService;
};

#endif // DIAGNOSTICSSERVICE_H
