#include "busitem_interface.h"

BusItemInterface::BusItemInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent) :
	QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
{
}
