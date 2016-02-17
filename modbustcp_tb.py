#!/usr/bin/python
from pymodbus.client.sync import ModbusTcpClient
from pymodbus.exceptions import ModbusException
import struct
import unittest

# This script will test various Modbus TCP operations on a CCGX. It expects a vebus device
# (Multi/Quattro) and a BMV to be connected to the CCGX.

#client_ip = '192.168.4.100'
#port = 502
client_ip = 'localhost'
port = 5020

class TestModbusTcp(unittest.TestCase):
    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)

    def setUp(self):
        self.client = ModbusTcpClient(client_ip, port=port)

    def tearDown(self):
        self.client.close()

    def _checked_read(self, address, length, unit=1):
        result = self.client.read_holding_registers(address, length, unit=unit)
        self.assertLess(result.function_code, 0x80)
        self.assertEqual(len(result.registers), length)
        return result.registers

    def _checked_write(self, address, value, unit=1):
        result = self.client.write_register(address, value, unit=unit)
        self.assertLess(result.function_code, 0x80)

    def test_1(self):
        # Test reading ve.bus address 33 until 36
        self._checked_read(3, 34, unit=246)

    def test_2to5(self):
        # Write multi /Mode and reset again
        r = self._checked_read(33, 1, unit=246)
        self._checked_write(33, 2, unit=246)
        r2 = self._checked_read(33, 1, unit=246)
        self._checked_write(33, r[0], unit=246)
        r3 = self._checked_read(33,1, unit=246)
        self.assertEqual(r2[0], 2)
        self.assertEqual(r[0], r3[0])

    def test_6(self):
        # Test function code 4, read_input_registers address 256 until 267
        self._checked_read(259, 2, unit=245)

    def test_7to9(self):
        # Test function code 16
        r = self._checked_read(33, 1, unit=246)
        result = self.client.write_registers(33, [2], unit=246)
        self.assertTrue(result.function_code < 0x80)
        r2 = self._checked_read(33, 1, unit=246)
        result = self.client.write_registers(33, [r[0]], unit=246)
        self.assertTrue(result.function_code < 0x80)
        r3 = self._checked_read(33, 1, unit=246)
        self.assertEqual(r2[0], 2)
        self.assertEqual(r[0], r3[0])

    def test_10(self):
        # Test function code 16, write_registers address 256 until 267"
        r = self._checked_read(22, 1, unit=246)
        result = self.client.write_registers(22, [r[0]/2], unit=246)
        self.assertLess(result.function_code, 0x80)
        self._checked_read(22, 1, unit=246)
        result = self.client.write_registers(22, r, unit=246)
        self.assertLess(result.function_code, 0x80)
        r2 = self._checked_read(22, 1, unit=246)
        self.assertEqual(r, r2)

    def test_11(self):
        # Test writing to ve.bus /State.
        result = self.client.write_register(31, 2, unit=246)
        self.assertGreaterEqual(result.function_code, 0x80)

    def test_12(self):
        # Test reading service without /DeviceInstance
        result = self._checked_read(2701, 1, unit=0)
        self.assertGreaterEqual(result[0], 0)
        self.assertLessEqual(result[0], 100)

    def test_13(self):
        # Test writing to service without /DeviceInstance
        prev = self._checked_read(2702, 1, unit=0)
        self._checked_write(2702, 55, unit=0)
        current = self._checked_read(2702, 1, unit=0)
        self.assertEqual(current[0], 55)
        self._checked_write(2702, prev[0], unit=0)
        current = self._checked_read(2702, 1, unit=0)
        self.assertEqual(current[0], prev[0])

    def test_14(self):
        # Test reading a string
        # Register 800 return the MAC address of the CCGX as string.
        result = self._checked_read(800, 6, unit=0)
        bytes = b''
        for r in result:
            bytes += chr(r >> 8)
            bytes += chr(r & 255)
        mac_address = int(struct.unpack('12s', bytes)[0], 16)

if __name__ == '__main__':
    unittest.main()
