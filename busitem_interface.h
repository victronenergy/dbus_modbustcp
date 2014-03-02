#ifndef BUSITEMINTERFACE_H
#define BUSITEMINTERFACE_H

#include <QObject>
#include <QtDBus/QtDBus>

class BusItemInterface : public QDBusAbstractInterface
{
	Q_OBJECT
public:
	static inline const char *staticInterfaceName()
	{ return "com.victronenergy.BusItem"; }

	BusItemInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

public slots:
	QVariant getValue() {
		QDBusReply<QVariant> reply = call("GetValue");
		return reply.value();
	 }

	QDBusReply<int> setValue(const QVariant & value)
	{
		return call("SetValue", value);
	}

signals:
	 void PropertiesChanged(const QVariantMap &changes);

private:

};

#endif // BUSITEMINTERFACE_H
