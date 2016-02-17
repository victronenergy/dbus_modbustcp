#include "app.h"

App::App(VeQItem *dbusRoot, int tcpPort, QObject *parent) :
	QObject(parent),
	mServer(tcpPort, parent),
	mBackend(parent),
	mDBusServices(dbusRoot, parent),
	mMapping(&mDBusServices, parent)
{
	connect(&mServer, SIGNAL(modbusRequest(ADU*)), &mBackend, SLOT(modbusRequest(ADU*)));
	connect(&mBackend, SIGNAL(modbusReply(ADU*)), &mServer, SLOT(modbusReply(ADU*)));
	connect(&mBackend, SIGNAL(mappingRequest(MappingRequest *)), &mMapping, SLOT(handleRequest(MappingRequest *)));
	connect(&mMapping, SIGNAL(requestCompleted(MappingRequest *)),
			&mBackend, SLOT(requestCompleted(MappingRequest *)));
	mDBusServices.initialScan();
}
