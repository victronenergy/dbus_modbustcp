#!/usr/bin/env python3
"""
Static analysis of attributes.csv for column validity.
No Modbus connection or running system needed.
"""
import argparse
import csv
import sys

# ANSI color codes
RED = '\033[91m'
GREEN = '\033[92m'
RESET = '\033[0m'

parser = argparse.ArgumentParser(
    description='Check attributes.csv for column validity',
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog="""
Examples:
  Basic validity check (fails if any errors found):
    %(prog)s /path/to/attributes.csv

Exit codes:
  0 - No errors found
  1 - Errors found
""")

parser.add_argument('csv_path', nargs='?',
                    default='/opt/victronenergy/dbus-modbustcp/attributes.csv',
                    help='Path to attributes.csv (default: /opt/victronenergy/dbus-modbustcp/attributes.csv)')
args = parser.parse_args()

# D-Bus basic types from the D-Bus specification
DBUS_BASIC_TYPES = set('ybnqiuxtdhsog')

# Valid modbus data types (plain or with a bracketed size)
MODBUS_PLAIN_TYPES = {'uint16', 'int16', 'uint32', 'int32'}
MODBUS_SIZED_PREFIXES = ('string[', 'reserved[', 'RESERVED[', 'INTERNAL[')
MODBUS_PLAIN_RESERVED = {'RESERVED', 'reserved'}


def isValidModbusType(value):
    if value in MODBUS_PLAIN_TYPES or value in MODBUS_PLAIN_RESERVED:
        return True
    for prefix in MODBUS_SIZED_PREFIXES:
        if value.startswith(prefix) and value.endswith(']'):
            inner = value[len(prefix):-1]
            return inner.isdigit()
    return False


def checkRow(lineNum, row):
    """Validate a single CSV row. Returns list of error strings."""
    errors = []

    if len(row) != 8:
        errors.append(f"expected 8 columns, got {len(row)}")
        return errors  # Further checks would index out of bounds

    service, path, dbusType, _unit, register, modbusType, scale, rw = row

    if not service.startswith('com.victronenergy.'):
        errors.append(f"service '{service}' does not start with 'com.victronenergy.'")

    if not path.startswith('/') and path != 'RESERVED':
        errors.append(f"D-Bus path '{path}' does not start with '/'")

    if len(dbusType) != 1 or dbusType not in DBUS_BASIC_TYPES:
        errors.append(f"D-Bus type '{dbusType}' is not a valid basic type")

    if not register.strip().isdigit():
        errors.append(f"register '{register}' is not a positive integer")

    if not isValidModbusType(modbusType):
        errors.append(f"modbus type '{modbusType}' is not a recognised type")

    if scale.strip():
        try:
            float(scale)
        except ValueError:
            errors.append(f"scale '{scale}' is not a number")

    if rw not in ('R', 'W'):
        errors.append(f"R/W column '{rw}' is not 'R' or 'W'")

    return errors


def checkCsvValidity(csvPath):
    """Check CSV for column validity."""
    print(f"### Checking {csvPath} for column validity\n")

    try:
        with open(csvPath, newline='', encoding='utf_8') as f:
            reader = csv.reader(f)
            rows = list(reader)
    except IOError:
        raise SystemExit(f"{RED}### Failed to read {csvPath}{RESET}")

    errorCount = 0

    for lineNum, row in enumerate(rows, start=1):
        errors = checkRow(lineNum, row)
        if errors:
            errorCount += len(errors)
            for err in errors:
                print(f"{RED}  Line {lineNum}: {err}{RESET}")
                if len(row) > 1:
                    print(f"    Row: {','.join(row)}")

    print()
    if errorCount > 0:
        print(f"### Summary: {RED}{errorCount} error(s) found{RESET}")
    else:
        print(f"### Summary: {GREEN}No errors found{RESET}")

    return errorCount


if __name__ == '__main__':
    errors = checkCsvValidity(args.csv_path)
    sys.exit(1 if errors > 0 else 0)
