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

App::App(VeQItem *subRoot, VeQItem *pubRoot, QObject *transport, QObject *parent) :
	QObject(parent),
	mTransport(transport),
	mBackend(parent),
	mDBusServices(subRoot, parent),
	mMapping(&mDBusServices, parent)
{
	mTransport->setParent(this);

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

	connect(mTransport, SIGNAL(modbusRequest(ADU*)), &mBackend, SLOT(modbusRequest(ADU*)));
	connect(&mBackend, SIGNAL(modbusReply(ADU*)), mTransport, SLOT(modbusReply(ADU*)));
	connect(&mBackend, SIGNAL(mappingRequest(MappingRequest *)), &mMapping, SLOT(handleRequest(MappingRequest *)));
	connect(&mMapping, SIGNAL(requestCompleted(MappingRequest *)),
			&mBackend, SLOT(requestCompleted(MappingRequest *)));
	// pubRoot is null when the transport does not publish a diagnostics service
	// (the RTU slave), so only export com.victronenergy.modbustcp when we have one.
	if (pubRoot) {
		VeQItem *serviceRoot = pubRoot->itemGetOrCreate("com.victronenergy.modbustcp");
		new DiagnosticsService(&mDBusServices, &mMapping, serviceRoot, this);
	}
	mDBusServices.initialScan();
}
