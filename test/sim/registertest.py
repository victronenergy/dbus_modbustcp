import sys
import unittest
import logging
import os
import subprocess
import socket
from time import sleep

import gobject
from pymodbus.client.sync import ModbusTcpClient as ModbusClient

HOST='127.0.0.1'
PORT=20502

devnull = open(os.devnull, 'w')
here = os.path.dirname(__file__)

def isOpen(ip, port):
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	try:
		s.connect((ip, port))
		s.shutdown(2)
		return True
	except:
		return False

class Simulation(object):
	def __init__(self, fn):
		self.proc = subprocess.Popen([sys.executable,
			os.path.join(here, "simulate.py"), os.path.join(here, fn)],
			stdout=devnull, stderr=devnull)
		sleep(1) # Time to settle

	def __enter__(self):
		return self

	def __exit__(self, *args):
		self.proc.kill()
		self.proc.wait()

class RegisterTest(unittest.TestCase):
	def __init__(self, *args, **kwargs):
		self.procs = []
		super(RegisterTest, self).__init__(*args, **kwargs)

	def setUp(self):
		self.assertFalse(isOpen(HOST, PORT), "Port already in use")
		binary = os.environ.get('BIN', None) or os.path.join(here, "..", "..", "dbus-modbustcp")
		proc = subprocess.Popen(
			[binary, "-p", str(PORT), "--dbus", "session"], stdout=devnull, stderr=devnull)

		# Wait for it
		for i in xrange(10):
			sleep(0.1)
			if isOpen(HOST, PORT):
				break
		else:
			proc.kill(); proc.wait()
			self.assertTrue(False, "dbus-modbustcp did not start")

		self.procs.append(proc)
		self.modbus = ModbusClient(host=HOST, port=PORT)
		self.modbus.connect()

	def check_register(self, unit, register, size, value):
		r = self.modbus.read_holding_registers(register, size, unit=unit)
		self.assertEqual(r.registers, value)

	def test_system(self):
		with Simulation('system.csv'):
			self.check_register(0, 808, 1, [10])
			self.check_register(0, 809, 1, [20])
			self.check_register(0, 810, 1, [30])
			self.check_register(0, 811, 1, [110])
			self.check_register(0, 812, 1, [120])
			self.check_register(0, 813, 1, [130])
			self.check_register(0, 814, 1, [210])
			self.check_register(0, 815, 1, [220])
			self.check_register(0, 816, 1, [230])
			self.check_register(0, 817, 1, [310])
			self.check_register(0, 818, 1, [320])
			self.check_register(0, 819, 1, [330])
			self.check_register(0, 820, 1, [410])
			self.check_register(0, 821, 1, [420])
			self.check_register(0, 822, 1, [430])
			self.check_register(0, 823, 1, [510])
			self.check_register(0, 824, 1, [520])
			self.check_register(0, 825, 1, [530])
			self.check_register(0, 826, 1, [1])

			self.check_register(0, 840, 1, [500])
			self.check_register(0, 841, 1, [100])
			self.check_register(0, 842, 1, [600])
			self.check_register(0, 843, 1, [85])
			self.check_register(0, 844, 1, [1])
			self.check_register(0, 845, 1, [65316]) # -220
			self.check_register(0, 846, 1, [1])

			self.check_register(0, 850, 1, [300])
			self.check_register(0, 851, 1, [200])
			self.check_register(0, 855, 1, [10])
			self.check_register(0, 860, 1, [500])
			self.check_register(0, 865, 1, [220])
			self.check_register(0, 866, 1, [234])

			self.check_register(0, 806, 1, [1])
			self.check_register(0, 807, 1, [1])

			# serial number, bad beef
			self.check_register(0, 800, 6, [0x6261, 0x6462, 0x6565, 0x6631, 0x3562, 0x6164]) #ba, db, ee, f1, 5b, ad

	def test_vebus(self):
		with Simulation('vebus.csv'):
			# self.check_register(246, 1, 1, [0x11, 0x3e98]) # 1130136
			self.check_register(246, 3, 3, [2310, 2320, 2330])
			self.check_register(246, 6, 3, [10, 20, 30])
			self.check_register(246, 9, 3, [5000, 5000, 5000])
			self.check_register(246, 12, 3, [23, 46, 69])
			self.check_register(246, 15, 3, [2310, 2320, 2330])
			self.check_register(246, 18, 3, [10, 20, 30])
			self.check_register(246, 21, 1, [5000])
			self.check_register(246, 22, 1, [250])
			self.check_register(246, 23, 3, [23, 46, 69])
			self.check_register(246, 26, 2, [5000, 250])
			self.check_register(246, 28, 1, [3])
			self.check_register(246, 29, 1, [0])
			self.check_register(246, 30, 1, [850])
			self.check_register(246, 31, 3, [4, 17, 1])
			self.check_register(246, 56, 1, [1])

			# Alarms
			self.check_register(246, 34, 3, [1, 1, 1])
			self.check_register(246, 42, 14, 14*[1])
			self.check_register(246, 63, 1, [2])
			self.check_register(246, 64, 1, [2])

			# Setpoints
			self.check_register(246, 37, 5, [111, 1, 1, 222, 333])

			# VE.Bus BMS
			self.check_register(246, 57, 4, 4*[1])

			# Temperature
			self.check_register(246, 61, 1, [234])

			# Other mode3 registers
			self.check_register(246, 65, 4, [0, 10, 20, 30])

	def test_settings(self):
		with Simulation('settings.csv'):
			self.check_register(0, 2900, 3, [4, 250, 1])
			self.check_register(0, 2700, 4, [500, 100, 100, 5])
			self.check_register(0, 2704, 3, [80, 65535, 10]) # 65535 means -1
			self.check_register(0, 2707, 2, [1, 0])

	def test_battery(self):
		with Simulation('battery.csv'):
			self.check_register(247, 259, 2, [5200, 1200]) # Primary and secondary voltages
			self.check_register(247, 261, 2, [110, 345]) # amps, temperature
			self.check_register(247, 263, 2, [2600, 100]) # midpoint, deviation
			self.check_register(247, 265, 2, [64536, 850]) # AH (-10), SOC
			self.check_register(247, 280, 1, [1]) # Relay
			self.check_register(247, 303, 6, [1, 990, 565, 420, 1000, 500]) # TTG, SOH, Info
			self.check_register(247, 309, 1, [1000]) # Capacity, 
			self.check_register(247, 1282, 4, [9, 20, 1, 1]) # Capacity, Error, Switch, Balancing
			self.check_register(247, 1286, 6, [8, 2, 4, 4, 280, 400]) # System setup
			self.check_register(247, 318, 2, [50, 500]) # Cell temperature limits

			# Alarms
			self.check_register(247, 267, 13, 13*[1])
			self.check_register(247, 324, 3, 3*[1])

			# IO
			self.check_register(247, 1297, 3, [1, 1, 1])

	def test_solarcharger(self):
		with Simulation('solarcharger.csv'):
			self.check_register(245, 771, 5, [4800, 100, 230, 1, 3])
			self.check_register(245, 776, 2, [9600, 50])
			self.check_register(245, 788, 1, [1])
			self.check_register(245, 789, 1, [4800])
			self.check_register(245, 778, 3, [1, 3000, 1])
			self.check_register(245, 781, 3, [2, 2, 2])
			self.check_register(245, 784, 4, [100, 20, 300, 40])
			self.check_register(245, 790, 1, [500])
			self.check_register(245, 791, 1, [1])

	def test_grid(self):
		with Simulation('grid.csv'):
			self.check_register(30, 2600, 3, [100, 200, 300])
			self.check_register(30, 2609, 7, [0x3132, 0x3334, 0x3536, 0x3700, 0, 0, 0])
			self.check_register(30, 2616, 6, [2310, 10, 2320, 20, 2330, 30])
			self.check_register(30, 2603, 6, [1100, 1200, 1300, 2100, 2200, 2300])
			self.check_register(30, 2622, 12, [0, 1100, 0, 1200, 0, 1300, 0, 2100, 0, 2200, 0, 2300])
	
	def test_gps(self):
		with Simulation('gps.csv'):
			self.check_register(1, 2800, 10,
				[60334, 52072, 2870, 20480, 17900, 34300, 1, 5, 0, 12])

	def test_none_existent(self):
		with self.assertRaises(AttributeError):
			self.check_register(0, 1, 1, [])

	def tearDown(self):
		for proc in self.procs:
			proc.kill()
			proc.wait()

if __name__ == "__main__":
	logging.basicConfig(stream=sys.stderr)
	logging.getLogger('').setLevel(logging.WARNING)
	unittest.main()
