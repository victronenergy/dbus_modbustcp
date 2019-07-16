InterfaceBusItem = 'com.victronenergy.BusItem'

import sys
import os
from csv import reader as csvreader
from dbus.mainloop.glib import DBusGMainLoop
import dbus
import dbus.service
import gobject
import pickle
import logging
from collections import defaultdict
from functools import total_ordering
from itertools import izip, repeat
import heapq
from argparse import ArgumentParser

def wrap(t, v):
	de = {
		"i": dbus.types.Int32,
		"u": dbus.types.UInt32,
		"n": dbus.types.Int16,
		"q": dbus.types.UInt16,
		"d": dbus.types.Double,
		"s": dbus.types.String,
		"y": lambda x: dbus.types.Byte(int(x)),
	}
	return de[t](v)

class SessionBus(dbus.bus.BusConnection):
	def __new__(cls):
		return dbus.bus.BusConnection.__new__(cls, dbus.bus.BusConnection.TYPE_SESSION)

class DbusRootObject(dbus.service.Object):
	def __init__(self, busName, values):
		super(DbusRootObject, self).__init__(busName, '/')
		self.values = values

	@dbus.service.method(InterfaceBusItem, out_signature = 'v')
	def GetValue(self):
		values = { k[1:]: v.value for k, v in self.values.items() }
		return dbus.Dictionary(values, signature=dbus.Signature('sv'),
			variant_level=1)

class DbusPathObject(dbus.service.Object):
	def __init__(self, busName, objectPath, value):
		super(DbusPathObject, self).__init__(busName, objectPath)
		self._objectPath = objectPath
		self.value = value

	@dbus.service.method(InterfaceBusItem, out_signature = 'v')
	def GetValue(self):
		return self.value

def open_csv(bus, fn):
	reader = csvreader(open(fn, 'rb'))
	service = {}
	name = reader.next()[0]
	busName = dbus.service.BusName(name, bus)
	for row in reader:
		path, typ, value = row[:3]
		if not value.strip():
			# Invalid
			value = dbus.Array([], signature=dbus.Signature('u'), variant_level=1)
		service[path] = DbusPathObject(busName, path, wrap(typ, value))
	return DbusRootObject(busName, service)


def simulate(*files):
	return [open_csv(SessionBus(), fn) for fn in files]

def main():
	DBusGMainLoop(set_as_default=True)
	roots = simulate(*sys.argv[1:])
	gobject.MainLoop().run()

if __name__ == "__main__":
	main()
