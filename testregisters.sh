#!/bin/bash -e

# First build the binary
./build.sh
cp attributes.csv unitid2di.csv build/dbus-modbustcp

# Then run a simulation test
cd test/sim
BIN=../../build/dbus-modbustcp/dbus-modbustcp dbus-launch python registertest.py
