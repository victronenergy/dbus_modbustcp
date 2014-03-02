#ifndef DEFINES_H
#define DEFINES_H

#if TARGET_ccgx
	#define DBUS_CONNECTION QDBusConnection::systemBus()
	#define DBUS_TYPE QDBusConnection::SystemBus
#else
	#define DBUS_CONNECTION QDBusConnection::sessionBus()
	#define DBUS_TYPE QDBusConnection::SessionBus
#endif

#endif // DEFINES_H
