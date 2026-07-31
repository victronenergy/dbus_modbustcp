#include <QCoreApplication>
#include <veutil/qt/ve_qitems_dbus.hpp>
#include "app.h"
#include "arguments.h"
#include "nostorage_qitem_producer.h"
#include "rtu_server.h"
#include "QsLog.h"

void initLogger(QsLogging::Level logLevel)
{
	QsLogging::Logger &logger = QsLogging::Logger::instance();
	QsLogging::DestinationPtr debugDestination(
			QsLogging::DestinationFactory::MakeDebugOutputDestination() );
	logger.addDestination(debugDestination);
	logger.setIncludeTimestamp(false);

	QLOG_INFO() << "dbus_modbusrtu" << "v" VERSION << "started";
	QLOG_INFO() << "Built with Qt" << QT_VERSION_STR << "running on" << qVersion();
	QLOG_INFO() << "Built on" << __DATE__ << "at" << __TIME__;
	logger.setLoggingLevel(logLevel);
}

void usage(Arguments &arg)
{
	arg.addArg("-h", "Print this help");
	arg.addArg("-d level", "Debug level: 0=TRACE, 1=DEBUG, 2=INFO...");
	arg.addArg("--dbus", "D-Bus connection: session, system, ...");
	arg.addArg("-s", "Serial device for the Modbus RTU slave (e.g. /dev/ttyUSB0)");
	arg.addArg("-b", "Serial baud rate (8N1, default 9600)");
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

	if (!arg.contains("s")) {
		QLOG_ERROR() << "No serial device given, use -s <device>";
		arg.help();
		exit(1);
	}
	QString serialDevice = arg.value("s");
	int baud = arg.contains("b") ? arg.value("b").toInt() : 9600;

	QsLogging::Level logLevel = QsLogging::InfoLevel;
	if (arg.contains("d"))
		logLevel = static_cast<QsLogging::Level>(arg.value("d").toInt());
	initLogger(logLevel);

	QString dbusConnection = arg.contains("dbus") ? arg.value("dbus") : "system";

	NostorageQItemProducer producer(VeQItems::getRoot(), "sub", true, false);
	producer.setAutoCreateItems(false);
	producer.open(dbusConnection);

	// The RTU slave only acts as a gateway; it publishes no diagnostics service,
	// so pass a null publish root to App.
	App dbusModbusApp(producer.services(), 0,
					  new RtuServer(serialDevice, baud));

	return app.exec();
}
