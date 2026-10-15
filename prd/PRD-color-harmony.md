# Feature: Color Harmony Auto-Pick (web UI + firmware storage)

## Overview

Users set 1–5 colors for each lamp surface (Shade and Base) in the web config UI
(`software/lamp-ui`, Vue 3). This feature adds four things:

1. **A color wheel in the shared color picker dialog.** Every color swatch gets
   it, expression colors included.
2. **An "Auto-pick colors" toggle per surface on the Home tab.** When it's on,
   the top color (the **primary**) drives the rest: colors 2–N are generated from
   the primary with a color-harmony scheme, and are shown but disabled.
3. **A "Color Harmony" section on the Setup tab** to pick the scheme, with a
   Shade / Base / Both selector that works like the expression target selector.
4. **Firmware storage** of the toggle and scheme per surface, so they survive a
   reload and the Flutter app can read them in a later phase.

**Revision 1 (2026-10-14) changed items 1-3** after the user reviewed the build:
no wheel in the picker; Auto-pick moves off the Home tab into the Setup tab as
Shade / Base chips with one shared, collapsible scheme list. See "Revision 1"
and Tasks 008-010. Item 4 is unchanged.

The web UI is the proving ground. The Flutter port is a later phase and out of
scope here. The generator spec and golden fixture built in this PRD are what
that port will be checked against.

## Working environment (read before every task)

- **Repo and branch.** All work happens in the main checkout at
  `/Users/mikebartlett/Developer/GitHub/lamp-os` on branch
  `feature/color-wheel-addition`. It is NOT a worktree. Before any build, test,
  or commit, run `git rev-parse --show-toplevel` and
  `git rev-parse --abbrev-ref HEAD` and confirm both. If either is wrong, stop
  and record a blocker.
- **Staging.** Stage files by path. Never `git add -A` / `git add .`, and never
  stage `.codegraph/` (untracked local tooling).
- **Read `CLAUDE.md` first.** Its comment conventions and code conventions apply
  to every line you write. A summary is at the end of this PRD.
- **Use the root npm tasks** (`npm run ui:test`, `npm run lamp:test`, etc.), not
  raw `vitest` / `pio` invocations, once Task-001 has added the UI tasks.
- **One task per iteration, one commit per task.** Use conventional commit
  messages matching the repo history: `feat(web-ui): …`, `feat(fw): …`,
  `test(web-ui): …`, `docs(dev): …`, `chore(web-ui): …`.
- **Don't improvise design changes.** If the code differs from what a task
  describes in a way that forces a design decision, record it under Blockers in
  `PROGRESS.md` and stop. The decisions below were settled with the user.

## Design decisions (settled; don't re-litigate)

- **D1. The primary is the top row.** The top row in the color list is the top
  of the lamp, which is the **last index** of the stored palette array.
  `ColorGradient.vue` stores palettes bottom→top (index 0 = pixel 0) and renders
  them reversed.
- **D2. Generation is client-side only.** The firmware only stores `autoColors`
  and `colorScheme`. Generated colors are ordinary colors in
  `segments[].colors`, so the firmware renders them like any manual palette.
- **D3. Regenerate only from user-event handlers.** (Revision R4 replaces the
  toggle trigger below.) The triggers are:
  - the primary changes (every live-drag tick, plus the Cancel/close revert);
  - the auto toggle is turned on;
  - a scheme is picked in Setup;
  - a color is added or removed while auto is on.

  Never regenerate in a watcher or on config load. Loading the page must show
  the saved colors exactly, even if they don't match what the generator would
  produce. Another client may have edited them.
- **D4. Turning auto on and off.** Turning it on with a count of 1 raises the
  count to the scheme's natural count, then overwrites colors 2–N. Turning it
  off keeps the current colors as they are.
- **D5. While auto is on:**
  - derived rows stay visible but their pickers are disabled;
  - Clone is disabled on every row;
  - the primary row has no Remove button;
  - Remove on a derived row lowers the count by 1 and regenerates;
  - Add raises the count by 1 (max 5) and regenerates.
- **D6. Warm white.** Derived colors copy the primary's warm-white (WW) byte.
  The wheel and all hue/saturation math operate on RGB only.
- **D7. One scheme per surface.** The Setup tab's Shade / Base / Both selector
  is an editing lens and is not stored. "Both" writes the same scheme to both
  surfaces. **Superseded by R3.**
- **D8. Unknown scheme ids.** A scheme id the UI doesn't know (for example from
  a newer client) is treated as Analogous for generation and display. The stored
  value is not rewritten unless the user picks a scheme.
- **D9. Firmware stores `colorScheme` verbatim** as a uint8 and never acts on
  it. Clients validate. Adding a scheme later needs no firmware change.
- **D10. One shared wheel.** The wheel lives in the shared `ColorPicker.vue`,
  so every color swatch gets it. **Superseded by R1.**
- **D11. No new runtime dependencies.** The UI bundle ships in the lamp's SPIFFS.
  Dev-only dependencies are fine if pinned to exact versions (no `^` / `~`).
- **D12. No release work.** No `VERSION` bump and no `docs/CHANGELOG.md` entry.
  The release decision happens later with the other devs.
- **D13. No protocol or GATT change.** The new fields ride inside the existing
  settings JSON payloads. CLAUDE.md allows payload evolution: no
  `PROTOCOL_VERSION` bump, no `kGattLayout` change.

## Revision 1 (2026-10-14; settled with the user)

The user reviewed the build on a lamp and changed the design. These decisions
supersede the ones they name. Tasks 008-010 implement them; where they conflict
with Tasks 004-006, the revision wins.

- **R1. No color wheel.** The wheel and the Brightness slider come out of
  `ColorPicker.vue`, because they took too much room in the dialog. Delete
  `ColorWheel.vue` and its test. The picker returns to the large preview, the
  hex input, and the R / G / B / Warm White sliders. Keep Task-004's overlay
  rule (the dialog closes only when the press and the click both start on the
  overlay), so a slider drag released outside the dialog still neither closes
  nor reverts it. Supersedes D10 and the wheel and Brightness parts of Task-004.
- **R2. Auto-pick lives in Setup, per surface.** The Home tab loses the
  Auto-pick toggle row. The Setup tab's Color Harmony section gets "Auto-pick
  colors for" with two independent chips, Shade and Base: none, either, or both
  can be on. Each chip is that surface's stored `autoColors` flag. A chip shows
  only when its surface's colors are editable; the section stays hidden when
  neither is.
- **R3. One shared scheme.** The UI shows one scheme. A pick writes the same id
  to `colorScheme` on every editable surface, whether or not its chip is on,
  then regenerates each surface whose chip is on. The firmware still stores the
  key per surface. When both surfaces are editable and their stored schemes
  differ (by `schemeById`, so D8 holds), no option is selected and the note
  "Shade uses X, Base uses Y. Picking a scheme sets both." shows until the user
  picks. The Shade / Base / Both target lens is gone. Supersedes D7 and the
  target-selector parts of Task-006.
- **R4. A chip follows D4.** Turning a chip on: if that surface has one color,
  raise its count to the scheme's natural count, then overwrite colors 2-N with
  `harmonizePalette`. Turning it off keeps the colors. This replaces the D3
  trigger "the auto toggle is turned on" with "a surface's chip is turned on in
  Setup". The other D3 triggers, D3's no-watcher rule, and D5 stay.
- **R5. The scheme list is collapsible.** Collapsed by default: one summary row
  with the label "Scheme", the selected scheme's name ("Mixed" in the R3 mixed
  state), its swatch strip, and a button that reads "Change" and opens the list,
  then reads "Hide" while it is open, like the Expressions tab's Configure /
  Hide. Open: the existing radio list. A pick keeps the list open. Open or
  closed is UI state and never stored.
- **R6. Home shows a compact note while Auto-pick is on.** A surface with
  `autoColors` on shows one short line, "Auto-picked · <scheme name>", where the
  scheme name is a button-styled link to the Setup tab (`open-scheme-settings`).
  With a neutral primary the line reads "Auto-picked · pick a more colorful top
  color" with no link. No line while `autoColors` is off. The disabled prop
  disables the link. The D5 behaviors stay: locked derived rows, no Clone, no
  Remove on the primary, Add / Remove and primary edits regenerate.
- **R7. No firmware change.** `autoColors` and `colorScheme` keep the
  per-surface storage contract below.

## Color harmony spec

This section is the contract. Task-002 copies it into `docs/dev/color-harmony.md`
and implements it exactly.

### Schemes

| id | key | Name | Natural count | One-line description (UI copy) |
|---|---|---|---|---|
| 0 | `analogous` | Analogous | 3 | Neighboring hues. Smooth, calm blends. |
| 1 | `monochromatic` | Monochromatic | 3 | One hue, fading toward pastel. |
| 2 | `complementary` | Complementary | 2 | The primary and its opposite. High contrast. |
| 3 | `splitComplementary` | Split complementary | 3 | The primary plus the two hues beside its opposite. |
| 4 | `triadic` | Triadic | 3 | Three hues evenly spaced around the wheel. |
| 5 | `square` | Square | 4 | Four hues evenly spaced around the wheel. |

Id 0 is the default, both when the key is absent and for unknown ids.

**Why Analogous is the default.** The firmware blends linearly in RGB between
adjacent stops along the strip (`software/lamp-os/src/util/gradient.cpp`).
Adjacent stops with opposite hues pass through a grey band, so contrast schemes
look muddier on a lamp than analogous or monochromatic ones. The tint entries in
the table below are there to soften those transitions.

### Derived-color table

`harmonyColors(primary, scheme, count)` returns colors **top→bottom**. Index 0
is the primary itself: the normalized input string, never round-tripped through
HSV. Each derived entry is `(Δh, s×)`:

- hue becomes `(h + Δh) mod 360`;
- saturation becomes `s × s×`;
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
- Monochromatic entry `k` is `(0, 1 − k/N)`, where N is the count.

Count is clamped to `[1, 5]`. A count of 1 returns `[primary]`.

### Math rules (exact, so the Dart port matches bit-for-bit)

- **Parsing.** Use the existing `parseHexww`. RGB is converted to floats in
  `[0,1]` as `byte / 255`.
- **RGB→HSV.**
  - `max`/`min` are taken over r, g, b; `d = max − min`.
  - `v = max`.
  - `s = max == 0 ? 0 : d / max`.
  - `h = 0` when `d == 0`. Otherwise:
    - `max == r` → `60 × (((g − b) / d) mod 6)`
    - `max == g` → `60 × ((b − r) / d + 2)`
    - `max == b` → `60 × ((r − g) / d + 4)`
  - Normalize h to `[0, 360)`.
  - `mod` is always the non-negative remainder. Note that JS `%` returns
    negatives, so use `((x % m) + m) % m`.
- **HSV→RGB.**
  - `c = v × s`, `h' = h / 60`, `x = c × (1 − |h' mod 2 − 1|)`, `m = v − c`.
  - The sector `⌊h'⌋` picks `(r,g,b)` from these, then `m` is added to each:

    | Sector | (r, g, b) |
    |---|---|
    | 0 | (c, x, 0) |
    | 1 | (x, c, 0) |
    | 2 | (0, c, x) |
    | 3 | (0, x, c) |
    | 4 | (x, 0, c) |
    | 5 | (c, 0, x) |

  - Each channel byte is `Math.round(value × 255)`, clamped to 0–255.
- **Output format.** Every output is 8-digit uppercase `#RRGGBBWW`.
- **Neutral primary.** When the primary has no hue (`s == 0` or `v == 0`; white,
  grey, black, or pure warm white), hue rotation and saturation scaling change
  nothing, so every derived color equals the primary. This is expected and
  documented. The UI shows a hint (Task-005).

### Hand-checked vectors (must pass)

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

### Orientation adapter

`harmonizePalette(palette, scheme, count = palette.length)` takes a palette in
**storage order** (bottom→top) and reads the primary from its last index. It
returns `harmonyColors(primary, scheme, count)` reversed, so the primary stays
at the last index. Components use this adapter. Only the spec and tests use
`harmonyColors` directly.

## Firmware storage contract

Two keys are added inside both the `base` and `shade` JSON objects:

| Key | Type | Default when absent | Rule |
|---|---|---|---|
| `autoColors` | bool | `false` | |
| `colorScheme` | integer | `0` | Stored verbatim as uint8. Values outside 0–255 load as 0. |

The keys must flow through every path:

| Path | Where |
|---|---|
| NVS persistence | `config_codec` `fromJson` / `toJson` |
| `GET /api/settings` | built from `Config::asJsonDocument()`, i.e. `toJson` |
| BLE section reads | `Config::asBaseJson()` / `asShadeJson()`, served paged, so no ATT size concern |
| BLE `settings_blob` writes | `apply::settingsBlobLocal`, applied only when the key is present |

The last rule matters: a partial write that omits the keys (for example from
today's Flutter app) must leave them unchanged.

## Tasks

Dependencies:

- Task-001 → Task-002 → (Task-004, Task-005) → Task-006 → Task-007.
- Task-003 (firmware) is independent. Run it any time before Task-007.
- Revision 1: Task-008 -> Task-009 -> Task-010, after Task-007.

---

### Task-001: UI tooling (dev proxy, root npm tasks, component-test setup)

**Priority**: High
**Estimated Iterations**: 1

**Files**:
- `software/lamp-ui/vite.config.ts`
- `software/lamp-ui/package.json` and `package-lock.json`
- root `package.json`

**Steps**:
- **Baseline.** Before changing anything, run `npm run ui:build` and record the
  byte size of `software/lamp-ui/dist/index.html.gz` in `PROGRESS.md` Notes as
  the baseline.
- **Dev proxy.** In `vite.config.ts`, when `process.env.LAMP_HOST` is set, add
  a dev-server proxy:
  - `/api` → `http://${LAMP_HOST}`
  - `/ws` → `ws://${LAMP_HOST}` with `ws: true`
  - both with `changeOrigin: true`

  When `LAMP_HOST` is unset, the config must behave exactly as today. The proxy
  must not affect `vite build` output. `App.vue` already connects to
  `ws://${location.host}/ws`, so no App change is needed.
- **Root npm tasks.** Add to the root `package.json`:
  - `ui:dev` → `cd software/lamp-ui && npm run dev` (inherits `LAMP_HOST`)
  - `ui:test` → `cd software/lamp-ui && npm test`
  - A `//ui:dev` doc string in the style of the existing `//lamp:flash` entry.
    It should say: run with `LAMP_HOST=<lamp ip>` to drive a real lamp. The
    laptop must be joined to the lamp's setup Wi-Fi (softAP, normally
    `192.168.4.1`). Set "Keep Setup Available" in the Setup tab so the AP
    doesn't expire mid-session.
- **Test deps.** Add `@vue/test-utils` and `jsdom` as exact-pinned
  devDependencies compatible with `vitest@2.1.9`. Suggested versions are
  `@vue/test-utils@2.4.6` and `jsdom@25.0.1`; verify they install and run.
  Component test files opt in per file with `// @vitest-environment jsdom`.
  Existing tests stay on the default environment.
- **Smoke test.** Add one component smoke test,
  `src/components/BooleanInput.test.ts`: mount `BooleanInput`, click it, and
  assert it emitted `update:modelValue` with `[true]`.

**Acceptance Criteria**:
- [ ] Baseline gzip size recorded in `PROGRESS.md`.
- [ ] `npm run ui:test` passes (existing tests plus the smoke test).
- [ ] `npm run ui:build` passes. Its `type-check` covers `src/**`, test files
      included.
- [ ] With `LAMP_HOST=192.0.2.1 npm run ui:dev` running in the background,
      `curl -s -o /dev/null -w '%{http_code}' localhost:5173/api/settings`
      returns a proxy error (5xx), not the 200 SPA fallback it returns without
      `LAMP_HOST`. Kill the dev server afterwards.
- [ ] `software/lamp-ui/package.json` `dependencies` still contains only `vue`.

**Verification**:

```bash
git rev-parse --show-toplevel && git rev-parse --abbrev-ref HEAD
npm run ui:test
npm run ui:build
```

---

### Task-002: Harmony generator, HSV utilities, spec doc

**Priority**: High
**Estimated Iterations**: 1-2

**Files**:
- `software/lamp-ui/src/utils/colorUtils.ts`
- `software/lamp-ui/src/utils/colorHarmony.ts` (new)
- `software/lamp-ui/src/utils/colorHarmony.test.ts` (new)
- `software/lamp-ui/src/utils/colorHarmony.golden.json` (new)
- `docs/dev/color-harmony.md` (new)

**Steps**:
- **HSV helpers in `colorUtils.ts`.** Add `rgbToHsv(red, green, blue)` →
  `{ h, s, v }` and `hsvToRgb(h, s, v)` → `{ red, green, blue }`, exactly per
  "Math rules". Add a `formatHexww(components: ColorComponents): string`
  (8-digit uppercase) that reuses the existing private `toByteHex`. Leave
  existing functions unchanged.
- **`colorHarmony.ts` exports:**
  - `COLOR_SCHEMES`: a readonly, ordered array of
    `{ id, key, name, description, naturalCount }` matching the Schemes table.
  - `schemeById(id: number | undefined)`: returns the scheme, or Analogous for
    an unknown or undefined id.
  - `harmonyColors(primary: string, schemeId: number, count: number): string[]`
    implementing the derived-color table. Store the table as a single const
    data structure that mirrors the spec. Monochromatic and Analogous may use
    their row rules.
  - `harmonizePalette(palette: string[], schemeId: number, count?: number): string[]`,
    the orientation adapter.
  - A one-line comment pointing to `docs/dev/color-harmony.md`. No walkthrough
    comments in code.
- **Tests in `colorHarmony.test.ts`:**
  - every hand-checked vector;
  - count clamping (0 → 1, 9 → 5);
  - an unknown scheme id behaves as Analogous;
  - `harmonizePalette` keeps the primary at the last index and its output is
    the reverse of `harmonyColors`;
  - `rgbToHsv` / `hsvToRgb` round trips on the primaries below;
  - the golden test.
- **Golden fixture.** `colorHarmony.golden.json` has the shape
  `{ "cases": [{ "primary", "scheme", "count", "expected": [...] }] }`. It
  covers every scheme × count 2–5 for the primaries `#FF000000`, `#3A7BD5C8`,
  `#F4A6B800`, and `#FFFFFF00` (96 cases). The golden test asserts equality.
  When `process.env.UPDATE_HARMONY_GOLDEN` is set, it rewrites the file
  instead. Generate it once that way, then confirm a normal run passes.
- **Spec doc.** `docs/dev/color-harmony.md` contains the full "Color harmony
  spec" section above: schemes, table, math rules, vectors, orientation,
  neutral-primary behavior, and why Analogous is the default. It also gives the
  golden fixture path and its regen command
  (`UPDATE_HARMONY_GOLDEN=1 npm run ui:test`), and states that the Flutter port
  must pass the same fixture. Leave a "Storage" heading for Task-003 to fill.
  Follow the tone of the existing `docs/dev/*.md` files.

**Acceptance Criteria**:
- [ ] All hand-checked vectors pass.
- [ ] Golden fixture committed with 96 cases. A normal `npm run ui:test`
      asserts against it and passes.
- [ ] `npm run ui:build` passes (type-check).
- [ ] `docs/dev/color-harmony.md` exists and matches the implemented table.

**Verification**:

```bash
npm run ui:test
npm run ui:build
```

---

### Task-003: Firmware stores `autoColors` + `colorScheme` per surface

**Priority**: High
**Estimated Iterations**: 1-2

**Files** (paths relative to `software/lamp-os/src/`):
- `config/config_types.hpp`
- `config/config_codec.cpp`
- `config/config.cpp`
- `components/apply/apply_settings_blob.hpp`
- `software/lamp-os/test/test_config_codec/codec.cpp`
- `docs/dev/color-harmony.md`

**Steps**:
- **Settings classes.** `ShadeSettings` and `BaseSettings` gain
  `bool autoColors = false;` and `uint8_t colorScheme = 0;`. State the contract
  once, on the declaration: client-owned, stored verbatim, see
  `docs/dev/color-harmony.md`.
- **Decode (`fromJson`).** Read both keys in the `base` and `shade` nodes, with
  `false` / `0` when absent. For `colorScheme`, read it as an integer and store
  values outside 0–255 as 0. Follow the existing `socialMode` read style.
- **Encode (`toJson`).** Emit both keys in both nodes.
- **BLE sections.** `Config::asBaseJson()` and `Config::asShadeJson()` emit
  both keys.
- **Partial writes.** In `apply::settingsBlobLocal`, inside the existing
  `base` / `shade` branches:
  - apply `autoColors` only when `is<bool>()`;
  - apply `colorScheme` only when it is an integer in 0–255;
  - when absent, leave the field untouched.

  Keep the existing style of the surrounding branches.
- **Trace the web PUT path.** Follow `handlePutSettings` in
  `components/webapp/webapp.cpp` (it persists raw JSON) and confirm the
  in-memory `Config` picks up the new fields, by reload or reboot, so a later
  commit drain (which re-serializes from the structs) can't drop them. Record
  the finding in `PROGRESS.md` Notes. If it reveals a real gap, record a
  blocker instead of redesigning.
- **Native tests** in `test_config_codec/codec.cpp`, each registered with
  `RUN_TEST`:
  - absent keys give `false` / `0` on both surfaces;
  - a round trip preserves shade `true` / `3` and base `false` / `5`;
  - `colorScheme: 200` round-trips verbatim;
  - `colorScheme: 300` and `-1` load as 0.
- **No harness for `settingsBlobLocal`.** It depends on `::config` and the
  render path. If it can't be tested natively without a large harness, say so
  in `PROGRESS.md` Notes rather than building one.
- **Docs.** Fill the "Storage" section of `docs/dev/color-harmony.md` with the
  keys, defaults, and the four paths from "Firmware storage contract". Also
  check `docs/dev/networking.md`, `docs/dev/lamp-framework.md`, and
  `docs/dev/building-custom-lamps.md`. If any of them list the base/shade
  settings keys, update them.
- **Commit message.** It must say: "settings payload only; no protocol or GATT
  change."

**Acceptance Criteria**:
- [ ] New codec tests pass. The full native suite stays green.
- [ ] Both variants build.
- [ ] `PROGRESS.md` Notes record the PUT-path finding and whether
      `settingsBlobLocal` is natively testable.
- [ ] No change to `header.hpp` protocol constants or `gatt_layout.hpp`.

**Verification**:

```bash
git rev-parse --show-toplevel && git rev-parse --abbrev-ref HEAD
npm run lamp:test
npm run lamp:build
VARIANT=snafu npm run lamp:build
```

---

### Task-004: Color wheel in the shared ColorPicker (superseded by Task-008)

**Priority**: High
**Estimated Iterations**: 2-3

**Files**:
- `software/lamp-ui/src/components/ColorWheel.vue` (new)
- `software/lamp-ui/src/components/ColorPicker.vue`
- tests alongside them

**Steps**:
- **Component API.** `ColorWheel.vue` takes props `hue` (0–360),
  `saturation` (0–1), and `disabled?`. It emits
  `change: [{ hue: number; saturation: number }]` as a single event per update,
  so one drag tick causes one preview send.
- **Rendering, CSS only (no canvas, no library).**
  - A square container `min(240px, 64vw)` holding a circular disc.
  - The disc background is
    `radial-gradient(circle closest-side, #fff, rgba(255,255,255,0))` layered
    over `conic-gradient(#f00, #ff0, #0f0, #0ff, #00f, #f0f, #f00)`.
  - Hue 0 (red) sits at the top and increases clockwise, which matches the
    conic-gradient default.
  - A ring-style thumb sits at angle = hue, radius = saturation × R, filled
    with the current hue/saturation at full brightness.
- **Pointer input.**
  - `pointerdown` on the disc sets the value and calls `setPointerCapture`.
  - `pointermove` while captured updates the value.
  - `pointerup` / `pointercancel` release the capture.
  - Set `touch-action: none` so mobile pages don't scroll mid-drag.
- **Math.** Export pure functions and unit-test them:
  - `pointToHueSat(dx, dy, radius)`: hue = `(atan2(dx, −dy)` in degrees
    `+ 360) % 360`; saturation = `min(1, distance / radius)`.
  - `hueSatToPoint(hue, saturation, radius)`.
- **Keyboard.** The wheel is focusable.
  - ArrowRight / ArrowLeft change hue by ±5°, wrapping.
  - ArrowUp / ArrowDown change saturation by ±0.05, clamped.
  - Use `aria-label="Color wheel"` and
    `aria-valuetext="Hue N°, saturation N%"`.
- **`disabled`** blocks interaction and reduces opacity, like the other inputs.
- **ColorPicker layout.** The dialog body order becomes:
  1. large `ColorPreview`
  2. `ColorWheel`
  3. a "Brightness" `NumberSlider` (0–100, `append="%"`)
  4. the existing hex input
  5. the existing R / G / B / Warm White sliders, unchanged
- **ColorPicker sync.**
  - A wheel change or a Brightness change sets RGB to
    `hsvToRgb(hue, sat, brightness / 100)`, leaves WW unchanged, and then runs
    the existing `updateColor()` path. Live preview and the hex field then
    behave exactly as they do for slider drags.
  - An RGB slider change, a hex input change, or opening the dialog recomputes
    hue / saturation / brightness via `rgbToHsv`.
  - When `s == 0` or `v == 0`, hue is undefined, so keep the previous hue (the
    thumb must not snap to red). When `v == 0`, also keep the previous
    saturation.
- **Pointer edge cases.**
  - A `pointerdown` on the wheel area blurs the hex input, the same as the
    sliders container does (`blurHexInput`).
  - A wheel drag that ends outside the dialog must not close or revert the
    dialog.
  - Cancel / close revert behavior is unchanged.
- **Tests (jsdom).**
  - The math functions: top = 0°, right = 90°, bottom = 180°, left = 270°;
    center gives saturation 0; beyond the edge clamps to 1.
  - ColorPicker: open the dialog, trigger a wheel `change`, and assert
    `update:modelValue` emits the expected `#RRGGBBWW` with WW preserved. Stub
    `Teleport` via `global: { stubs: { teleport: true } }`.

**Acceptance Criteria**:
- [ ] Every color swatch (Shade, Base, and expression colors) opens a dialog
      with the wheel, because they all share `ColorPicker.vue`.
- [ ] Hex, RGB sliders, wheel, and Brightness stay in sync in both directions.
      WW is never changed by the wheel or Brightness.
- [ ] Tests above pass. `npm run ui:build` passes.
- [ ] The gzip size after this task is recorded in `PROGRESS.md` next to the
      baseline.

**Verification**:

```bash
npm run ui:test
npm run ui:build
```

---

### Task-005: Auto-pick toggle on the Home tab (toggle moved by Task-009)

**Priority**: High
**Estimated Iterations**: 2-3

**Files**:
- `software/lamp-ui/src/types.ts`
- `software/lamp-ui/src/App.vue`
- `software/lamp-ui/src/components/ColorGradient.vue`
- tests

**Steps**:
- **Types.** `BaseConfig` and `ShadeConfig` gain `autoColors?: boolean` and
  `colorScheme?: number`, with a one-line comment pointing to
  `docs/dev/color-harmony.md`.
- **Defaults in `App.vue`.**
  - Add both fields (`false`, `0`) to the initial `cfg` value for both
    surfaces.
  - In `onMounted`, after `cfg.value = data`, default any absent field to
    `false` / `0` on both surfaces. This must happen **before** the
    change-detection baseline (`originalSettings.value = editSnapshot()`) is
    taken, so a fresh load doesn't show "Preview, not saved".
- **New `ColorGradient.vue` API.**
  - Props: `autoColors?: boolean` (default `false`) and `scheme?: number`
    (default `0`).
  - Emits: `update:autoColors: [boolean]` and `open-scheme-settings: []`.
- **Render order, top to bottom:**
  1. the primary row (last array index);
  2. the **auto-pick row**;
  3. the remaining rows (array index N−2 down to 0).
- **The auto-pick row.**
  - A `BooleanInput` labeled "Auto-pick colors".
  - Helper text: `<scheme name> · change in Setup`, using
    `schemeById(scheme).name`. "change in Setup" is a button-styled text link
    that emits `open-scheme-settings`.
  - When auto is on and the primary is neutral (`rgbToHsv` gives `s == 0` or
    `v == 0`), the helper instead reads: "Pick a more colorful primary to vary
    the other colors."
  - The row is always visible, even with one color.
- **Behavior.** Implement D3–D5. All regeneration goes through
  `harmonizePalette` inside event handlers: primary `update:modelValue`, toggle
  on, add, remove. There must be **no** watcher that regenerates.
  - The existing `watch(() => props.modelValue, …)` that syncs `localColors`
    stays, and must not trigger generation.
  - The existing `disabled` prop (WebSocket disconnected) still disables
    everything, the toggle included.
  - With auto off, all existing behavior is unchanged.
- **App bindings.** On both `ColorGradient`s:
  - `v-model:auto-colors="cfg.shade.autoColors"` (and `cfg.base.autoColors`);
  - `:scheme="cfg.shade.colorScheme ?? 0"` (and the base equivalent);
  - `@open-scheme-settings="activeTab = 'lamp-setup'"`.

  `cfg` is already the PUT body and is part of `editSnapshot()`, so Save and
  `hasChanges` cover the new fields with no further change.
- **Component tests (jsdom) for `ColorGradient`:**
  1. Toggle on with 1 color emits a palette of the scheme's natural count,
     with the primary at the last index.
  2. Toggle on with 4 manual colors overwrites colors 2–4 and keeps the count
     at 4.
  3. Toggle off emits `update:autoColors(false)` and no palette change.
  4. A primary update while auto is on regenerates the whole palette.
  5. A primary update while auto is off changes only that index.
  6. Derived-row pickers are disabled while auto is on and enabled while it's
     off.
  7. While auto is on, Add gives count+1 regenerated, Remove on a derived row
     gives count−1 regenerated, and the primary has no Remove.
  8. Clone is disabled while auto is on.
  9. **Load safety.** Changing the `modelValue` or `scheme` prop from the
     parent emits no regenerated palette.

**Acceptance Criteria**:
- [ ] All nine tests pass. `npm run ui:build` passes.
- [ ] A fresh page load against a config with no `autoColors` /
      `colorScheme` keys shows no "Preview, not saved" badge.
- [ ] Shade and Base toggles are independent.

**Verification**:

```bash
npm run ui:test
npm run ui:build
```

---

### Task-006: Color Harmony section on the Setup tab (reworked by Task-009)

**Priority**: Medium
**Estimated Iterations**: 2

**Files**:
- `software/lamp-ui/src/components/SegmentedControl.vue` (new)
- `software/lamp-ui/src/components/ColorSchemePicker.vue` (new)
- `software/lamp-ui/src/components/expressions/ExpressionConfig.vue`
- `software/lamp-ui/src/App.vue`
- tests

**Steps**:
- **Extract `SegmentedControl.vue`.** Take the Shade / Base / Both button group
  out of `ExpressionConfig.vue` (reuse before build).
  - Props: `modelValue: number`, `options: { value: number; label: string }[]`,
    `disabled?`. Emits `update:modelValue`.
  - Move the `.targets` / `.target` CSS into it.
  - Use it for **both** `.targets` groups in `ExpressionConfig.vue` (the
    target group near line 146 and the second group near line 237).
  - Expression behavior and appearance must not change.
- **New `ColorSchemePicker.vue`.**
  - Props: `shadeScheme`, `baseScheme` (numbers); `shadePrimary`,
    `basePrimary` (hexww); `shadeEditable`, `baseEditable`, `disabled`
    (booleans).
  - Emits `pick: [target: 1 | 2 | 3, schemeId: number]`. Target values follow
    the `Expression.target` convention: 1 = Shade, 2 = Base, 3 = Both.
- **Target selector inside `ColorSchemePicker`.**
  - Shown only when both surfaces are editable. If only one is editable, the
    picker edits that one and shows no selector.
  - The initial target is Both when the two schemes match, otherwise Shade.
    It's internal UI state and not stored.
- **Scheme list.** A radio group (`role="radiogroup"`, each option
  `role="radio"` with `aria-checked`). Each option shows:
  - the scheme's name and description;
  - a swatch strip of `harmonyColors(previewPrimary, id, naturalCount)`, where
    `previewPrimary` is the targeted surface's primary (Shade's when the target
    is Both). Reuse `ColorPreview` or `createGradientFromHexww` for the
    swatches.
- **Selection state.**
  - Selected = the targeted surface's scheme via `schemeById`, so an unknown id
    shows Analogous selected.
  - Target Both with different schemes: no option selected, plus the note
    "Shade uses X, Base uses Y. Picking a scheme sets both."
  - Export a pure helper,
    `schemeSelection(shadeScheme, baseScheme, target) → { selectedId: number | null }`,
    and unit-test it.
- **App section placement.** Add a Setup section with
  `<h1>Color Harmony</h1>`, between "At-Home Mode" and "Lamp Base LED Profile".
  Use an existing heading color class that differs from both neighbors.
  - Help text: "The scheme Auto-pick colors uses on the Home tab."
  - Hidden when neither surface's colors are editable (`shadeColorsEditable` /
    `baseColorsEditable`).
- **App `pick` handler.** For each targeted, editable surface:
  - set `cfg.<surface>.colorScheme = id`;
  - if that surface's `autoColors` is on, replace its palette (`shadeColors` /
    `baseColors`) with `harmonizePalette(current, id)`.

  The existing preview watchers then show the change on the lamp, and the form
  is marked as changed.

**Acceptance Criteria**:
- [ ] Picking a scheme with target Both updates both surfaces' `colorScheme`.
      Surfaces with auto on regenerate; surfaces with auto off keep their colors.
- [ ] Mixed schemes under Both show no selection plus the note.
- [ ] On a lamp with a fixed (non-editable) shade, the selector is hidden and
      only Base is edited. With neither editable, the section is hidden.
- [ ] The expression target control looks and behaves as before.
- [ ] Tests pass for: `SegmentedControl` emits; `schemeSelection`;
      `ColorSchemePicker`'s initial target, pick emits, mixed state, and
      single-editable case.
- [ ] `npm run ui:build` passes.

**Verification**:

```bash
npm run ui:test
npm run ui:build
```

---

### Task-007: Integration check + docs index

**Priority**: Medium
**Estimated Iterations**: 1

**Steps**:
- **Run every check:**
  - `npm run ui:test`
  - `npm run ui:build`
  - `npm run lamp:test`
  - `npm run lamp:build`
  - `VARIANT=snafu npm run lamp:build`
  - `npm run lamp:buildfs` (proves the SPIFFS image builds with the larger UI)
- **Bundle size.** Record the final `dist/index.html.gz` size against the
  baseline in `PROGRESS.md`.
- **Docs index.** Add `color-harmony.md` to `docs/dev/README.md`'s index.
  Update `docs/dev/architecture.md` only if it describes web-UI features at
  this level of detail.
- **Comment-convention sweep.** Over `git diff ca6bebcd..HEAD` (the base commit
  recorded in `PROGRESS.md`):
  - grep added lines for `Note:`, `Why:`, ` we `, `previously`, `for now`,
    `placeholder`, `Phase `, em-dashes (`—`), `TODO` without a concrete tag,
    and `/Users/`;
  - fix every hit in code comments. This PRD and `PROGRESS.md` are exempt.
- **Dependencies.** Confirm `software/lamp-ui/package.json` `dependencies` is
  still only `vue`.

**Acceptance Criteria**:
- [ ] All six commands pass.
- [ ] Sweep is clean.
- [ ] `PROGRESS.md` shows every task complete, with bundle sizes recorded.

---

### Task-008: Remove the color wheel and Brightness slider (R1)

**Priority**: High
**Estimated Iterations**: 1

**Files**:
- `software/lamp-ui/src/components/ColorPicker.vue`
- `software/lamp-ui/src/components/ColorPicker.test.ts`
- `software/lamp-ui/src/components/ColorWheel.vue` (delete)
- `software/lamp-ui/src/components/ColorWheel.test.ts` (delete)

**Steps**:
- **Remove the wheel section.** Take out `ColorWheel`, the Brightness
  `NumberSlider`, and everything only they use: the hue / saturation /
  brightness state, the HSV sync helpers, the wheel and Brightness handlers, and
  their CSS. `git rm` `ColorWheel.vue` and `ColorWheel.test.ts`.
- **Pre-wheel layout.** The dialog body becomes the large `ColorPreview`, the
  hex input, then the R / G / B / Warm White sliders, as at `ca6bebcd`
  (`git show ca6bebcd:software/lamp-ui/src/components/ColorPicker.vue`).
- **Keep two fixes from this branch.** The overlay rule (close only when the
  pointerdown and the click both target the overlay). And one slider handler
  for all four channels that ignores a value equal to the current one:
  `NumberSlider` re-emits `update:modelValue` when its prop changes
  programmatically (a hex edit), so without the guard a hex edit emits once per
  changed channel.
- **Leave `rgbToHsv` / `hsvToRgb`** in `colorUtils.ts`; the generator and the
  neutral-primary check use them.
- **Tests.** Drop the wheel and Brightness tests. Keep or adapt tests for what
  remains, with the real `Teleport` where an echo could hide behind the stub:
  slider and hex emits, WW preserved, Cancel / close revert, a hex edit emits
  once, and a drag released outside the dialog neither closes nor reverts it.
- **Bundle.** Record the gzip size after this task in `PROGRESS.md`.

**Acceptance Criteria**:
- [ ] `grep -rn "ColorWheel\|color-brightness\|syncHsvFromRgb\|applyHsv" software/lamp-ui/src`
      finds nothing, and both ColorWheel files are deleted.
- [ ] The dialog body is the large preview, the hex input, and the four
      sliders, in that order.
- [ ] Tests cover the overlay rule, a single emit per hex edit, WW
      preservation, and Cancel / close revert.
- [ ] `npm run ui:test` and `npm run ui:build` pass; gzip size recorded.

**Verification**:

```bash
npm run ui:test
npm run ui:build
```

---

### Task-009: Auto-pick moves to Setup: chips, one scheme, collapsible list (R2-R6)

**Priority**: High
**Estimated Iterations**: 2-3

**Files**:
- `software/lamp-ui/src/components/SegmentedControl.vue`
- `software/lamp-ui/src/components/ColorSchemePicker.vue`
- `software/lamp-ui/src/components/ColorGradient.vue`
- `software/lamp-ui/src/App.vue`
- tests alongside them

**Steps**:
- **SegmentedControl multi-select.** Add a multi-select mode: the model is the
  array of selected values, each button toggles its own value, and buttons
  carry `aria-pressed`. Single-select markup, CSS, and behavior stay exactly as
  they are, so `ExpressionConfig.vue` renders and behaves identically.
- **ColorSchemePicker becomes the Color Harmony control.**
  - Inputs: each surface's `autoColors`, `colorScheme`, primary (the last
    palette index), and editability, plus `disabled`.
  - Emits: one event to set a surface's Auto-pick (surface value
    `TARGET_SHADE` / `TARGET_BASE` and on / off), and one to pick a scheme id.
  - "Auto-pick colors for" with the multi-select `SegmentedControl`, one chip
    per editable surface (R2).
  - The R5 summary row, collapsed by default, and the existing radiogroup list
    when open. Reuse the look of `ExpressionsList.vue`'s Configure / Hide
    button for Change / Hide.
  - Selection per R3: the shared scheme via `schemeById`; with one editable
    surface, that surface's scheme; null plus the mixed note when both are
    editable and differ. Rework `schemeSelection` to this rule (no target
    parameter) and keep it exported and unit-tested.
  - Swatch primary: the first surface whose chip is on (Shade before Base);
    with none on, Shade's primary when Shade is editable, else Base's.
  - Remove the Shade / Base / Both selector.
- **App.vue.**
  - Keep the section's `<h1>Color Harmony</h1>` and placement. The info-text
    callout reads: "Auto-pick fills in a surface's other colors from its top
    color. Pick the surfaces, then the scheme."
  - Chip handler: set `cfg.<surface>.autoColors`; turning on applies R4 via
    `harmonizePalette`; turning off changes nothing else.
  - Pick handler: write the id to `colorScheme` on every editable surface, then
    replace the palette of each surface whose `autoColors` is on with
    `harmonizePalette(current, id)`.
  - Home bindings: pass `:auto-colors="cfg.<surface>.autoColors ?? false"`
    (no `v-model`), plus `:scheme` and `@open-scheme-settings` as now.
- **ColorGradient.vue.**
  - Remove the auto-pick row: the switch, its helper text, `setAutoColors`, the
    `update:autoColors` emit, and anything left unused.
  - Add the R6 note. Keep D3's handler-only regeneration and all D5 behavior.
- **Tests.**
  - SegmentedControl: multi-select toggles values and sets `aria-pressed`;
    single-select unchanged.
  - Color Harmony control: a chip per editable surface emits the surface and
    on / off; the summary starts collapsed, Change opens, Hide closes; a pick
    emits the id and keeps the list open; mixed state; single-editable case;
    `schemeSelection`.
  - ColorGradient: no toggle row; the note shows only while `autoColors` is on
    and links to Setup; neutral copy; the disabled prop disables the link.
    Rewrite the existing tests that relied on the toggle to set the
    `autoColors` prop instead.
  - R4 with a real handler: turning a chip on with one color raises the count to
    the natural count; turning it off keeps the colors (App-level, or the
    control plus an extracted pure helper).

**Acceptance Criteria**:
- [ ] The Home tab has no Auto-pick toggle. A surface with Auto-pick on shows
      the one-line note and locked derived rows; with it off, no note.
- [ ] Setup's Color Harmony section shows independent Shade / Base chips (only
      for editable surfaces), a collapsed-by-default scheme summary with
      Change / Hide, and the scheme list when open.
- [ ] A pick writes the scheme to every editable surface and regenerates only
      surfaces with Auto-pick on. A chip turned on applies D4; off keeps colors.
- [ ] The expression target controls look and behave as before.
- [ ] The tests above pass. `npm run ui:test` and `npm run ui:build` pass.

**Verification**:

```bash
npm run ui:test
npm run ui:build
```

---

### Task-010: Revision docs and integration check

**Priority**: Medium
**Estimated Iterations**: 1

**Steps**:
- **Docs.** Wherever `docs/dev/` describes where Auto-pick and the scheme live,
  the regeneration triggers, or the picker's controls, update it to Revision 1.
  The generator spec, golden fixture, and storage contract stay.
- **Run every check** from Task-007 (firmware tasks with the `DEVELOPER_DIR`
  prefix `PROGRESS.md` records) plus the golden cross-check.
- **Bundle.** Record the final gzip size next to the earlier sizes.
- **Sweep.** Run Task-007's comment-convention sweep over the revision's diff
  (`git diff 878e12ef..HEAD`), and check no code or doc still mentions
  `ColorWheel`, the Home toggle, or the Shade / Base / Both scheme lens.
- **Dependencies.** `software/lamp-ui/package.json` `dependencies` is still
  only `vue`.

**Acceptance Criteria**:
- [ ] All six Task-007 commands and the golden cross-check pass.
- [ ] `docs/dev/` matches the shipped UI; the sweep is clean.
- [ ] `PROGRESS.md` shows Tasks 008-010 complete with the final bundle size.

---

## Manual verification on a lamp (for the user, after Task-010)

Executors cannot do this step. It's listed so the user can sign off.

1. **Flash.** `PORT=/dev/cu.usbserial-N npm run lamp:flash` flashes the dev
   firmware and the SPIFFS UI. For hot-reload iteration instead, join the
   lamp's setup Wi-Fi and run `LAMP_HOST=192.168.4.1 npm run ui:dev`.
2. **Picker.** The dialog shows the preview, hex, and R / G / B / Warm White
   sliders, with no wheel. Dragging previews live on the lamp. Cancel reverts
   the lamp. A slider drag released outside the dialog doesn't close it.
3. **Auto-pick (Setup chips).**
   - Turn on the Shade chip with one Shade color: the count rises to the
     scheme's natural count and the colors fill in. On Home, the derived rows
     lock and the "Auto-picked · <scheme>" note shows.
   - On Home, drag the Shade primary: every Shade color follows live.
   - Turn the chip off: the colors stay and the note goes away.
   - Add / remove on Home while it's on: regenerates.
   - Shade and Base chips are independent: try each alone and both.
4. **Scheme list.** Collapsed by default; Change opens it, Hide closes it. A
   pick sets both surfaces' scheme; surfaces with Auto-pick on update live. The
   note's scheme link on Home jumps to Setup.
5. **Persistence.**
   - Save, then reload: the chips, scheme, and colors all persist.
   - A reload never alters the colors by itself.
6. **Fixed shade.** No shipping variant has a fixed shade (snafu has no web
   UI), so the single-editable-surface path is covered by component tests only.
7. **Mixed fleet.** A lamp on older firmware ignores the new keys without
   errors. The toggle and scheme just reset to defaults after a reload.
8. **Judge the looks.** Note which schemes blend muddy between stops on real
   LEDs. If you tune the table, change `docs/dev/color-harmony.md` and the
   const together, then regenerate the golden with
   `UPDATE_HARMONY_GOLDEN=1 npm run ui:test`.

## Technical constraints

- **UI.** Vue 3.5 `<script setup lang="ts">`, Vite 4, TypeScript 5.9, vitest 2.
  Match the existing file style: 2-space indent, single quotes, no semicolons,
  scoped CSS using the existing `--brand-*` / `--color-*` variables, and the
  480 px mobile breakpoint. Don't leave `console.log` behind.
- **Firmware.** C++ / Arduino-ESP32 / ArduinoJson, native Unity tests via
  PlatformIO. Definitions go in `.cpp` unless there's a header-only reason.
  No speculative API. Framework code must not include variant headers.
- **Heap.** The new fields add a few bytes to the config and section JSON. Add
  no new per-tick or roster-style allocations (see `docs/dev/embedded-heap.md`).

## Conventions summary (from CLAUDE.md, enforced in review)

- **Comments.**
  - Default to none. Write one only for a non-obvious WHY, in about 2 lines,
    terse.
  - No `we`, no attention labels (`Note:`, `Why:`), no em-dashes.
  - No history narration, no hedges (`for now`, `just in case`), no
    forward-leaning prose (`placeholder`, `reserved for`).
  - State a contract once, on the primary declaration. Anything longer belongs
    in `docs/dev/color-harmony.md` behind a one-line pointer.
- **References** point only to in-repo `docs/dev/…` files that exist.
- **Reuse before you build.** Use `parseHexww`, `toByteHex`, `ColorPreview`,
  `BooleanInput`, `NumberSlider`, `FormField`, and the extracted
  `SegmentedControl`.

## Out of scope

- The Flutter app (phase 2). It will read the same JSON keys and port
  `colorHarmony.ts` against `colorHarmony.golden.json`.
- Auto-pick for expression colors. They get the wheel, not the toggle.
- Generating colors in firmware, or broadcasting the scheme over the mesh /
  HELLO.
- A `VERSION` bump, a `CHANGELOG` entry, or any release.
- Per-segment palettes. The UI edits one palette per surface.
