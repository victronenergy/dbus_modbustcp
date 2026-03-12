#!/usr/bin/env python3
"""
Static analysis of attributes.csv for register overlaps.
No Modbus connection or running system needed.
"""
import argparse
import csv
import re
import sys

# ANSI color codes
RED = '\033[91m'
GREEN = '\033[92m'
RESET = '\033[0m'

parser = argparse.ArgumentParser(
    description='Check attributes.csv for register overlaps',
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog="""
Examples:
  Basic overlap check (fails if any overlaps found):
    %(prog)s /path/to/attributes.csv

  Show detailed context for each overlap:
    %(prog)s --verbose

Exit codes:
  0 - No overlaps found
  1 - Overlaps found
""")

parser.add_argument('csv_path', nargs='?',
                    default='/opt/victronenergy/dbus-modbustcp/attributes.csv',
                    help='Path to attributes.csv (default: /opt/victronenergy/dbus-modbustcp/attributes.csv)')
parser.add_argument('-v', '--verbose', action='store_true',
                    help='Show details of each overlapping pair')
args = parser.parse_args()


def getRegistersCount(modbusType):
    """Determine how many registers a modbus type spans."""
    if modbusType.startswith('string[') or modbusType.startswith('reserved[') or modbusType.startswith('INTERNAL['):
        temp = re.findall(r'\d+', modbusType)
        if len(temp) == 1:
            return int(temp[0])
    if modbusType.endswith('int32'):
        return 2
    else:
        return 1


def findOverlaps(csvPath):
    """Analyze attributes.csv and return overlaps per service type."""
    try:
        with open(csvPath, newline='', encoding='utf_8') as f:
            reader = csv.reader(f)
            attributes = [row for row in reader]
    except IOError:
        raise SystemExit(f"{RED}### Failed to read {csvPath}{RESET}")

    # Group attributes by service type
    serviceAttrs = {}
    for attr in attributes:
        try:
            serviceType = attr[0]
            register = int(attr[4])
            regCount = getRegistersCount(attr[5])

            if serviceType not in serviceAttrs:
                serviceAttrs[serviceType] = []

            serviceAttrs[serviceType].append({
                'start': register,
                'end': register + regCount - 1,
                'path': attr[1],
                'type': attr[5]
            })
        except (ValueError, IndexError):
            continue

    # Find overlaps for each service type
    allOverlaps = {}
    for serviceType in sorted(serviceAttrs.keys()):
        attrs = serviceAttrs[serviceType]
        attrs.sort(key=lambda x: x['start'])

        overlaps = []
        for i in range(len(attrs) - 1):
            a = attrs[i]
            b = attrs[i + 1]
            if a['end'] >= b['start']:
                overlaps.append((a, b))

        allOverlaps[serviceType] = {
            'overlaps': overlaps,
            'attrs': attrs,
        }

    return allOverlaps


def checkCsvForOverlaps(csvPath):
    """Check CSV for overlaps."""
    print(f"### Checking {csvPath} for register overlaps\n")

    allOverlaps = findOverlaps(csvPath)

    overlapCount = 0

    for serviceType in sorted(allOverlaps.keys()):
        data = allOverlaps[serviceType]
        overlaps = data['overlaps']

        if not overlaps:
            continue

        overlapCount += len(overlaps)
        print(f"### {serviceType}: {RED}{len(overlaps)} overlap(s){RESET}")

        for a, b in overlaps:
            print(f"    {RED}- Registers {a['start']}-{a['end']} overlaps {b['start']}-{b['end']}{RESET}")
            if args.verbose:
                print(f"        Entry A: {a['start']}-{a['end']} {a['path']} ({a['type']})")
                print(f"        Entry B: {b['start']}-{b['end']} {b['path']} ({b['type']})")

    print()
    if overlapCount > 0:
        print(f"### Summary: {RED}{overlapCount} overlap(s) found{RESET}")
    else:
        print(f"### Summary: {GREEN}No overlaps found{RESET}")

    return overlapCount


if __name__ == '__main__':
    overlaps = checkCsvForOverlaps(args.csv_path)
    sys.exit(1 if overlaps > 0 else 0)
