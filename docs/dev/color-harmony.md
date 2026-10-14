# Color harmony

The harmony generator derives a palette's other colors from its primary, the
top color in the list, using a color-harmony scheme. This doc is the contract
for that generator: the scheme ids, the derived-color table, the math, and the
golden fixture it is checked against. The web UI implements it in TypeScript.
The Flutter app's port must produce the same output. Generation is client-side
only. The firmware never computes a harmony color.

Files:

| File | Purpose |
|---|---|
| `software/lamp-ui/src/utils/colorHarmony.ts` | `COLOR_SCHEMES`, `schemeById`, `harmonyColors`, `harmonizePalette`, `autoPickPalette` |
| `software/lamp-ui/src/utils/colorUtils.ts` | `parseHexww`, `formatHexww`, `rgbToHsv`, `hsvToRgb` |
| `software/lamp-ui/src/utils/colorHarmony.test.ts` | Hand-checked vectors, edge cases, the golden test |
| `software/lamp-ui/src/utils/colorHarmony.golden.json` | The golden fixture |

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

`harmonyColors(primary, scheme, count)` returns colors **top→bottom**. Index 0
is the primary itself: the normalized input string, never round-tripped through
HSV. Each derived entry is `(Δh, s×)`:

- hue becomes `(h + Δh) mod 360`;
- saturation becomes `s × s×`, and HSV→RGB takes that product as its `s`, so its chroma is `v × (s × s×)`;
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
    channel's entry from the table, clamped to 0–255. Halves
    round up (`Math.round`; Dart `round()` agrees for non-negative values).
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

| Primary | Scheme | Count | Expected (top→bottom) |
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

## Orientation adapter

`harmonizePalette(palette, scheme, count = palette.length)` takes a palette in
**storage order** (bottom→top) and reads the primary from its last index. It
returns `harmonyColors(primary, scheme, count)` reversed, so the primary stays
at the last index. Code that writes a palette uses this adapter. A display-only
preview, such as the Setup tab's scheme swatches, calls `harmonyColors`
directly.

## Web UI

Auto-pick is configured on the Setup tab and shown on the Home tab.

- **Setup, Color Harmony section.** Shown when at least one surface's colors
  are editable. `ColorSchemePicker.vue` holds the controls:
  - "Auto-pick colors for" is a multi-select chip group, one chip per editable
    surface (Shade, Base). Each chip sets that surface's `autoColors`
    independently.
  - "Scheme" is one scheme shared by both surfaces. It starts as a summary row
    (the scheme's name and swatch) with a Change / Hide button that opens or
    closes the scheme list. The list is a radio group and a pick keeps it open.
    The summary reads "Mixed" with a note when both surfaces are editable and
    their schemes differ after `schemeById`. An unknown id counts as Analogous,
    so stored ids 0 and 99 match and the summary shows Analogous with no note.
    With one editable surface, that surface's scheme is shown.
    Swatches preview the first surface whose chip is on, Shade before Base.
- **Home, color strips.** Home only shows the state. A surface with Auto-pick on locks
  every row but the primary and shows an "Auto-picked" note with a link to the
  Setup tab. A neutral primary shows "pick a more colorful top color" instead.

Regeneration runs only in these handlers, never in a watcher:

| Trigger | Result |
|---|---|
| Chip turned on | `autoPickPalette`: a single color grows to the scheme's natural count, otherwise the count is kept, then the palette is regenerated from its primary. |
| Chip turned off | `autoColors` is cleared and the colors stay as they are. |
| Scheme pick | `colorScheme` is written on every editable surface. Surfaces with Auto-pick on regenerate with `harmonizePalette`; the others keep their colors. |
| Primary edited on Home | Derived colors regenerate. |
| Color added or removed on Home | Regenerates at the new count. |

The shared color picker has no harmony controls. It edits one color through the
hex input and the R, G, B, and Warm White sliders.

## Golden fixture

`software/lamp-ui/src/utils/colorHarmony.golden.json` pins the generator's
output as `{ "cases": [{ "primary", "scheme", "count", "expected" }] }`. It
covers every scheme × count 2–5 for the primaries `#FF000000`, `#3A7BD5C8`,
`#F4A6B800`, and `#FFFFFF00` (96 cases). `scheme` is the numeric id, the value
that gets stored, so the fixture also pins the id mapping. `expected` is
`harmonyColors` output, top→bottom.

`colorHarmony.test.ts` asserts the generator reproduces the file. After a
deliberate table change, change this doc and the table constant in
`colorHarmony.ts` together, then regenerate from the repo root:

```sh
UPDATE_HARMONY_GOLDEN=1 npm run ui:test
```

The output is deterministic, so with no table change the command leaves git
clean. After a table change, review the golden diff before committing.

The Flutter port must pass the same fixture, case for case, and every hand-checked vector. Passing them doesn't prove parity. Reordering a formula can flip a tie that neither catches, so implement each formula in the order written.

## Storage

The firmware stores two keys per surface, inside both the `base` and `shade`
objects of the settings JSON. `autoColors` is whether Auto-pick is on for the surface and
`colorScheme` is a scheme id from the table above. The firmware never acts on
either. Generated colors are ordinary entries in `segments[].colors`, so a
generated palette renders like a manual one.

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
