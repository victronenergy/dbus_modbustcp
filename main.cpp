#include <QCoreApplication>
#include <QDBusConnection>
#include "app.h"
#include "QsLog.h"
#include "defines.h"
#include "version.h"
#include "arguments.h"

QsLogging::Logger& logger = QsLogging::Logger::instance();

void initLogger(QsLogging::Level logLevel)
{
	// init the logging mechanism
	QsLogging::DestinationPtr debugDestination(
			QsLogging::DestinationFactory::MakeDebugOutputDestination() );
	logger.addDestination(debugDestination);

	QLOG_INFO() << "dbus_modbustcp" << VERSION << " started" << " (" << REVISION << ")";
	QLOG_INFO() << "Built with Qt" << QT_VERSION_STR << "running on" << qVersion();
	QLOG_INFO() << "Built on" << __DATE__ << "at" << __TIME__;
	logger.setLoggingLevel(logLevel);
}

void usage(Arguments & arg)
{
	arg.addArg("-h", "Print this help");
	arg.addArg("-d level", "Debug level: 0=TRACE, 1=DEBUG, 2=INFO...");
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

	initLogger(QsLogging::InfoLevel);
	if (arg.contains("d"))
		logger.setLoggingLevel((QsLogging::Level)arg.value("d").toInt());

	QDBusConnection dbus = DBUS_CONNECTION;
	if (!dbus.isConnected()) {
		QLOG_ERROR() << "DBus connection failed.";
		exit(EXIT_FAILURE);
	}

	App dbusModbusApp;

	return app.exec();
}
