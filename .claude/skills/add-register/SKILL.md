---
name: add-register
description: Add a new Modbus TCP register for a D-Bus path, updating attributes.csv, test/baseline.json and CCGX-Modbus-TCP-register-list.xlsx, then run the register-map CI checks. Use when asked to add, expose or map a D-Bus path/setting to a Modbus register, or to add a register for a service like com.victronenergy.vebus.
---

# Add a Modbus register

The register map lives in three places that must agree:

- `attributes.csv`: what the server actually serves
- `test/baseline.json`: the known (accepted) gaps in each service's address range
- `CCGX-Modbus-TCP-register-list.xlsx`: the published documentation

Work through the steps in order. Don't commit unless the user asks.

## 1. Gather the inputs

You need:

- **Service**, e.g. `com.victronenergy.vebus`
- **D-Bus path**, e.g. `/Settings/WeakAcInput`
- **D-Bus type**: a single D-Bus basic type letter. `i` is used for integers and enums, `d` for floats, `s` for strings, `y` for bytes. Match what similar registers in the service use.
- **Unit or enum text**, e.g. `W`, `V AC`, `0=Off;1=On`. Never use commas. Use `;` between enum values.
- **Scale factor**, e.g. `1`, `10`, `100`
- **Access**: `R` (read-only) or `W` (writable)
- **Description** for the xlsx, e.g. `Weak AC input setting`
- **Issue link** for the commit message, if there is one

If the user hasn't given something and it can't be inferred from similar registers, ask. Look at neighbouring registers for the service's conventions (units, scale factors for voltage/current/power, and so on).

## 2. Choose the width: 16 or 32 bits

Work out the realistic value range and multiply it by the scale factor:

| Type     | Raw range                     | Use when                                        |
|----------|-------------------------------|-------------------------------------------------|
| `uint16` | 0 to 65535                    | Non-negative, and max × scale ≤ 65535           |
| `int16`  | -32768 to 32767               | Can be negative, and \|value\| × scale ≤ 32767  |
| `uint32` | 0 to 4294967295               | Doesn't fit in 16 bits                          |
| `int32`  | -2147483648 to 2147483647     | Signed, doesn't fit in 16 bits                  |
| `string[N]` | N registers, 2 chars each  | Text (D-Bus type `s`)                           |

Think about future growth. Power in W on large systems, energy counters and anything summed over phases or devices often overflows 16 bits. For example, 50 kW at scale 1 doesn't fit in `int16`. Choose a signed type when the value can go negative (current, power flowing both ways). Tell the user which type you chose and why.

## 3. Choose the address

1. List the service's rows: `grep -n '^com.victronenergy.<svc>,' attributes.csv`. Rows are grouped per service and ordered by address.
2. Find registers related to the new path (same path prefix, same feature, phase groups L1/L2/L3). The new register should sit close to them.
3. Look for `RESERVED` rows near them, e.g. `com.victronenergy.vebus,RESERVED,d,,122,reserved[7],1,R`. A `reserved[N]` block at address A covers A to A+N-1. Taking space from one is the preferred option. Shrink it or split it into two reserved rows so that every address stays covered. Commit c4e1daf is an example: `reserved[8]`@122 became `reserved[7]`@122 plus the new register @129.
4. If there's no suitable reserved space, use:
   - the service's next free address after its current last register, or
   - a gap already listed for that service in `test/baseline.json`.
   Check that the address isn't used by another service in the same address space. `check_register_gaps.py` treats `com.victronenergy.settings`, `system`, `hub4` and `platform` as one group (`SYSTEM`), because they share a unit ID.
5. **32-bit registers must start on an even address**, even though the protocol doesn't require it. If that leaves a one-register hole, fill it with a `reserved[1]` row, or leave it inside the existing reserved block.
6. Show the user the proposed address, together with the neighbouring rows, before you edit anything.

## 4. Edit `attributes.csv`

Each row has 8 columns:

```
service,path,dbustype,unit,address,modbustype,scale,R|W
```

Example: `com.victronenergy.vebus,/Settings/WeakAcInput,i,0=Strong AC input;1=Weak AC input,129,uint16,1,W`

Insert the row in address order within the service's block, and adjust any reserved row you are taking space from.

## 5. Check `test/baseline.json`

The baseline lists each service's accepted gaps as `[start, end]` ranges. The gap check fails on any gap range that isn't listed exactly. Run:

```
python3 test/check_register_gaps.py attributes.csv --baseline test/baseline.json
```

- If it passes, the baseline doesn't need changing (typical when you used reserved space).
- If it reports a new gap, first ask whether the gap should be filled by a reserved row. Only intentional gaps should go in the baseline.
- If the new register sits inside, or next to, a baseline gap, that gap shrinks or splits and no longer matches. Regenerate the baseline:
  ```
  python3 test/check_register_gaps.py attributes.csv --generate-baseline > test/baseline.json
  git diff test/baseline.json
  ```
  Check in the diff that only the ranges you meant to touch changed.

## 6. Update `CCGX-Modbus-TCP-register-list.xlsx`

Use the bundled script. It needs openpyxl (`pip install openpyxl`, or a venv):

```
python3 .claude/skills/add-register/scripts/xlsx_add_register.py \
    --service com.victronenergy.vebus \
    --description "Weak AC input setting" \
    --address 129 --type uint16 --scale 1 \
    --path /Settings/WeakAcInput --writable yes \
    --unit "0=Strong AC input;1=Weak AC input" \
    --doc-summary "Add Weak AC input setting (register 129)"
```

What the script does:

- Inserts the row on the **Field list** sheet in address order within the service's block, copying the formatting of the nearest register row.
  - Columns: A service, B description, C address, D type, E scale factor, F range, G D-Bus path, H writable (`yes`/`no`, from CSV `W`/`R`), I D-Bus unit (unit or enum text), J remarks (`--remarks`).
- Computes **Range** from the type and scale, e.g. `int16`/10 gives `-3276.8 to 3276.7`. Pass `--range` to override it.
- Shrinks or splits the service's `RESERVED` row (shown as e.g. `122-127` in column C) if the new address falls inside it.
- Refuses to add a register that overlaps an existing one.
- Appends `--doc-summary` as a new line on the **Document versions** sheet, in column B only. Don't add a new `Rev NN` line; that is done manually at release.
- Works around an openpyxl bug that would otherwise leave invalid `xfId`s in `styles.xml`.

Afterwards:

- The script ends with a **Check in Excel** list giving the final row numbers of the new register, any adjusted RESERVED rows and the Document versions line. Keep it for step 8.
- Check the script's output. Reserved ranges in the xlsx don't always match the CSV exactly, so if the new register was taken from reserved space in the CSV but the script didn't report adjusting a RESERVED row, look at the xlsx reserved row by hand.
- If the service has no rows in the xlsx yet, the script stops. Add the first row by hand, or ask the user.

## 7. Run the CI checks

These are the same checks as `.github/workflows/check-register-gaps.yml`. All four must pass:

```
python3 test/check_register_gaps.py attributes.csv --baseline test/baseline.json
python3 test/check_register_overlaps.py attributes.csv
python3 test/check_column_validity.py attributes.csv
python3 test/check_xlsx_validity.py CCGX-Modbus-TCP-register-list.xlsx
```

Fix any failures before you finish. Show the user `git diff --stat` and the `attributes.csv` diff.

## 8. Have the user check the spreadsheet in Excel

LibreOffice silently accepts some corruption that Microsoft Excel rejects, and a corrupted register list has reached customers before. `check_xlsx_validity.py` only catches known problems, so a person must open the file in Excel before it is committed.

Give the user the **Check in Excel** list from step 6. For example:

```
Field list, row 221: com.victronenergy.vebus 235 /MicroGrid/AllowBlackStart
Field list, row 222: RESERVED 236-239
Document versions, row 184: Add Microgrid black start allowed (register 235)
```

Ask them to open `CCGX-Modbus-TCP-register-list.xlsx` in **Microsoft Excel** (LibreOffice is not enough) and confirm that:

- Excel opens it without a "We found a problem with some content… Do you want us to try to recover as much as we can?" prompt
- the new register is on the listed row of the **Field list** sheet, between the right neighbouring addresses, with the right values and the same formatting as the rows around it
- any adjusted RESERVED row shows the reduced range
- the **Document versions** line is there, with no new Rev number

Ask them to close Excel without saving, so that the diff contains only the script's change.

**Don't commit until the user confirms that the file is fine in Excel.** If Excel reports a problem, stop and investigate. Ask the user before discarding the change with `git checkout CCGX-Modbus-TCP-register-list.xlsx`.

## 9. Commit (only when asked)

Follow the repo's style: a short subject prefixed with the service's short name, and the issue link in the body. For example:

```
vebus: Add WeakAcInput setting register

https://github.com/victronenergy/venus-private/issues/703
```

Commit `attributes.csv`, `CCGX-Modbus-TCP-register-list.xlsx` and, if it changed, `test/baseline.json` together.
