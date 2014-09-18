#!/usr/bin/python
from pymodbus.client.sync import ModbusTcpClient
from pymodbus.exceptions import ModbusException

client = ModbusTcpClient('192.168.34.10')

# Test reading ve.bus address 33 until 36
print "1: Test function code 3, read_holding_registers ve.bus address 33 until 36"
try:
  result = client.read_holding_registers(3,34, unit=246)
except ModbusException:
  print "\tFail"
else:
  print "\tPass",result.registers

# Keep for later
mode = result.registers[30]

# Test writing to ve.bus /Mode
print "2: Test writing 2 to ve.bus /Mode"
try:
  result = client.write_register(33, 2, unit=246)
except ModbusException:
  print "\tFail"
else:
  print "\tPass"

# Test readback ve.bus /Mode
print "3: Test readback ve.bus /Mode"
try:
  result = client.read_holding_registers(33,1, unit=246)
except ModbusException:
  print "\tFail"
else:
  print "\tPass", result.registers

# Test write back old value to ve.bus /Mode
print "4: Test write back old value to ve.bus /Mode"
try:
  result = client.write_register(33, mode, unit=246)
except ModbusException:
  print "\tFail"
else:
  print "\tPass"

# TEST 5
print "5: Test readback ve.bus /Mode"
try:
  result = client.read_holding_registers(33,1, unit=246)
except ModbusException:
  print "\tFail"
else:
  print "\tPass",result.registers

# TEST 6
print "6: Test function code 4, read_input_registers address 256 until 267"
try:
  result = client.read_input_registers(259,2,unit=245)
except Exception:
  print "\tFail",result.registers
else:
  print "\tPass",result.registers

# TEST 7
print "7: Test function code 16, write_registers address 256 until 267"
try:
  result = client.write_registers(33,[2]*1,unit=246)
except ModbusException:
  print "\tFail"
else:
  print "\tPass"

# TEST 8
print "8: Test readback ve.bus /Mode"
try:
  result = client.read_holding_registers(33,1, unit=246)
except ModbusException:
  print "\tFail"
else:
  print "\tPass", result.registers

# TEST 9
print "9: Test function code 16, write_registers address 256 until 267"
try:
  result = client.write_registers(33,[mode]*1,unit=246)
except ModbusException:
  print "\tFail"
else:
  print "\tPass"

# TEST 10
print "10: Test function code 16, write_registers address 256 until 267"
try:
  result = client.write_registers(22,[100]*1,unit=246)
except ModbusException:
  print "\tFail"
else:
  print "\tPass"

# TEST 11
print "11: Test writing to ve.bus /State."
try:
  result = client.write_register(31, 2, unit=246)
except:
#ModbusException:
  print "\tPass"
  pass
else:
  print "\tFail"

client.close()

