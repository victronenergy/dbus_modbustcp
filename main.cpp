#include <QCoreApplication>
#include <velib/qt/ve_qitems_dbus.hpp>
#include <velib/qt/ve_qitem_dbus_publisher.hpp>
#include "app.h"
#include "arguments.h"
#include "nostorage_qitem_producer.h"
#include "logging.h"

void usage(Arguments &arg)
{
	arg.addArg("-h", "Print this help");
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

	qInfo() << "dbus_modbustcp" << "v" VERSION << "started";
	qInfo() << "Built with Qt" << QT_VERSION_STR << "running on" << qVersion();
	qInfo() << "Built on" << __DATE__ << "at" << __TIME__;

	QString dbusConnection = arg.contains("dbus") ? arg.value("dbus") : "system";

	NostorageQItemProducer producer(VeQItems::getRoot(), "sub", true, false);
	producer.setAutoCreateItems(false);
	producer.open(dbusConnection);

	VeQItemProducer pub(VeQItems::getRoot(), "pub");
	VeQItemDbusPublisher publisher(pub.services());
	publisher.open(dbusConnection);

	App dbusModbusApp(producer.services(), pub.services(), tcpPort);

	return app.exec();
}
