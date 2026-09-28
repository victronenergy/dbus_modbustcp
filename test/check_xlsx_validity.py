#!/usr/bin/env python3
"""
Static analysis of an xlsx file for structural validity.
Catches problems that Excel rejects as corruption but LibreOffice silently
accepts, such as font names longer than 31 characters.
No MS Office needed.
"""
import argparse
import posixpath
import re
import sys
import xml.etree.ElementTree as ET
import zipfile

# ANSI color codes
RED = '\033[91m'
GREEN = '\033[92m'
RESET = '\033[0m'

parser = argparse.ArgumentParser(
    description='Check an xlsx file for structural validity',
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog="""
Examples:
  Basic validity check (fails if any errors found):
    %(prog)s CCGX-Modbus-TCP-register-list.xlsx

Exit codes:
  0 - No errors found
  1 - Errors found
""")

parser.add_argument('xlsx_path', nargs='?',
                    default='CCGX-Modbus-TCP-register-list.xlsx',
                    help='Path to the xlsx file (default: CCGX-Modbus-TCP-register-list.xlsx)')
args = parser.parse_args()

NS_MAIN = 'http://schemas.openxmlformats.org/spreadsheetml/2006/main'
NS_REL = 'http://schemas.openxmlformats.org/officeDocument/2006/relationships'
NS_PKG_REL = 'http://schemas.openxmlformats.org/package/2006/relationships'
NS_CT = 'http://schemas.openxmlformats.org/package/2006/content-types'

# Excel limits
MAX_FONT_NAME = 31
MAX_SHEET_NAME = 31
MAX_CELL_TEXT = 32767
MAX_ROWS = 1048576
MAX_COLS = 16384
FIRST_CUSTOM_NUMFMT = 164
INVALID_SHEET_CHARS = set('[]:*?/\\')

# Stop reporting after this many errors per part, one fault tends to repeat
MAX_ERRORS_PER_PART = 20


def m(tag):
    return f'{{{NS_MAIN}}}{tag}'


def col2num(letters):
    n = 0
    for ch in letters:
        n = n * 26 + ord(ch) - ord('A') + 1
    return n


def textOf(elem):
    """Concatenated text of all <t> descendants (shared strings, inline strings)."""
    return ''.join(t.text or '' for t in elem.iter(m('t')))


def parsePart(zf, name, errors):
    try:
        return ET.fromstring(zf.read(name))
    except KeyError:
        errors.append((name, 'part is missing'))
    except ET.ParseError as e:
        errors.append((name, f'not well-formed XML: {e}'))
    return None


def relsPathFor(partName):
    """Path of the .rels file belonging to a part ('' means the package root)."""
    base, fname = posixpath.split(partName)
    return posixpath.join(base, '_rels', fname + '.rels')


def readRels(zf, partName, errors):
    """Return {rId: (target part name, is external)} for a part's relationships."""
    relsName = relsPathFor(partName)
    if relsName not in zf.namelist():
        return {}
    root = parsePart(zf, relsName, errors)
    if root is None:
        return {}
    baseDir = posixpath.dirname(partName)
    rels = {}
    for rel in root.findall(f'{{{NS_PKG_REL}}}Relationship'):
        target = rel.get('Target', '')
        external = rel.get('TargetMode') == 'External'
        if not external:
            if target.startswith('/'):
                target = target[1:]
            else:
                target = posixpath.normpath(posixpath.join(baseDir, target))
        rels[rel.get('Id')] = (target, external)
    return rels


def checkPackage(zf):
    errors = []
    names = [n for n in zf.namelist() if not n.endswith('/')]

    for name in names:
        if name.endswith('.xml') or name.endswith('.rels'):
            parsePart(zf, name, errors)

    ct = parsePart(zf, '[Content_Types].xml', errors)
    if ct is not None:
        overrides = {o.get('PartName', '').lstrip('/')
                     for o in ct.findall(f'{{{NS_CT}}}Override')}
        defaults = {d.get('Extension', '').lower()
                    for d in ct.findall(f'{{{NS_CT}}}Default')}
        for name in names:
            if name == '[Content_Types].xml':
                continue
            ext = name.rsplit('.', 1)[-1].lower() if '.' in name else ''
            if name not in overrides and ext not in defaults:
                errors.append(('[Content_Types].xml', f'no content type for part {name}'))
        for name in overrides:
            if name not in names:
                errors.append(('[Content_Types].xml', f'override for missing part {name}'))

    for relsName in names:
        if not relsName.endswith('.rels'):
            continue
        # Map the .rels file back to its source part
        relsDir, relsFile = posixpath.split(relsName)
        sourcePart = posixpath.join(posixpath.dirname(relsDir), relsFile[:-len('.rels')])
        for rId, (target, external) in readRels(zf, sourcePart, errors).items():
            if not external and target not in names:
                errors.append((relsName, f'{rId} points to missing part {target}'))

    return errors


def checkWorkbook(zf):
    """Returns (errors, [(sheet name, part name)])."""
    errors = []
    sheets = []
    wb = parsePart(zf, 'xl/workbook.xml', errors)
    if wb is None:
        return errors, sheets
    rels = readRels(zf, 'xl/workbook.xml', errors)
    seen = set()
    for sheet in wb.iter(m('sheet')):
        name = sheet.get('name', '')
        if len(name) > MAX_SHEET_NAME:
            errors.append(('xl/workbook.xml',
                           f"sheet name '{name}' is {len(name)} chars, Excel allows {MAX_SHEET_NAME}"))
        bad = INVALID_SHEET_CHARS.intersection(name)
        if bad:
            errors.append(('xl/workbook.xml',
                           f"sheet name '{name}' contains invalid characters {''.join(sorted(bad))}"))
        if name.lower() in seen:
            errors.append(('xl/workbook.xml', f"duplicate sheet name '{name}'"))
        seen.add(name.lower())
        rId = sheet.get(f'{{{NS_REL}}}id')
        if rId in rels:
            sheets.append((name, rels[rId][0]))
        else:
            errors.append(('xl/workbook.xml', f"sheet '{name}' refers to unknown relationship {rId}"))
    return errors, sheets


def checkCount(part, container, childTag, errors):
    if container is None or container.get('count') is None:
        return
    actual = len(container.findall(m(childTag)))
    if int(container.get('count')) != actual:
        errors.append((part, f"<{container.tag.split('}')[1]}> count={container.get('count')} "
                             f"but has {actual} entries"))


def checkStyles(zf):
    """Returns (errors, number of cellXfs)."""
    part = 'xl/styles.xml'
    errors = []
    root = parsePart(zf, part, errors)
    if root is None:
        return errors, 0

    sections = {}
    for tag, child in (('numFmts', 'numFmt'), ('fonts', 'font'), ('fills', 'fill'),
                       ('borders', 'border'), ('cellStyleXfs', 'xf'),
                       ('cellXfs', 'xf'), ('cellStyles', 'cellStyle')):
        container = root.find(m(tag))
        checkCount(part, container, child, errors)
        sections[tag] = container.findall(m(child)) if container is not None else []

    for i, font in enumerate(sections['fonts']):
        name = font.find(m('name'))
        if name is not None and len(name.get('val', '')) > MAX_FONT_NAME:
            errors.append((part, f"font {i} name '{name.get('val')}' is {len(name.get('val'))} "
                                 f"chars, Excel allows {MAX_FONT_NAME}"))

    numFmtIds = {int(f.get('numFmtId')) for f in sections['numFmts']}
    nFonts = len(sections['fonts'])
    nFills = len(sections['fills'])
    nBorders = len(sections['borders'])
    nStyleXfs = len(sections['cellStyleXfs'])

    for tag in ('cellStyleXfs', 'cellXfs'):
        for i, xf in enumerate(sections[tag]):
            for attr, limit in (('fontId', nFonts), ('fillId', nFills), ('borderId', nBorders)):
                if int(xf.get(attr, 0)) >= limit:
                    errors.append((part, f'{tag}[{i}] {attr}={xf.get(attr)} but only {limit} defined'))
            numFmtId = int(xf.get('numFmtId', 0))
            if numFmtId >= FIRST_CUSTOM_NUMFMT and numFmtId not in numFmtIds:
                errors.append((part, f'{tag}[{i}] numFmtId={numFmtId} is not defined in numFmts'))
            if tag == 'cellXfs' and int(xf.get('xfId', 0)) >= nStyleXfs:
                errors.append((part, f'cellXfs[{i}] xfId={xf.get("xfId")} but only '
                                     f'{nStyleXfs} cellStyleXfs defined'))

    for style in sections['cellStyles']:
        if int(style.get('xfId', 0)) >= nStyleXfs:
            errors.append((part, f"cellStyle '{style.get('name')}' xfId={style.get('xfId')} "
                                 f"but only {nStyleXfs} cellStyleXfs defined"))

    return errors, len(sections['cellXfs'])


def checkSharedStrings(zf):
    """Returns (errors, number of shared strings)."""
    part = 'xl/sharedStrings.xml'
    errors = []
    if part not in zf.namelist():
        return errors, 0
    root = parsePart(zf, part, errors)
    if root is None:
        return errors, 0
    sis = root.findall(m('si'))
    if root.get('uniqueCount') is not None and int(root.get('uniqueCount')) != len(sis):
        errors.append((part, f"uniqueCount={root.get('uniqueCount')} but has {len(sis)} entries"))
    for i, si in enumerate(sis):
        length = len(textOf(si))
        if length > MAX_CELL_TEXT:
            errors.append((part, f'string {i} is {length} chars, Excel allows {MAX_CELL_TEXT}'))
    return errors, len(sis)


def checkSheet(zf, sheetName, part, nCellXfs, nStrings):
    label = f"{part} ('{sheetName}')"
    errors = []
    root = parsePart(zf, part, errors)
    if root is None:
        return errors

    cols = root.find(m('cols'))
    if cols is not None:
        prevMax = 0
        for col in cols.findall(m('col')):
            lo, hi = int(col.get('min')), int(col.get('max'))
            if lo > hi or lo <= prevMax or hi > MAX_COLS:
                errors.append((label, f'<col min={lo} max={hi}> is out of order, overlapping or out of range'))
            prevMax = max(prevMax, hi)
            if int(col.get('style', 0)) >= nCellXfs:
                errors.append((label, f'<col min={lo} max={hi}> style={col.get("style")} '
                                      f'but only {nCellXfs} cellXfs defined'))

    prevRow = 0
    for row in root.iter(m('row')):
        rowNum = int(row.get('r', prevRow + 1))
        if rowNum <= prevRow or rowNum > MAX_ROWS:
            errors.append((label, f'row {rowNum} is out of order or out of range'))
        prevRow = rowNum
        if int(row.get('s', 0)) >= nCellXfs:
            errors.append((label, f'row {rowNum} style={row.get("s")} but only {nCellXfs} cellXfs defined'))

        prevCol = 0
        for cell in row.findall(m('c')):
            ref = cell.get('r')
            if ref is None:
                colNum = prevCol + 1
            else:
                match = re.fullmatch(r'([A-Z]{1,3})(\d+)', ref)
                if not match:
                    errors.append((label, f"invalid cell reference '{ref}'"))
                    continue
                colNum = col2num(match.group(1))
                if int(match.group(2)) != rowNum:
                    errors.append((label, f'cell {ref} is inside row {rowNum}'))
                if colNum <= prevCol or colNum > MAX_COLS:
                    errors.append((label, f'cell {ref} is out of order or out of range'))
            prevCol = colNum
            ref = ref or f'column {colNum} of row {rowNum}'

            if int(cell.get('s', 0)) >= nCellXfs:
                errors.append((label, f'cell {ref} style={cell.get("s")} but only {nCellXfs} cellXfs defined'))

            cellType = cell.get('t', 'n')
            value = cell.find(m('v'))
            if cellType == 's':
                try:
                    if value is None or not 0 <= int(value.text) < nStrings:
                        raise ValueError
                except (TypeError, ValueError):
                    errors.append((label, f'cell {ref} has an invalid shared string index'))
            elif cellType == 'inlineStr':
                inline = cell.find(m('is'))
                if inline is not None and len(textOf(inline)) > MAX_CELL_TEXT:
                    errors.append((label, f'cell {ref} text exceeds {MAX_CELL_TEXT} chars'))
            elif cellType == 'str':
                if value is not None and len(value.text or '') > MAX_CELL_TEXT:
                    errors.append((label, f'cell {ref} text exceeds {MAX_CELL_TEXT} chars'))

    return errors


def checkXlsxValidity(xlsxPath):
    """Check an xlsx file for structural validity."""
    print(f"### Checking {xlsxPath} for xlsx validity\n")

    try:
        zf = zipfile.ZipFile(xlsxPath)
    except (IOError, zipfile.BadZipFile) as e:
        raise SystemExit(f"{RED}### Failed to read {xlsxPath}: {e}{RESET}")

    with zf:
        errors = checkPackage(zf)
        wbErrors, sheets = checkWorkbook(zf)
        styleErrors, nCellXfs = checkStyles(zf)
        stringErrors, nStrings = checkSharedStrings(zf)
        errors += wbErrors + styleErrors + stringErrors
        for sheetName, part in sheets:
            errors += checkSheet(zf, sheetName, part, nCellXfs, nStrings)

    perPart = {}
    for part, msg in errors:
        perPart.setdefault(part, []).append(msg)

    for part, msgs in perPart.items():
        for msg in msgs[:MAX_ERRORS_PER_PART]:
            print(f"{RED}  {part}: {msg}{RESET}")
        if len(msgs) > MAX_ERRORS_PER_PART:
            print(f"{RED}  {part}: ... and {len(msgs) - MAX_ERRORS_PER_PART} more{RESET}")

    print()
    if errors:
        print(f"### Summary: {RED}{len(errors)} error(s) found{RESET}")
    else:
        print(f"### Summary: {GREEN}No errors found{RESET}")

    return len(errors)


if __name__ == '__main__':
    errors = checkXlsxValidity(args.xlsx_path)
    sys.exit(1 if errors > 0 else 0)
