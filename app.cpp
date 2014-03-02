#include "app.h"

App::App(QObject *parent) :
	QObject(parent),
	mServer(parent),
	mBackend(parent),
	mDBusServices(parent),
	mMapping(&mDBusServices, parent)
{
	connect(&mServer, SIGNAL(modbusRequest(ADU*const)), &mBackend, SLOT(modbusRequest(ADU*const)));
	connect(&mBackend, SIGNAL(modbusReply(ADU*const)), &mServer, SLOT(modbusReply(ADU*const)));
	connect(&mBackend, SIGNAL(getValues(int,int,int,QByteArray&)),&mMapping, SLOT(getValues(int,int,int,QByteArray&)));
	//connect(&mDBusServices, SIGNAL(dbusServiceFound(DBusService*)), &mMapping, SLOT(dbusServiceFound(DBusService*)));

	mDBusServices.initialScan();
}
