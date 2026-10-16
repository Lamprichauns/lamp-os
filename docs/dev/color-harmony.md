# Color harmony

The harmony generator derives a palette's other colors from its primary, the
palette's last stored color, using a color-harmony scheme. This doc is the
contract for that generator: the scheme ids, the derived-color table, the math,
and the golden fixture it is checked against. Two clients implement it: the web
UI in TypeScript and the Flutter app in Dart. Both must pass the same golden
fixture and hand-checked vectors. Generation is client-side only. The firmware
never computes a harmony color.

The last three sections cover how the firmware stores each surface's Auto-pick
state, how the web UI exposes it, and how the Flutter app does.

Files:

| File | Purpose |
|---|---|
| `software/lamp-ui/src/utils/colorHarmony.ts` | `COLOR_SCHEMES`, `schemeById`, `harmonyColors`, `harmonizePalette`, `autoPickPalette` |
| `software/lamp-ui/src/utils/colorUtils.ts` | `parseHexww`, `formatHexww`, `rgbToHsv`, `hsvToRgb` |
| `software/lamp-ui/src/utils/colorHarmony.test.ts` | Hand-checked vectors, edge cases, the golden test |
| `software/lamp-ui/src/utils/colorHarmony.golden.json` | The golden fixture |
| `software/lamp-app-flutter/lib/features/control/domain/color_harmony.dart` | Dart generator: `kHarmonySchemes`, `harmonySchemeById`, `rgbToHsv`, `hsvToRgb`, `harmonyColors`, `harmonizePalette`, `autoPickPalette`, `isNeutralColor`, `sharedSchemeId` |
| `software/lamp-app-flutter/test/features/control/domain/color_harmony_test.dart` | The same vectors and edge cases, plus the golden test reading the fixture in place |

## Schemes

| id | key | Name | Natural count | One-line description (UI copy) |
|---|---|---|---|---|
| 0 | `analogous` | Analogous | 3 | Neighboring hues. Smooth, calm blends. |
| 1 | `monochromatic` | Monochromatic | 3 | One hue, fading toward pastel. |
| 2 | `complementary` | Complementary | 2 | The primary and its opposite. High contrast. |
| 3 | `splitComplementary` | Split complementary | 3 | The primary plus the two hues beside its opposite. |
| 4 | `triadic` | Triadic | 3 | Three hues evenly spaced around the wheel. |
| 5 | `square` | Square | 4 | Four hues evenly spaced around the wheel. |

Natural count is the palette size a scheme is designed around. Id 0 is the
default, both when no scheme is stored and for ids not in this table. Ids are
what gets stored, so they never change. A new scheme takes the next id.

### Why Analogous is the default

The firmware blends linearly in RGB between adjacent stops along the strip
(`software/lamp-os/src/util/gradient.cpp`). Adjacent stops with opposite hues
pass through a grey band, so contrast schemes look muddier on a lamp than
analogous or monochromatic ones. The tint entries in the table below are there
to soften those transitions.

## Derived-color table

`harmonyColors(primary, scheme, count)` returns the primary first. Index 0 is
the primary itself: the normalized input string, never round-tripped through
HSV. Each derived entry is `(Δh, s×)`:

- hue becomes `(h + Δh) mod 360`;
- saturation becomes `s × s×`, and HSV→RGB takes that product as its `s`, so
  its chroma is `v × (s × s×)`;
- V (brightness) is the primary's V, unchanged;
- WW is the primary's WW byte.

| Scheme | count 2 | count 3 | count 4 | count 5 |
|---|---|---|---|---|
| Analogous | (30,1) | (30,1) (60,1) | (30,1) (60,1) (90,1) | (30,1) (60,1) (90,1) (120,1) |
| Monochromatic | (0,½) | (0,⅔) (0,⅓) | (0,¾) (0,½) (0,¼) | (0,0.8) (0,0.6) (0,0.4) (0,0.2) |
| Complementary | (180,1) | (0,½) (180,1) | (0,½) (180,½) (180,1) | (0,½) (0,0) (180,½) (180,1) |
| Split complementary | (150,1) | (150,1) (210,1) | (0,½) (150,1) (210,1) | (0,½) (150,1) (210,1) (210,½) |
| Triadic | (120,1) | (120,1) (240,1) | (0,½) (120,1) (240,1) | (0,½) (120,1) (240,1) (240,½) |
| Square | (180,1) | (90,1) (180,1) | (90,1) (180,1) (270,1) | (0,½) (90,1) (180,1) (270,1) |

Two rules generate whole rows:

- Analogous entry `k` (1-based) is `(30k, 1)`.
- Monochromatic entry `k` is `(0, 1 − k/N)`, where N is the count. The code
  evaluates the rule, so the fractions above are shorthand for its double
  results.

Count is an integer clamped to `[1, 5]`. A count of 1 returns `[primary]`.

## Math rules

Exact, so a port matches bit-for-bit.

- **Parsing.** Inputs are hex `#RRGGBB` or `#RRGGBBWW`, in either case. Six
  digits mean WW is 0. Parse with `parseHexww`. Any other string is outside this
  contract and its output is unspecified. RGB is converted to floats in
  `[0, 1]` as `byte / 255`.
- **`mod`.** `mod(x, m)` is `((x % m) + m) % m`, with `%` the truncating
  remainder (JS `%`, Dart `remainder`). Apply it literally at all four uses
  below, even to values already in range. The addition rounds, so a plain
  remainder can differ in the last bit. Dart's `%` operator is the Euclidean
  modulo, which equals a plain remainder for non-negative x, so it can't stand
  in for `mod` or for the `%` inside it. A slip at any one use passes the golden
  fixture. The hand-checked vectors below catch each one.
- **RGB→HSV.**
  - `max`/`min` are taken over r, g, b; `d = max − min`.
  - `v = max`.
  - `s = max == 0 ? 0 : d / max`.
  - `h = 0` when `d == 0`. Otherwise:
    - `max == r` → `60 × mod((g − b) / d, 6)`
    - `max == g` → `60 × ((b − r) / d + 2)`
    - `max == b` → `60 × ((r − g) / d + 4)`
  - Normalize h to `[0, 360)` with `mod(h, 360)`.
- **HSV→RGB.**
  - `h` is first reduced with `mod(h, 360)`, so any finite hue is valid. The
    generator passes `h + Δh` unreduced, which makes this the one
    `(h + Δh) mod 360` the table calls for. Reduce it exactly once.
  - `c = v × s`, `h' = h / 60`, `x = c × (1 − |mod(h', 2) − 1|)`, `m = v − c`.
  - The sector `⌊h'⌋` picks `(r, g, b)` from these:

    | Sector | (r, g, b) |
    |---|---|
    | 0 | (c, x, 0) |
    | 1 | (x, c, 0) |
    | 2 | (0, c, x) |
    | 3 | (0, x, c) |
    | 4 | (x, 0, c) |
    | 5 | (c, 0, x) |

  - Each channel byte is `round((value + m) × 255)`, where value is that
    channel's entry from the table, clamped to 0–255. Halves round up
    (`Math.round`; Dart `round()` agrees for non-negative values).
- **Ties.** Some outputs land exactly on a rounding tie, a channel of `k + 0.5`
  in byte space (for example `#3A7BD5C8` shifted by 30°). Which way a tie rounds
  depends on the operation order, so implement the formulas in the order
  written. A platform HSV conversion can disagree on ties.
- **Output format.** Every output for an input in the contract is 8-digit
  uppercase `#RRGGBBWW`.
- **Neutral primary.** When the primary has no hue (`s == 0` or `v == 0`; white,
  grey, black, or pure warm white), hue rotation and saturation scaling change
  nothing, so every derived color equals the primary. This is expected, not an
  error. Callers can detect it with `rgbToHsv` and tell the user.

## Hand-checked vectors

These must pass.

| Primary | Scheme | Count | Expected (primary first) |
|---|---|---|---|
| `#FF000000` | complementary | 2 | `#FF000000 #00FFFF00` |
| `#FF000000` | triadic | 3 | `#FF000000 #00FF0000 #0000FF00` |
| `#FF000000` | analogous | 3 | `#FF000000 #FF800000 #FFFF0000` |
| `#FF000000` | square | 4 | `#FF000000 #80FF0000 #00FFFF00 #8000FF00` |
| `#FF000000` | splitComplementary | 3 | `#FF000000 #00FF8000 #0080FF00` |
| `#FF000000` | monochromatic | 2 | `#FF000000 #FF808000` |
| `#FF000000` | complementary | 5 | `#FF000000 #FF808000 #FFFFFF00 #80FFFF00 #00FFFF00` |
| `#FF0000C8` | complementary | 2 | `#FF0000C8 #00FFFFC8` (WW copied) |
| `#FFFFFF00` | triadic | 3 | `#FFFFFF00 #FFFFFF00 #FFFFFF00` (neutral) |
| `#23000500` | analogous | 2 | `#23000500 #230C0000` (rounding tie; Dart `%` in `mod` gives `#230D0000`) |
| `#07010000` | analogous | 2 | `#07010000 #07050000` (a plain remainder for `mod((g − b) / d, 6)` gives `#07040000`) |
| `#00010B00` | complementary | 4 | `#00010B00 #06060B00 #0B0A0600 #0B0A0000` (skipping RGB→HSV's `mod(h, 360)` gives `#0B0B0600` third) |
| `#000B0100` | analogous | 2 | `#000B0100 #000B0700` (a plain remainder for HSV→RGB's `mod(h, 360)` gives `#000B0600`) |
| `#00010500` | splitComplementary | 2 | `#00010500 #05010000` (a plain remainder for `mod(h', 2)` gives `#05020000`) |
| `#01011700` | monochromatic | 4 | `#01011700 #06061700 #0C0C1700 #12121700` (chroma as `(v × s) × s×` gives `#07071700` second) |
| `#000A2100` | analogous | 2 | `#000A2100 #07002100` (hue from raw bytes instead of `byte / 255` gives `#06002100`) |
| `#00001A00` | monochromatic | 4 | `#00001A00 #06061A00 #0D0D1A00 #14141A00` (`m = v × (1 − s)` instead of `v − c` gives `#07071A00` second) |
| `#01012000` | analogous | 2 | `#01012000 #10012000` (`round(value × 255 + m × 255)` gives `#11012000`) |

## Orientation adapter

`harmonizePalette(palette, scheme, count = palette.length)` takes a palette in
**storage order** and reads the primary from its last index. It returns
`harmonyColors(primary, scheme, count)` reversed, so the primary stays at the
last index. Which physical end that is depends on the surface. Shade index 0 is
the bottom of the strip, so the Shade's primary is its top color. On lamps
whose Base strip is declared `reversed` (standard, staff, snafu), Base index 0
is the top of the base, so the Base's primary is its bottom color. The Flutter
client's Orientation bullet has the details.

Code that writes a palette uses this adapter. A display-only preview, such as
the Setup tab's scheme swatches, calls `harmonyColors` directly.

## Golden fixture

`software/lamp-ui/src/utils/colorHarmony.golden.json` pins the generator's
output as `{ "cases": [{ "primary", "scheme", "count", "expected" }] }`. It
covers every scheme × count 2–5 for the primaries `#FF000000`, `#3A7BD5C8`,
`#F4A6B800`, and `#FFFFFF00` (96 cases). `scheme` is the numeric id, the value
that gets stored, so the fixture also pins the id mapping. `expected` is
`harmonyColors` output, primary first.

`colorHarmony.test.ts` and `color_harmony_test.dart` both assert their
generator reproduces the file; the Dart test reads it in place as
`../lamp-ui/src/utils/colorHarmony.golden.json` from the Flutter package root.
After a deliberate table change, change this doc, the table constant in
`colorHarmony.ts` and `_derivedTable` in `color_harmony.dart` in the same
commit, then regenerate from the repo root:

```sh
UPDATE_HARMONY_GOLDEN=1 npm run ui:test
```

The output is deterministic, so with no table change the command leaves git
clean. After a table change, review the golden diff, then run
`npm run app:test` and confirm the Dart generator passes the regenerated
fixture before committing. No CI job runs `ui:test` or `app:test`, so these
local runs are the only gate.

Both generators must pass the same fixture, case for case, and every
hand-checked vector. Passing them doesn't prove parity. Reordering a formula
can flip a tie that neither catches, so implement each formula in the order
written.

## Storage

The firmware stores two keys per surface, inside both the `base` and `shade`
objects of the settings JSON. `autoColors` is whether Auto-pick is on for the
surface and `colorScheme` is a scheme id from the table above. The firmware
never acts on either. Generated colors are ordinary entries in
`segments[].colors`, so a generated palette renders like a manual one.

| Key | Type | Default when absent | Rule |
|---|---|---|---|
| `autoColors` | bool | `false` | A value that isn't a bool reads as `false`. |
| `colorScheme` | integer | `0` | Stored verbatim as a uint8. Anything but an integer in 0–255 loads as `0`. |

`colorScheme` is not checked against the scheme table, so a client can store an
id this doc doesn't list yet and adding a scheme needs no firmware change.
Clients validate: an unknown id generates and displays as Analogous, and the
stored value stays until the user picks a scheme.

The keys ride inside the existing settings payloads. There is no
`PROTOCOL_VERSION` bump and no `kGattLayout` change. Older firmware ignores the
keys and leaves them out of its reads, so clients treat an absent key as
`false` / `0`.

The firmware writes colors as lowercase hex and the generator writes uppercase,
so compare colors case-insensitively. The web UI uppercases each palette when it
loads, so regenerating an unchanged palette doesn't read as an edit.

Both keys flow through every path:

| Path | Where | Behavior |
|---|---|---|
| NVS persistence | `fromJson` and the stored-shape writers `toStoredJson` / `writeStoredJson` in `software/lamp-os/src/config/config_codec.cpp` | `fromJson` applies the defaults above. Both writers always write both keys inside `base` and `shade`, so a split save carries them in the rest blob (`docs/dev/embedded-heap.md`). |
| `GET /api/settings` | `Config::asJsonDocument()`, which calls `toJson` | Both keys on both surfaces. |
| BLE section reads | `Config::asBaseJson()` / `asShadeJson()` in `software/lamp-os/src/config/config.cpp` | Both keys on both surfaces, served through the page protocol. A `settings_blob` write invalidates the section cache. |
| BLE `settings_blob` writes | `apply::colorHarmonyLocal` in `software/lamp-os/src/components/apply/apply_color_harmony.hpp`, called from `settingsBlobLocal` | A key applies only when present and valid: `autoColors` as a bool, `colorScheme` as an integer in 0–255. Otherwise the stored value stays. |

The last row matters. A client that doesn't know the keys, such as an older
app, writes blobs without them, and those writes must not reset what another
client saved.

`PUT /api/settings` (`handlePutSettings` in
`software/lamp-os/src/components/webapp/webapp.cpp`) hands the posted document
to the loop, where `config_codec::saveRawJson` parses it once through `fromJson`
and writes it with `saveConfig`; the lamp then reboots and the constructor
loads the stored config. The keys are validated at that save by the rules
above. The web UI sends back the document it read from GET, so the keys
travel with it. A release before 1.2.8 reads only the whole `cfg` string, so
on a split store it loads the base and shade sections as defaults, these keys
included.

## Web UI

Auto-pick is configured on the Setup tab and shown on the Home tab.

- **Setup, Color Harmony section.** Shown when at least one surface's colors
  are editable. `software/lamp-ui/src/components/ColorSchemePicker.vue` holds
  the controls:
  - "Auto-pick colors for" is a multi-select chip group, one chip per editable
    surface (Shade, Base). Each chip sets that surface's `autoColors`
    independently.
  - "Scheme" shows one scheme for both surfaces. It starts as a summary row
    (the scheme's name and swatch) with a Change / Hide button that opens or
    closes the scheme list. The list is a radio group and a pick keeps it open.
    A pick writes the id to `colorScheme` on every editable surface. Until
    then, each surface generates with its own stored `colorScheme`.
  - The summary reads "Mixed" with a note when both surfaces are editable and
    their schemes differ after `schemeById`. An unknown id counts as Analogous,
    so stored ids 0 and 99 match and the summary shows Analogous with no note.
    With one editable surface, that surface's scheme is shown.
  - Swatches preview the first editable surface whose chip is on, Shade before
    Base. With no chip on, they preview Shade's primary when Shade is editable,
    otherwise Base's.
- **Home, color strips.** Home has no Auto-pick control. While a surface's
  Auto-pick is on, its derived rows' pickers are disabled, Clone is disabled on
  every row, and the primary has no Remove. A note under the primary reads
  "Auto-picked · <scheme name>", and the name links to the Setup tab. A neutral
  primary reads "Auto-picked · pick a more colorful top color" with no link.
  With Auto-pick off there is no note.

Regeneration runs only in these handlers, never in a watcher, and always with
the surface's own stored `colorScheme`.

| Trigger | Result |
|---|---|
| Chip turned on | `autoPickPalette`: a single color grows to the scheme's natural count, otherwise the count is kept, then the palette is regenerated from its primary. |
| Chip turned off | `autoColors` is cleared and the colors stay as they are. |
| Scheme pick | `colorScheme` is written on every editable surface. Surfaces with Auto-pick on regenerate with `harmonizePalette`; the others keep their colors. |
| Primary edited on Home while Auto-pick is on | Derived colors regenerate. |
| Color added or removed on Home while Auto-pick is on | Regenerates at the new count. |

Regeneration caps the count at 5, so a longer palette, which another client can
write, shrinks to 5 the first time it regenerates.

The shared color picker has no harmony controls. It edits one color through the
hex input and the R, G, B, and Warm White sliders.

Known issue: the Base strip on Home lists the last index first, labels that
end of its gradient preview "Top", and its neutral note asks for a "top
color". On lamps whose Base strip is declared `reversed` (standard, staff,
snafu), Base index 0 is the top of the base, so the last index marks the
bottom of the base. The primary is still the last index, as in the Flutter
app, so only the labels are wrong.

## Flutter client

The Flutter app implements the same feature with the same keys, so either
client can change what the other shows.

| What | Where (under `software/lamp-app-flutter/lib/features/`) |
|---|---|
| Generator | `control/domain/color_harmony.dart` |
| Section fields and eligibility | `autoColors`, `colorScheme` and `harmonyEligible` on `BaseSection` / `ShadeSection` in `control/domain/sections.dart` |
| Writes | `setColorHarmony`, `setAutoPick` and `pickColorScheme` on `ControlNotifier` in `control/application/control_notifier.dart` |
| Color harmony screen | `lamp_shell/presentation/color_harmony_screen.dart`, a "Color harmony" row under the LEDs heading of the Setup tab (`setup_screen.dart`) |
| Editor lock | `AutoPickConfig` and `ColorStopsSheet.autoPick` in `control/presentation/widgets/color_stops_sheet.dart`, passed by `shade_editor_sheet.dart` and `base_editor_sheet.dart` |
| Home note | `AutoPickNote` in `control/presentation/widgets/auto_pick_note.dart`, under the title of `ShadeCard` and `BaseCard` |

The Dart generator is checked against the golden fixture, read in place, and
every hand-checked vector above (`color_harmony_test.dart`).

- **Orientation.** The Shade lists top-first: its sheets, `ShadeCard` and the
  critter preview show the last index first, because Shade index 0 is the
  bottom of the strip. The Base lists index 0 first, which is the top of the
  base on reversed-Base lamps. The primary is the last index on both
  surfaces, so it is the Shade's first row and the Base's last row, and Base
  copy says "bottom color" where Shade copy says "top color".
- **Eligibility.** A surface takes part in Auto-pick when its colors are
  editable and it has at most one segment (`harmonyEligible`). The Setup row,
  the screen's chips, the Home note and the editor lock all follow it. A
  multi-segment shade (snafu) has one palette per segment, so it gets none of
  them; the web UI gates on editability alone, so on a multi-segment editable
  surface it shows controls the app omits.
- **Color harmony screen.** "Auto-pick colors for" is a multi-select
  `SegmentedButton` with one segment per eligible surface. "Scheme" is a
  summary row with a Change / Hide button over the scheme list; a pick keeps
  the list open, and open or closed is never stored. "Mixed" and the swatch
  preview follow the web UI's rules above, over eligible surfaces.
- **Persistence.** Every chip or scheme change writes immediately, one
  settings_blob per user action with `reboot: false`, carrying only the
  harmony keys of the affected surfaces. The firmware keeps one pending
  settings_blob payload, so two back-to-back writes could drop the first.
  The screen ignores chip and scheme taps while an earlier change is still
  writing.
  When a change regenerates colors, the colors stream over the surface's
  colors characteristic first, then the flags go, then one `commit()`
  persists the colors. If the flag write fails, the flags revert, the
  pre-regeneration colors stream back, nothing commits, and the screen shows
  an error snackbar.
- **Editor lock.** While a surface is on, its derived rows are dimmed and open
  no picker, the primary row has no remove button, reorder is off, and Add
  hides at 5. The sheet takes the scheme once when it opens. It never writes
  the flags; Save stays the ordinary colors write and commit.
- **Home note.** While an eligible surface is on, its card shows
  "Auto-picked · <scheme name>" under the title, and the name pushes the
  Color harmony screen. A neutral primary reads "Auto-picked · pick a more
  colorful top color" ("bottom color" on the Base) with no link. The card's
  own tap still opens the editor.

Regeneration runs only in these handlers, never in `build`, `initState` or a
provider rebuild, and always with the surface's own stored `colorScheme`.
Opening a sheet shows the saved colors exactly.

| Trigger | Result |
|---|---|
| Chip turned on (`setAutoPick`) | `autoPickPalette` streams, `autoColors: true` is written, then a commit. |
| Chip turned off | `autoColors: false` is written and the colors stay as they are. |
| Scheme pick (`pickColorScheme`) | Each eligible surface with Auto-pick on streams a `harmonizePalette` result, `colorScheme` is written on every eligible surface in one settings_blob, then a commit if anything regenerated. |
| Primary edited in the editor while Auto-pick is on | Derived colors regenerate on every live tick and on Done. Picker Cancel restores the whole list. |
| Color added or removed in the editor while Auto-pick is on | Regenerates at the new count. |

Swiping down a modified stops sheet reverts it like Cancel, so an abandoned
generated palette never waits in lamp RAM for the next commit.
