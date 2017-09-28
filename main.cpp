#include <QCoreApplication>
#include <QDBusConnection>
#include "app.h"
#include "QsLog.h"
#include "arguments.h"

void initLogger(QsLogging::Level logLevel)
{
	QsLogging::Logger &logger = QsLogging::Logger::instance();
	QsLogging::DestinationPtr debugDestination(
			QsLogging::DestinationFactory::MakeDebugOutputDestination() );
	logger.addDestination(debugDestination);

	QLOG_INFO() << "dbus_modbustcp" << "v" VERSION << "started";
	QLOG_INFO() << "Built with Qt" << QT_VERSION_STR << "running on" << qVersion();
	QLOG_INFO() << "Built on" << __DATE__ << "at" << __TIME__;
	logger.setLoggingLevel(logLevel);
}

void usage(Arguments & arg)
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

	QDBusConnection dbus = QDBusConnection::systemBus();
	if (arg.contains("dbus")) {
		QString dbusAddress = arg.value("dbus");
		if (dbusAddress != "system") {
			if (dbusAddress == "session")
				dbus = QDBusConnection::sessionBus();
			else
				dbus = QDBusConnection::connectToBus(dbusAddress, "modbus_tcp");
		}
	}

	if (!dbus.isConnected()) {
		QLOG_ERROR() << "DBus connection failed.";
		exit(EXIT_FAILURE);
	}

	App dbusModbusApp(dbus, tcpPort);

	return app.exec();
}
