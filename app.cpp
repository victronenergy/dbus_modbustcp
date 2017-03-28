#include <velib/qt/ve_qitem.hpp>
#include "diagnostics_service.h"
#include "app.h"

App::App(VeQItem *subRoot, VeQItem *pubRoot, int tcpPort, QObject *parent) :
	QObject(parent),
	mServer(tcpPort, parent),
	mBackend(parent),
	mDBusServices(subRoot, parent),
	mMapping(&mDBusServices, parent)
{
	mMapping.importCSV("attributes.csv");
	mMapping.importUnitIDMapping("unitid2di.csv");
	connect(&mServer, SIGNAL(modbusRequest(ADU*)), &mBackend, SLOT(modbusRequest(ADU*)));
	connect(&mBackend, SIGNAL(modbusReply(ADU*)), &mServer, SLOT(modbusReply(ADU*)));
	connect(&mBackend, SIGNAL(mappingRequest(MappingRequest *)), &mMapping, SLOT(handleRequest(MappingRequest *)));
	connect(&mMapping, SIGNAL(requestCompleted(MappingRequest *)),
			&mBackend, SLOT(requestCompleted(MappingRequest *)));
	VeQItem *serviceRoot = pubRoot->itemGetOrCreate("com.victronenergy.modbustcp");
	new DiagnosticsService(&mDBusServices, &mMapping, serviceRoot, this);
	mDBusServices.initialScan();
}
