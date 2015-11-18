#ifndef DBUS_SERVICE_H
#define DBUS_SERVICE_H

#include <QObject>
#include <QString>
#include <velib/qt/ve_qitem.hpp>

class DBusService : public QObject
{
	Q_OBJECT
public:
	DBusService(VeQItem *serviceRoot, QObject *parent = 0);
	QString getServiceName() const;
	int getDeviceInstance() { return mDeviceInstance->getValue().toInt(); }

	bool getConnected() { return mConnected; }
	void setConnected(bool connected) { mConnected = connected; }
	void registerObjects(const QStringList &pathList);
	void registerObject(const QString &path);
	QPair<QVariant, bool> getValue(const QString &path) const;
	bool setValue(const QString &path, const QVariant &value);
	static QString getDeviceType(const QString &serviceName);

private:
	void setDeviceType(const QString &name);

	VeQItem *mServiceRoot;
	QString mServiceType;
	bool mConnected;
	VeQItem *mDeviceInstance;
};

#endif
