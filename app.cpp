#include "app.h"

App::App(const QDBusConnection &dbus, int tcpPort, QObject *parent) :
	QObject(parent),
	mServer(tcpPort, parent),
	mBackend(parent),
	mDBusServices(dbus, parent),
	mMapping(&mDBusServices, parent)
{
	connect(&mServer, SIGNAL(modbusRequest(ADU*const)), &mBackend, SLOT(modbusRequest(ADU*const)));
	connect(&mBackend, SIGNAL(modbusReply(ADU*const)), &mServer, SLOT(modbusReply(ADU*const)));
	connect(&mBackend, SIGNAL(getValues(int,int,int,QByteArray&,Mappings::MappingErrors&)),&mMapping, SLOT(getValues(int,int,int,QByteArray&,Mappings::MappingErrors&)));
	connect(&mBackend, SIGNAL(setValues(int,int,int,QByteArray&,Mappings::MappingErrors&)),&mMapping, SLOT(setValues(int,int,int,QByteArray&,Mappings::MappingErrors&)));

	mDBusServices.initialScan();
}
