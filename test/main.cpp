#include <QCoreApplication>
#include <QsLog.h>
#include <QTest>
#include "diagnostics_service_test.h"
#include "mapping_test.h"

int main(int argc, char *argv[])
{
	QsLogging::Logger &logger = QsLogging::Logger::instance();
	QsLogging::DestinationPtr debugDestination(
		QsLogging::DestinationFactory::MakeDebugOutputDestination());
	logger.addDestination(debugDestination);
	logger.setLoggingLevel(QsLogging::WarnLevel);

	QCoreApplication app(argc, argv);

	MappingTest mt;
	int r = QTest::qExec(&mt, argc, argv);
	DiagnosticsServiceTest dst;
	r += QTest::qExec(&dst, argc, argv);
	return r;
}
