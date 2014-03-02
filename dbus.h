#ifndef DBUS_H
#define DBUS_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusServiceWatcher>

class dbus : public QObject
{
	Q_OBJECT
public:
	explicit dbus(QObject *parent = 0);

signals:

public slots:

private slots:
	void serviceRegisterd(const QString serviceName);
	void serviceUnregisterd(const QString serviceName);
	void serviceOwnerChanged(const QString & name, const QString & oldOwner, const QString & newOwner);

private:
	QDBusConnection mDBus;
	QDBusServiceWatcher mDBusWatcher;
};

#endif // DBUS_H
