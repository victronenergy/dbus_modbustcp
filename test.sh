#!/bin/bash

mkdir -p build/test
cd build/test
qmake CXX=$CXX ../../test/modbustcp_test.pro && make && ./modbustcp_test
if [[ $? -ne 0 ]] ; then
    exit 1
fi
