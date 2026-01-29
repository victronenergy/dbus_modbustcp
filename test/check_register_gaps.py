#!/usr/bin/env python3
"""
Static analysis of attributes.csv for register gaps.
No Modbus connection or running system needed.
"""
import argparse
import csv
import json
import re
import sys

# ANSI color codes
RED = '\033[91m'
GREEN = '\033[92m'
YELLOW = '\033[93m'
RESET = '\033[0m'

parser = argparse.ArgumentParser(
    description='Check attributes.csv for register gaps',
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog="""
Examples:
  Basic gap check (fails if any gaps found):
    %(prog)s /path/to/attributes.csv

  Show detailed context for each gap:
    %(prog)s --verbose

  Generate a baseline of current gaps (for accepting existing gaps):
    %(prog)s --generate-baseline > baseline.json

  Check against baseline (fails only on NEW gaps):
    %(prog)s --baseline baseline.json

  Typical CI workflow:
    1. First time, generate baseline:
       %(prog)s --generate-baseline > baseline.json
    2. Commit baseline.json to repo
    3. CI runs:
       %(prog)s --baseline baseline.json
    4. If intentional changes are made, regenerate baseline

Exit codes:
  0 - No gaps found (or all gaps in baseline)
  1 - New gaps found (not in baseline)
""")

parser.add_argument('csv_path', nargs='?',
                    default='/opt/victronenergy/dbus-modbustcp/attributes.csv',
                    help='Path to attributes.csv (default: /opt/victronenergy/dbus-modbustcp/attributes.csv)')
parser.add_argument('-v', '--verbose', action='store_true',
                    help='Show extra context for gaps (attributes before/after)')
parser.add_argument('--generate-baseline', action='store_true',
                    help='Output current gaps as JSON baseline to stdout')
parser.add_argument('--baseline', type=str, metavar='FILE',
                    help='Compare against baseline JSON file; only fail on new gaps')
args = parser.parse_args()


def getRegistersCount(modbusType):
    """Determine how many registers a modbus type spans."""
    if modbusType.startswith('string['):
        temp = re.findall(r'\d+', modbusType)
        if len(temp) == 1:
            return int(temp[0])
    if modbusType.endswith('int32'):
        return 2
    else:
        return 1


def findGaps(csvPath):
    """Analyze attributes.csv and return gaps per service type."""
    try:
        with open(csvPath, newline='', encoding='utf_8') as f:
            reader = csv.reader(f)
            attributes = [row for row in reader
                         if not any('INTERNAL' in cell or 'RESERVED' in cell for cell in row)]
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

    # Find gaps for each service type
    allGaps = {}
    for serviceType in sorted(serviceAttrs.keys()):
        attrs = serviceAttrs[serviceType]
        attrs.sort(key=lambda x: x['start'])

        minReg = min(a['start'] for a in attrs)
        maxReg = max(a['end'] for a in attrs)

        # Build set of all defined registers
        definedRegisters = set()
        for attr in attrs:
            for r in range(attr['start'], attr['end'] + 1):
                definedRegisters.add(r)

        # Find gaps
        gaps = []
        for r in range(minReg, maxReg + 1):
            if r not in definedRegisters:
                gaps.append(r)

        if gaps:
            # Group consecutive gaps into ranges
            gapRanges = []
            gapStart = gaps[0]
            gapEnd = gaps[0]

            for g in gaps[1:]:
                if g == gapEnd + 1:
                    gapEnd = g
                else:
                    gapRanges.append([gapStart, gapEnd])
                    gapStart = g
                    gapEnd = g
            gapRanges.append([gapStart, gapEnd])

            allGaps[serviceType] = {
                'ranges': gapRanges,
                'attrs': attrs,
                'minReg': minReg,
                'maxReg': maxReg
            }
        else:
            allGaps[serviceType] = {
                'ranges': [],
                'attrs': attrs,
                'minReg': minReg,
                'maxReg': maxReg
            }

    return allGaps


def loadBaseline(baselinePath):
    """Load baseline JSON file."""
    try:
        with open(baselinePath, 'r') as f:
            return json.load(f)
    except IOError:
        raise SystemExit(f"{RED}### Failed to read baseline file: {baselinePath}{RESET}")
    except json.JSONDecodeError as e:
        raise SystemExit(f"{RED}### Invalid JSON in baseline file: {e}{RESET}")


def generateBaseline(allGaps):
    """Generate baseline JSON from current gaps."""
    baseline = {}
    for serviceType, data in allGaps.items():
        if data['ranges']:
            baseline[serviceType] = data['ranges']
    return baseline


def printGapContext(attrs, gStart, gEnd):
    """Print the attributes before and after a gap."""
    # Find attribute before gap
    beforeAttr = None
    for attr in attrs:
        if attr['end'] < gStart:
            beforeAttr = attr
        else:
            break

    # Find attribute after gap
    afterAttr = None
    for attr in attrs:
        if attr['start'] > gEnd:
            afterAttr = attr
            break

    if beforeAttr:
        print(f"        Before: {beforeAttr['end']} {beforeAttr['path']} ({beforeAttr['type']})")
    if afterAttr:
        print(f"        After:  {afterAttr['start']} {afterAttr['path']} ({afterAttr['type']})")


def checkCsvForGaps(csvPath, baseline=None):
    """Check CSV for gaps, optionally comparing against baseline."""
    if not args.generate_baseline:
        print(f"### Checking {csvPath} for register gaps\n")

    allGaps = findGaps(csvPath)

    # Generate baseline mode
    if args.generate_baseline:
        baselineData = generateBaseline(allGaps)
        print(json.dumps(baselineData, indent=2))
        return 0

    newGapCount = 0
    knownGapCount = 0

    for serviceType in sorted(allGaps.keys()):
        data = allGaps[serviceType]
        gapRanges = data['ranges']

        if not gapRanges:
            print(f"### {serviceType}: registers {data['minReg']}-{data['maxReg']} - {GREEN}No gaps{RESET}")
            continue

        # Check which gaps are new vs known
        baselineRanges = []
        if baseline and serviceType in baseline:
            baselineRanges = [tuple(r) for r in baseline[serviceType]]

        newRanges = []
        knownRanges = []
        for gapRange in gapRanges:
            if tuple(gapRange) in baselineRanges:
                knownRanges.append(gapRange)
            else:
                newRanges.append(gapRange)

        print(f"### {serviceType}: registers {data['minReg']}-{data['maxReg']}")

        if knownRanges:
            knownGapCount += len(knownRanges)
            print(f"    {YELLOW}Known gaps ({len(knownRanges)} ranges):{RESET}")
            for gStart, gEnd in knownRanges:
                if gStart == gEnd:
                    print(f"      {YELLOW}- Register {gStart}{RESET}")
                else:
                    print(f"      {YELLOW}- Registers {gStart}-{gEnd}{RESET}")
                if args.verbose:
                    printGapContext(data['attrs'], gStart, gEnd)

        if newRanges:
            newGapCount += len(newRanges)
            print(f"    {RED}New gaps ({len(newRanges)} ranges):{RESET}")
            for gStart, gEnd in newRanges:
                if gStart == gEnd:
                    print(f"      {RED}- Register {gStart}{RESET}")
                else:
                    print(f"      {RED}- Registers {gStart}-{gEnd}{RESET}")
                if args.verbose:
                    printGapContext(data['attrs'], gStart, gEnd)

    # Summary
    print()
    if baseline:
        if newGapCount > 0:
            print(f"### Summary: {RED}{newGapCount} new gap(s){RESET}, {YELLOW}{knownGapCount} known gap(s){RESET}")
        else:
            print(f"### Summary: {GREEN}No new gaps{RESET}, {YELLOW}{knownGapCount} known gap(s){RESET}")
    else:
        totalGaps = newGapCount + knownGapCount
        if totalGaps > 0:
            print(f"### Summary: {RED}{totalGaps} service(s) with gaps{RESET}")
        else:
            print(f"### Summary: {GREEN}No gaps found{RESET}")

    return newGapCount


if __name__ == '__main__':
    baseline = None
    if args.baseline:
        baseline = loadBaseline(args.baseline)

    newGaps = checkCsvForGaps(args.csv_path, baseline)
    sys.exit(1 if newGaps > 0 else 0)
