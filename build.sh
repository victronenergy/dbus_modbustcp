#!/bin/bash

mkdir -p build/dbus-modbustcp
cd build/dbus-modbustcp
qmake CXX=$CXX ../../dbus_modbustcp.pro && make
if [[ $? -ne 0 ]] ; then
    exit 1
fi
