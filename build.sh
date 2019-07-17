#!/bin/bash -e

mkdir -p build/dbus-modbustcp
cd build/dbus-modbustcp
qmake CXX=$CXX ../../dbus_modbustcp.pro && make
