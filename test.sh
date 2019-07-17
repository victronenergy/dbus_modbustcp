#!/bin/bash -e

mkdir -p build/test
cd build/test
qmake CXX=$CXX ../../test/modbustcp_test.pro && make && ./modbustcp_test
