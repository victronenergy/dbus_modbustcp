#include <QCoreApplication>
#include <velib/qt/ve_qitems_dbus.hpp>
#include <velib/qt/v_busitems.h>
#include "app.h"
#include "arguments.h"
#include "nostorage_qitem_producer.h"
#include "QsLog.h"

void initLogger(QsLogging::Level logLevel)
{
	QsLogging::Logger &logger = QsLogging::Logger::instance();
	QsLogging::DestinationPtr debugDestination(
			QsLogging::DestinationFactory::MakeDebugOutputDestination() );
	logger.addDestination(debugDestination);
	logger.setIncludeTimestamp(false);

	QLOG_INFO() << "dbus_modbustcp" << "v" VERSION << "started";
	QLOG_INFO() << "Built with Qt" << QT_VERSION_STR << "running on" << qVersion();
	QLOG_INFO() << "Built on" << __DATE__ << "at" << __TIME__;
	logger.setLoggingLevel(logLevel);
}

void usage(Arguments &arg)
{
	arg.addArg("-h", "Print this help");
	arg.addArg("-d level", "Debug level: 0=TRACE, 1=DEBUG, 2=INFO...");
	arg.addArg("--dbus", "D-Bus connection: session, system, ...");
	arg.addArg("-p", "Modbus TCP port");
}

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	Arguments arg;

	usage(arg);
	if (arg.contains("h")) {
		arg.help();
		exit(0);
	}

	int tcpPort = 502;
	if (arg.contains("p"))
		tcpPort = arg.value("p").toInt();

	QsLogging::Level logLevel = QsLogging::InfoLevel;
	if (arg.contains("d"))
		logLevel = static_cast<QsLogging::Level>(arg.value("d").toInt());
	initLogger(logLevel);

	QString dbusConnection = arg.contains("dbus") ? arg.value("dbus") : "system";

	NostorageQItemProducer producer(VeQItems::getRoot(), "sub", true, false);
	producer.setListenIndividually(true);
	producer.open(dbusConnection);

	App dbusModbusApp(producer.services(), tcpPort);

	return app.exec();
}
