#include <QStringList>
#include <QDBusConnectionInterface>
#include "dbus.h"
#include "defines.h"
#include "QsLog.h"

dbus::dbus(QObject *parent) :
	QObject(parent),
	mDBus(DBUS_CONNECTION),
	mDBusWatcher(parent)
{
	mDBusWatcher.setConnection(DBUS_CONNECTION);
	mDBusWatcher.setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);
	mDBusWatcher.addWatchedService("com.victronenergy.qwacs");
	connect(&mDBusWatcher, SIGNAL(serviceRegistered(QString)), this, SLOT(serviceRegisterd(QString)));
	connect(&mDBusWatcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(serviceUnregisterd(QString)));
	//connect(&mDBusWatcher, SIGNAL(serviceOwerChanged(QString,QString,QString)), this, SLOT(serviceOwnerChanged(QString,QString,QString)));

	QDBusConnectionInterface *interface = mDBus.interface();
	QStringList serviceNames = interface->registeredServiceNames();
	QLOG_TRACE() << "Manager serviceNames:" << serviceNames;
	connect(interface, SIGNAL(serviceOwnerChanged(QString,QString,QString)), this, SLOT(serviceOwnerChanged(QString,QString,QString)));
}

void dbus::serviceRegisterd(const QString serviceName)
{
	QLOG_INFO() << "[dbus] serviceRegisterd" << serviceName;
}

void dbus::serviceUnregisterd(const QString serviceName)
{
	QLOG_TRACE() << "[dbus] serviceUnregisterd" << serviceName;
}

void dbus::serviceOwnerChanged(const QString & name, const QString & oldOwner, const QString & newOwner)
{
	QLOG_TRACE() << "[dbus] serviceOwnerChanged name=" << name << " oldOwner=" << oldOwner << " newOwner=" << newOwner ;
}
