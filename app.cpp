#include <veutil/qt/ve_qitems_dbus.hpp>
#include "diagnostics_service.h"
#include "app.h"
#include "QsLog.h"

class SettingsInfo : public VeQItemSettingsInfo
{
public:
	SettingsInfo()
	{
		add("ModbusServer/ReadWrite", 1, 0, 1);
	}
};

App::App(VeQItem *subRoot, VeQItem *pubRoot, int tcpPort, QObject *parent) :
	QObject(parent),
	mServer(tcpPort, parent),
	mBackend(parent),
	mDBusServices(subRoot, parent),
	mMapping(&mDBusServices, parent)
{
	mMapping.importCSV("attributes.csv");
	mMapping.importUnitIDMapping("unitid2di.csv");

	QLOG_INFO() << "Creating settings";
	mSettings = new VeQItemDbusSettings(
		VeQItems::getRoot()->itemGetOrCreate("sub"),
		QString("com.victronenergy.settings"));
	mSettings->addSettings(SettingsInfo());

	// If localsettings is not up yet, this will result in assuming that modbus
	// is read-only.
	VeQItem *rw = mSettings->root()->itemGetOrCreate("Settings/ModbusServer/ReadWrite");
	connect(rw, SIGNAL(valueChanged(QVariant)), &mMapping, SLOT(onReadWriteChanged(QVariant)));
	mMapping.onReadWriteChanged(rw->getValue());

	connect(&mServer, SIGNAL(modbusRequest(ADU*)), &mBackend, SLOT(modbusRequest(ADU*)));
	connect(&mBackend, SIGNAL(modbusReply(ADU*)), &mServer, SLOT(modbusReply(ADU*)));
	connect(&mBackend, SIGNAL(mappingRequest(MappingRequest *)), &mMapping, SLOT(handleRequest(MappingRequest *)));
	connect(&mMapping, SIGNAL(requestCompleted(MappingRequest *)),
			&mBackend, SLOT(requestCompleted(MappingRequest *)));
	VeQItem *serviceRoot = pubRoot->itemGetOrCreate("com.victronenergy.modbustcp");
	new DiagnosticsService(&mDBusServices, &mMapping, serviceRoot, this);
	mDBusServices.initialScan();
}
