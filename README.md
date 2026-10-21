# filament2

Legacy Qt4 / C++98 filament trend viewer.

## What it is used for

- Reading configured daily filament measurement files.
- Displaying device/tube measurements and their visual value bands for operator review.
- It presents measurements; it does not claim remaining tube life, a confirmed fault or an automatic maintenance decision.

## Project attribution

The original project, application design and field use are by **Ekrem Gokberk ISCAN**. AI-assisted development was used as a supporting tool during selected maintenance, documentation and implementation work.

## Archive paths (21 October 2026)

For each selected date, the first existing file wins. A malformed or unreadable preferred file shows an error; its legacy copy is not substituted. Source paths are available in tooltips and the usage table. Archives are read-only.

| Data | Preferred directory | Legacy fallback |
| --- | --- | --- |
| Filament | `/opt/Eko/logs/filament/Daily` | optional `filament2.ini` `filament/daily_directory`, then `/home/eds_cm/eko/filament/Daily` |
| Tube snapshot | `/opt/Eko/logs/tubelog/log` | `/root/tubelog/log` |
| X-ray On | `/opt/Eko/logs/xrayontime/log` | `/root/Eko/xrayontime/log` |
| X-ray Off | `/opt/Eko/logs/xrayontime/offlog` | `/root/Eko/xrayontime/offlog` |

Names remain `filament-YYYY-MM-DD` (also `.txt`), `tube-YYYY-MM-DD.csv`, `xrayontime-YYYY-MM-DD.csv` and `xrayofftime-YYYY-MM-DD.csv`. New and legacy files for the same date are never added together. No fixed migration date is required.

AutoSeasoning still writes `/root/tup_kayitlari.csv`; the operator's existing daily copy process supplies tube snapshots. Neither the active ledger nor collectors, copy schedules or the existing Current Date save scripts are changed here. Copy scripts must publish current files into the new directories; an existing new-date file takes priority over an updated legacy copy.

## Tube usage

The **Tube Usage** tab is entirely English and selects a device, tube position and snapshot date. It shows the removed and installed serials, recorded device X-ray On totals and valid/expected/missing/invalid day counts. An exact snapshot for the selected date is required; future tube snapshots are not used to infer past state. Refresh reloads the archives without changing the selection.

- The CSV's historical `Sökülen Tarih` column is the previous tube's **installation date**, not its removal date.
- Removed tube accounting: previous installation + 1 day through current installation - 1 day. Installed tube accounting: current installation + 1 day through the earlier of the selected date and yesterday. Today's date is excluded from the chart, expected/missing/invalid day counts and duration totals, even if a current-day On file exists. Today's tube snapshot can still be selected. A tube installed today has no complete days yet.
- Date-only installation boundaries are displayed but excluded from both attributed totals. Missing days are unknown; a recorded `00:00:00` is a valid zero. If no valid days exist the total is `--`.
- Totals are **recorded device operating time**, not an independently measured tube exposure or remaining-life estimate. Hours/minutes/seconds express the archive's duration, not installation-time precision. No change timestamps are invented.
- The chart/table displays up to the last 90 calendar days of the selected lifetime. Totals include all available dates in the lifetime. The final archive day may be partial. Daily snapshots cannot preserve every replacement made within one day; only the pair recorded in the chosen snapshot is shown.
- The inherited daily On format is a device section such as `ED_01` containing one `Total working time: HH:MM:SS` line. Duplicate, missing, malformed or greater-than-24-hour daily totals are errors. Actual field archive acceptance remains separate from synthetic regression tests.
- Click the underlined coverage of a period with invalid records to see every invalid date, its specific reason and full source path, including dates outside the 90-day chart. Click an individual missing/invalid daily duration for its details. Enter activates the focused cell as well. Explanations distinguish duplicate totals, malformed durations, out-of-range values, unreadable files and absent device totals.

Qt 4.7 / C++98, with no new runtime dependencies. Build out of tree; the existing `.pro` still retains its target installation paths. For a local build, override output locations after reading the project:

UI polish: shared chevron controls with a 32-pixel dropdown target, clearer focus/hover states and dark calendar; larger text and alternating table rows; full source paths remain in tooltips while the table shows filenames. The icon is an embedded PNG and requires no SVG plugin. The existing tabs' date/button row uses the same date controls with larger geometry.

```sh
qmake /path/to/filament2.pro -after 'DESTDIR=.' 'OBJECTS_DIR=.obj' 'MOC_DIR=.moc' 'RCC_DIR=.rcc' 'UI_DIR=.ui' 'QMAKE_CXXFLAGS+=-std=gnu++98'
make -s
```

Tests: run qmake on `tests/archive_test.pro` and `tests/usage_gui_test.pro` in separate build directories; then `make -s` and the resulting executable with `-silent`. GUI tests require an X display. Fixtures use temporary copies and do not write operational archives. The project does not include operational data files.

## Compatibility checks

Validated with Qt 4.7 locally and Qt 4.6.3 / GCC 4.3.2 on legacy Linux. The archive and GUI suites each passed with 8 Qt test totals. Actual scanner/archive acceptance is separate from these fixture tests.
