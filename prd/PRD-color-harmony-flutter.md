# Feature: Color Harmony Auto-Pick, phase 2 (Flutter app)

> **Status: active since 2026-10-15.** Phase 1 is `prd/PRD-color-harmony.md`
> (web UI + firmware storage), complete through Task-010 and Revision 1, on
> branch `feature/color-harmony`. The user started phase 2 before signing off
> the phase-1 manual lamp check and before the P1 bench check; `PROGRESS.md`
> Notes records both decisions. The SDK gate holds: Flutter 3.47.7 with
> Dart 3.13.5 (`pubspec.yaml` requires `sdk: ^3.12.0`).
>
> **Executor.** This PRD is written for Claude Opus 5.5 running as
> `ralph-executor`, one task per iteration, with reviewers gating each task.
> Tasks are sized for that loop: each names its files, its tests and its
> verification commands, and none needs a design decision.
>
> **Review amendments (2026-10-15).** A verified review against the code
> corrected identifiers, seeds, harness notes and test expectations, and closed
> two design gaps: the failure path of the Color harmony writes, and one
> settings_blob write per user action. The text below includes them and is
> authoritative.
>
> Never edit `prd/PRD-color-harmony.md`.

## Overview

Phase 1 shipped Auto-pick in the lamp's web UI and the two firmware keys that
store it. Its Revision 1 (settled with the user on 2026-10-14) fixed the shape
of the feature: **no color wheel**; Auto-pick is configured on the **Setup** tab
as independent **Shade** and **Base** chips with **one shared, collapsible
scheme list**; the Home tab only shows a compact note while a surface is on.
Phase 2 brings that same shape to the Flutter app, using the same generator,
golden fixture and firmware keys so both clients agree:

1. **A Color harmony screen under Setup › LEDs.** "Auto-pick colors for" with
   Shade and Base chips, then one scheme shown as a summary row with a Change /
   Hide button over the radio list. A chip change or a scheme pick writes to
   the lamp immediately, like every other Setup control.
2. **A note on the Home cards** while a surface is on: "Auto-picked · <scheme>",
   where the scheme name opens the Color harmony screen.
3. **Locked derived rows in the Shade and Base editors** while a surface is on:
   the top color drives the rest, derived rows can't be edited, and primary
   edits, Add and Remove regenerate the palette.
4. **Read and write the firmware keys** `autoColors` and `colorScheme`, so the
   chips and scheme follow the lamp between the app and the web UI.

Two supporting changes ride along. The Flutter Shade presentation flips to
top-first (P1) so its primary is the first row; the Base list already shows the
top of the base first on lamps whose Base strip is reversed, and its primary
stays the last row. A pre-existing swipe-to-dismiss bug is fixed (P11), because
auto-pick would otherwise leave whole generated palettes uncommitted in lamp
RAM.

The picker is untouched: the Flutter `ColorPickerSheet` already is the hex field
plus R / G / B / Warm White sliders that Revision 1 settled on.

## Success Criteria

- [ ] Tasks F01–F10 complete, each with a Reviewer PASS.
- [ ] `npm run app:test` shows no failure beyond the six F01 baseline failures
      recorded in `PROGRESS.md`.
- [ ] `npm run app:analyze` reports no issues beyond the F01 baseline.
- [ ] The Dart generator passes phase 1's golden fixture (96 cases) and every
      hand-checked vector in `docs/dev/color-harmony.md` (18 at the time of
      writing), and mirrors the TypeScript generator operation for operation.
- [ ] No pub dependency added. No change to firmware, `software/lamp-ui`,
      protocol constants or `kGattLayout`. No codegen run.
- [ ] Manual verification on a lamp (end of this PRD) signed off by the user.

## Working environment (read before every task)

- **Repo and branch.** Main checkout
  `/Users/mikebartlett/Developer/GitHub/lamp-os`. It is NOT a worktree. Phase 2
  commits go on `feature/color-harmony` (the user's choice, 2026-10-15); never
  cut a branch from `main`. Before any build, test, or commit, run
  `git rev-parse --show-toplevel` and `git rev-parse --abbrev-ref HEAD` and
  confirm both. If either is wrong, record a blocker and stop.
- **Shared checkout.** Another session's uncommitted firmware, protocol and
  docs edits live in this working tree, and the git index is shared. Touch only
  `software/lamp-app-flutter/lib/`, `software/lamp-app-flutter/test/` and
  `PROGRESS.md` (F10 adds the docs it names). Re-check the branch right before
  committing, and commit with an explicit pathspec
  (`git commit -m "..." -- <your files> PROGRESS.md`) so nothing staged by
  someone else rides along.
- **This PRD.** The working-tree copy is authoritative. `HEAD` may hold an
  older plan whose task numbers mean different work; never read the PRD
  through git.
- **Flutter SDK.** The bundled Dart must be >= 3.12.0 (`flutter --version`
  prints both versions). If `flutter` is missing or too old, record a blocker
  and stop. Never install toolchains yourself.
- **Commands.** Use only the root npm tasks: `npm run app:test` and
  `npm run app:analyze`. Both resolve packages, so don't run raw `flutter pub`
  or `dart` commands. Run a single file with
  `npm run app:test -- test/features/control/domain/color_harmony_test.dart`
  (paths are relative to `software/lamp-app-flutter`). No CI job builds or
  tests the app, so these local runs are the only gate.
- **SDK side effects.** Flutter 3.47.7 rewrites
  `software/lamp-app-flutter/pubspec.lock` and adds `build/**`, `android/**`
  and `ios/**` to the analyzer excludes in
  `software/lamp-app-flutter/analysis_options.yaml` whenever it runs. Never
  stage or revert either file; `PROGRESS.md` Notes records both.
- **No codegen.** No task changes a `@freezed` field list, a `@Riverpod`
  annotation or `build` signature, or a `@JsonSerializable` class. Adding
  methods to `ControlNotifier`, top-level extensions, and hand-written getters
  on the hand-written sections need no codegen. The new route is a
  hand-written `GoRoute` entry in `lib/core/routing/router.dart`.
  `router.g.dart` and `control_notifier.g.dart` carry a debug source hash that
  goes stale when `appRouter` or `ControlNotifier` change; that's expected and
  harmless. Don't run `npm run app:codegen` or `npm run app:clean`; they
  regenerate 33 tracked files. If a task seems to need codegen, record a
  blocker and stop.
- **Staging.** Stage files by path. Never `git add -A` / `git add .`. Never
  stage `.codegraph/` or the root `package-lock.json` (tracked; leave it
  untouched).
- **Baseline failures.** F01 recorded six failing tests and 18 analyzer issues
  outside this feature. "Tests pass" in any task means: the task's new and
  touched tests pass, and the full suite shows no failure beyond the F01
  baseline. Under load the suite has shown timing flakes, so rerun once before
  calling a failure new.
- **Commits.** One commit per task, conventional style, with the task ID:
  `feat(app): Task-F02 port the color harmony generator`. Include
  `PROGRESS.md` in the commit.
- **Read `CLAUDE.md` first.** Its comment and code conventions apply to every
  line. A summary is at the end of this PRD.
- **Don't improvise design changes.** If the code contradicts a task in a way
  that forces a design decision, record it under Blockers in `PROGRESS.md` and
  stop.

## Inherited decisions (from phase 1; binding)

Phase 1 settled these with the user. Revision 1 (R1–R7 in
`prd/PRD-color-harmony.md`) amended some of them; the amended form is what
binds here.

- **D1. The primary is the last stored index.** Generation reads it from the
  last index and keeps it there. On the Shade that is the top. On lamps whose
  Base strip is declared `reversed` (standard, staff, snafu), Base index 0 is
  the top of the base, so the Base primary is its bottom color. Phase 2 keeps
  D1 on both surfaces so both clients anchor the same index (P1).
- **D2. Generation is client-side only.** Firmware stores `autoColors` and
  `colorScheme`. Generated colors are ordinary colors.
- **D3 (as amended by R4). Regenerate only from user-event handlers:** a
  surface's chip turned on in Setup, a scheme pick in Setup, primary edits
  (every live tick and Done), and Add or Remove while on. Never in
  `initState`, `build`, `didUpdateWidget`, or on provider rebuilds. Opening a
  sheet shows the saved colors exactly, even when they don't match the
  generator. Picker Cancel maps to the stops sheet's existing whole-list
  restore.
- **D4 (R4).** Turning a chip on with one color raises the count to the
  scheme's natural count, then overwrites colors 2–N. Turning it off keeps the
  current colors. The web UI's `autoPickPalette` is this rule.
- **D5. While a surface is on:** derived rows stay visible but don't open the
  picker; the primary row has no remove button; remove on a derived row lowers
  the count by one and regenerates; Add raises the count by one (max 5) and
  regenerates. The web UI's Clone has no Flutter equivalent.
- **D6.** Derived colors copy the primary's warm-white byte. Hue math uses RGB
  only.
- **D7 (as amended by R3). One scheme shown, stored per surface.** The UI shows
  one scheme. A pick writes the same id to `colorScheme` on every eligible
  surface, whether or not its chip is on, then regenerates each surface whose
  chip is on. When the eligible surfaces' stored schemes differ (compared after
  mapping unknown ids to Analogous, so D8 holds), nothing is selected and a
  "Mixed" note shows until the user picks.
- **D8.** An unknown scheme id is treated as Analogous for generation and
  display. The stored id is not rewritten unless the user picks a scheme.
- **D10 (superseded by R1). No color wheel and no Brightness slider.** The
  picker is the hex field and the four channel sliders.
- **D11 (Flutter form).** No new pub dependencies.
- **D12.** No `VERSION` bump and no `docs/CHANGELOG.md` entry.
- **D13.** No protocol or GATT change. The keys ride the existing settings_blob
  characteristic and the base / shade section reads.
- **R2. Auto-pick lives in Setup, per surface,** as independent Shade and Base
  chips. The Home tab has no Auto-pick control.
- **R5. The scheme list is collapsible:** a summary row (label "Scheme", the
  selected name or "Mixed", its swatch strip) with a button that reads
  "Change", then "Hide" while the list is open. A pick keeps the list open.
  Open or closed is UI state, never stored.
- **R6. Home shows a compact note while a surface is on:** "Auto-picked ·
  <scheme name>", the name a link to the Setup screen; with a neutral primary,
  "Auto-picked · pick a more colorful top color" with no link. In the app the
  Base's line says "bottom color" (P1).

## Phase-2 decisions

Defaults chosen during planning. **P1 must be confirmed by the user before
activation.** The rest stand unless the user changes them.

- **P1. Top-first orientation for the Shade only (decided 2026-10-15).**
  Today the Flutter app lists index 0 first ("Stop 1") and draws index 0 at the
  top of the Shade/Base cards and the critter preview. Firmware puts palette
  index 0 on logical pixel 0 (`buildGradientWithStops`) and applies a strip's
  `reversed` flag only at flush, and docs/dev/expressions.md (the Pulse row,
  from bench fixes) records that on the standard lamp shade logical 0 is its
  bottom and base logical 0 is its top. The Base is declared `reversed` on the
  standard, staff and snafu lamps. So:
  - **Shade:** index 0 is the bottom. The Flutter Shade sheets, `ShadeCard` and
    the critter preview's shade gradient flip to top-first, so the primary (the
    last index, D1) is the first row and the top of the card.
  - **Base:** index 0 is the top on those lamps, so the Base sheet, `BaseCard`
    and the preview's base gradient already show the top first and stay as
    they are. The Base primary (the last index) is its last row and the bottom
    of its card. Base copy says "bottom color" where Shade copy says "top
    color".
  - The web UI lists the Base's last index first and labels it "Top", which is
    the bottom of the base on those lamps. That is a phase-1 follow-up, out of
    scope here (Known issues).

  The P1 bench check was not run (`PROGRESS.md` Notes); manual verification
  step 2 checks both surfaces. Loaf's Base is not declared reversed and its
  mounting isn't recorded; check it there if one is at hand.
- **P2. Single-segment surfaces only.** The keys are per surface, but a
  multi-segment shade (snafu has three shade segments, each with its own sheet)
  has one palette per segment. A surface is **eligible** for Auto-pick when its
  `colorsEditable` is true and it has at most one segment. The chips, the Home
  note and the editor lock all follow eligibility. The P1 Shade flip applies to
  every Shade sheet, segmented or not. The web UI gates on `colorsEditable`
  alone, so on a
  multi-segment editable surface (the lioness Base) the web shows a chip, a
  note and a Mixed comparison that the app omits. Parity claims below hold on
  single-segment surfaces.
- **P3. Placement: a Setup sub-screen.** The Setup tab is a `SettingsRow` list
  that drills into sub-screens, so the web's inline "Color Harmony" section
  becomes a row "Color harmony" under the LEDs heading, pushing a new
  `ColorHarmonyScreen` at `AppRoutes.colorHarmony(id)`
  (`/lamp/$id/setup/color-harmony`). The row and the screen exist only when at
  least one surface is eligible.
- **P4. Chips are a multi-select `SegmentedButton`.** The app's only
  pill-group idiom is `SegmentedButton` (Advanced LEDs' strip-type and Battery
  Saver pickers, the Motion picker). Shade and Base are its segments, with
  `multiSelectionEnabled: true` and `emptySelectionAllowed: true`, each segment
  present only when that surface is eligible. There is no `Radio` in `lib/`
  and its members are deprecated on this SDK; the scheme list is a column of
  tappable rows with a check icon, as the Web Config picker in
  `setup_screen.dart` does.
- **P5. Persistence is immediate.** Setup controls in this app write as the
  user changes them (`setLampSocialMode` and its neighbours); there is no Save
  button. A chip or scheme change calls the notifier, which updates state
  optimistically and writes **one** settings_blob with `reboot: false` per user
  action (the firmware keeps a single pending settings_blob payload, so two
  back-to-back writes can drop the first). When a change regenerates colors,
  the colors stream over the surface's colors characteristic first, then the
  flags go, then one `commit()` persists the colors, the same path the editors'
  Save uses. If the flag write fails, the flags revert through `_mutate`, the
  pre-regeneration colors stream back (the stops sheet's Cancel restore), no
  commit runs, and the screen shows `AppSnackbar.error`.
- **P6. The editor sheet only locks; it never toggles or persists.** The
  stops sheet learns whether the surface is on and which scheme it uses, so it
  can lock derived rows and regenerate on primary edits, Add and Remove. The
  flags themselves change only on the Color harmony screen. Save in the sheet
  is unchanged.
- **P7. Count cap.** Auto-pick follows the shared clamp of 5. Turning a chip on
  with 6 colors leaves 5. While on, Add hides at 5. With auto off, the manual
  cap stays 6.
- **P8. Swatch preview primary.** The scheme swatches preview the first
  eligible surface whose chip is on, Shade before Base; with no chip on,
  Shade's primary when Shade is eligible, otherwise Base's. Each strip is
  `harmonyColors(primary, id, naturalCount)` reversed, so the primary sits at
  the right end, as in the web UI.
- **P9. Home note placement.** The note is a second line under the card title
  in `ShadeCard` (segment 0 only) and `BaseCard`. The card's tap still opens
  the editor; the scheme name inside the note is its own button that pushes
  the Color harmony screen.
- **P11. Swipe-dismiss reverts.** Swiping down a modified stops sheet reverts,
  the same as Cancel. Today a swipe skips both the discard prompt and the
  revert, so the next unrelated commit (for example a brightness change)
  persists abandoned colors. Auto-pick makes this worse because one drag
  rewrites the whole palette. The fix covers every `ColorStopsSheet` caller.
  The expression Colors sheet and the wisp manual palette feed a draft through
  `onChanged`, so a swipe also drops the sheet's edits from that draft, exactly
  as Cancel already does.

## Shared contract

**Authority order:** the golden fixture, then the TypeScript that generated it,
then `docs/dev/color-harmony.md`, then this summary. If the TypeScript and the
doc disagree, mirror the TypeScript, record the difference in `PROGRESS.md`
Notes, and correct the doc in F10. `software/lamp-ui` stays untouched.

- **Schemes** (id · key · name · natural count):
  - 0 · `analogous` · Analogous · 3 (default)
  - 1 · `monochromatic` · Monochromatic · 3
  - 2 · `complementary` · Complementary · 2
  - 3 · `splitComplementary` · Split complementary · 3
  - 4 · `triadic` · Triadic · 3
  - 5 · `square` · Square · 4

  UI descriptions are the one-liners in the doc.
- **`harmonyColors(primary, scheme, count)`** returns colors top→bottom.
  Index 0 is the primary itself, never round-tripped through HSV. Count clamps
  to [1, 5].
- **`harmonizePalette(palette, scheme, count = palette.length)`** takes storage
  order (bottom→top), reads the primary from the last index, and returns
  `harmonyColors(...)` reversed. `palette` is never empty (firmware always
  stores at least one color); callers must not pass an empty list.
- **`autoPickPalette(palette, scheme, on)`** is the R4 rule: `on == false`
  returns the palette unchanged; `on == true` with one color raises the count to
  the scheme's natural count, otherwise keeps the count, then harmonizes.
- **Golden fixture:** `software/lamp-ui/src/utils/colorHarmony.golden.json`.
  From the Flutter package root (the working directory of `flutter test`) it is
  `../lamp-ui/src/utils/colorHarmony.golden.json`. Read it in place. Don't copy
  it into the app.
- **Firmware keys,** inside both the `base` and `shade` objects:
  - `autoColors`: bool, default false; a non-bool reads as false;
  - `colorScheme`: integer, default 0; anything but an integer in 0–255 loads
    as 0.

  `Config::asBaseJson()` and `Config::asShadeJson()` emit both, so the app's
  section reads carry them. A settings_blob write applies each key only when
  present and valid (`apply::colorHarmonyLocal`), so a write carrying only
  these keys leaves colors, px and byteOrder alone. On `dev`, the firmware
  stores them inside `base` and `shade` in both the whole `cfg` string and the
  split rest blob; the app never sees that layout. Firmware without phase 1
  drops the keys silently and its section reads omit them, so an absent key
  reads as `false` / `0`.
- **Hex case.** The firmware writes lowercase hex and the generator uppercase.
  `LampColor` parses either and `toHex()` is uppercase, so Dart comparisons go
  through `LampColor`, never through strings.

**Porting rules for bit-exact parity.** They apply to the generator (F02). The
golden's four primaries don't exercise every arithmetic choice, so a passing
golden is necessary but not sufficient; the doc's hand-checked vectors each pin
one slip.

- Port operation for operation from the TypeScript: `rgbToHsv` / `hsvToRgb` in
  `software/lamp-ui/src/utils/colorUtils.ts`; `harmonyColors`,
  `harmonizePalette`, `autoPickPalette` and the table in
  `software/lamp-ui/src/utils/colorHarmony.ts`. Read them before writing Dart.
- Mirror every floating-point expression with the same operands in the same
  order: hue normalization, channel assembly (`(ch + m) * 255` and
  `ch * 255 + m * 255` give different bytes), branch order, sector selection,
  and round-then-clamp order. Write each table factor as the same expression
  the TypeScript uses (`1 - k / count`, not a literal fraction).
- The TypeScript's only modulo is its helper `mod(x, m) = ((x % m) + m) % m`,
  used at four sites: `mod((g - b) / d, 6)` and `mod(h, 360)` in `rgbToHsv`,
  and `mod(h, 360)` and `mod(hPrime, 2)` in `hsvToRgb`. Port it once as
  `(x.remainder(m) + m).remainder(m)` and call it at all four sites. Never use
  Dart's `%` on doubles (Euclidean modulo): substituting `x % m` for the helper
  fails `#07010000`, `#00010B00`, `#000B0100` and `#00010500`, and writing the
  helper with Dart's `%` inside fails `#23000500`.
- Byte rounding: `(value).round()` matches `Math.round` for the non-negative
  values involved.
- Compare outputs as `LampColor` values or uppercase `toHex()` strings; never
  compare raw firmware hex strings.

## Test harness notes (F04–F09)

- `topFirst` and `autoPick` aren't on `showColorStopsSheet`, so tests host the
  sheet with
  `showAppSheet(ctx, builder: (_) => FractionallySizedBox(heightFactor: 0.6, child: ColorStopsSheet(...)))`,
  copying `showColorStopsSheet`'s body. A real modal route is needed for Save,
  Cancel and pop behavior.
- The list viewport in a 0.6-height sheet on the 800×600 test surface fits
  about two rows. Before tapping or asserting on a row, call
  `await tester.ensureVisible(finder); await tester.pumpAndSettle();`.
  Alternatively set `tester.view.physicalSize = const Size(800, 1600)` and
  `tester.view.devicePixelRatio = 1`, with `addTearDown(tester.view.reset)`.
- Simulate system back with `await tester.binding.handlePopRoute();`, as the
  existing editor tests do.
- With a picker open over a stops sheet, both routes stay onstage, so
  `find.text('Cancel')` can match twice. Use keys or `.last`.
- Live writes leave 60 ms and 500 ms timers. Pump at least 60 ms before
  asserting a BLE write, and drain about 600 ms before a test ends.
- Screen tests use the seeded `InMemoryBleClient` the way
  `test/features/lamp_shell/setup_screen_test.dart` and
  `advanced_leds_screen_test.dart` do. Both of those add the `InventoryLamp`
  with `controlPassword: 'secret'`, so their settings_blob writes are
  encrypted. For tests that decode settings_blob payloads, add the
  `InventoryLamp` without `controlPassword` (the
  `test/features/lamp_shell/battery_saver_test.dart` precedent), so each write
  is `[0x01 | json]` and decodes the way
  `test/features/control/write_settings_blob_test.dart` does. With a password,
  decode with `LampCrypto.decryptOpForTesting(write, password: 'secret', saltUuid16: uuidSaltLE16(BleUuids.settingsBlob), charShortName: 'settingsBlob')`.
  `ble.writesTo(...)` is per characteristic, so don't assert order across
  characteristics.
- Fail a settings_blob write with
  `ble.scheduleEncryptionFailure(devId, BleUuids.controlService, BleUuids.settingsBlob)`
  (precedent `test/features/control/expression_op_revert_test.dart`); the next
  write to it throws and `_mutate` reverts. Don't disconnect the client to
  simulate failure; that starts the reconnect ladder.
- The default shade seed is `["#000000FF"]`, which is neutral (v == 0), so it
  harmonizes to copies of itself and the Home note shows the neutral line.
  Tests that need a scheme-name link or a visibly regenerated shade palette
  seed a colorful `shadeColorsJson`, for example `'["#FF0000C8"]'`. The default
  base seed `#300783FF` is colorful.
- Navigation: `setup_screen_test.dart` and the card tests host widgets in a
  plain `MaterialApp`, which can't follow `context.push`. Tests that follow a
  route build a `GoRouter` as `test/features/control/presentation/control_screen_test.dart`
  does: the host route (a `Scaffold` around the screen or card) as
  `initialLocation`, plus a stub
  `GoRoute(path: '/lamp/:id/setup/color-harmony', builder: (_, _) => const Scaffold(body: Text('harmony-stub')))`.
  Tap, `pumpAndSettle`, and expect `find.text('harmony-stub')`. Don't assert
  `routerDelegate.currentConfiguration.uri` after a push; imperative pushes
  may not change it.
- With its list open, the Color harmony screen is taller than the 800×600 test
  surface. Use `ensureVisible` / `scrollUntilVisible`, or enlarge
  `tester.view.physicalSize` as above.
- Write prescribed keys as `const Key('...')`; `prefer_const_constructors`
  reports a non-const `Key` as an info, which fails `app:analyze`.
- Reconnect tests (F09) stay plain `test()` with `Future.delayed(Duration.zero)`
  pumps, as `reconnect_mirrors_firmware_test.dart` does.

## Tasks

Order: F01 → F02 → F03 → F04 → F05 → F06 → F07 → F08 → F09 → F10, strictly
sequential in this one working tree (each task edits `PROGRESS.md` and runs the
whole suite). F05 and F06 touch the same sheet, so F05 lands first. F08 needs
F02, F03 and F07 (it pushes `AppRoutes.colorHarmony`). F09 needs F06 and F07.

---

### Task-F01: Toolchain and prerequisite gate, baseline

**Priority**: High
**Estimated Iterations**: 1

**Steps**:
- Confirm repo and branch (see Working environment). Confirm the branch
  contains `origin/dev`: `git merge-base --is-ancestor origin/dev HEAD` must
  succeed. If it doesn't, record the blocker "branch is not on dev" and stop.
- `flutter --version` reports Dart >= 3.12.0. If not, record the blocker
  "install a Flutter SDK with Dart >= 3.12 (Flutter 3.44+), e.g.
  `brew install --cask flutter`" and stop.
- Confirm phase 1's outputs exist on this branch:
  - `docs/dev/color-harmony.md` with its Storage and Web UI sections;
  - `software/lamp-ui/src/utils/colorHarmony.golden.json` with 96 cases;
  - `software/lamp-ui/src/utils/colorHarmony.ts` exporting `autoPickPalette`,
    and the HSV helpers in `colorUtils.ts`;
  - `software/lamp-os/src/config/config.cpp` emits `autoColors` and
    `colorScheme` in both `Config::asBaseJson()` and `Config::asShadeJson()`;
  - `software/lamp-os/src/components/apply/apply_color_harmony.hpp` exists.

  If any is missing, record a blocker and stop.
- `PROGRESS.md` Notes must contain a bullet starting `- P1 confirmed by the
  user on` covering both Shade and Base, or a bullet starting
  `- P1 not checked:` recording the user's decision to build with P1
  unconfirmed. If neither exists, record the blocker "P1 bench check not
  recorded" and stop.
- Confirm `lib/` and `test/` under `software/lamp-app-flutter` contain no
  `autoColors` or `harmony` identifiers and no section field named
  `colorScheme` yet (Flutter's `Theme.of(context).colorScheme` appears about
  235 times and doesn't count). If they do, read what exists and record it in
  Notes before continuing.
- From the repo root, run `npm run app:analyze` and `npm run app:test`.
- Record in `PROGRESS.md` Notes:
  - the Flutter and Dart versions;
  - the current `HEAD` hash as the **phase-2 base commit**;
  - the analyzer issue count and the files involved;
  - any failing tests;
  - whether `pubspec.lock` was rewritten (leave it unstaged).

  These are the baseline. Don't fix unrelated failures.

**Acceptance Criteria**:
- [ ] Baseline and phase-2 base commit recorded in `PROGRESS.md`.
- [ ] No tracked file other than `PROGRESS.md` is staged or committed.

**Verification**:

```bash
git rev-parse --show-toplevel && git rev-parse --abbrev-ref HEAD
git merge-base --is-ancestor origin/dev HEAD && echo on-dev
flutter --version
npm run app:analyze
npm run app:test
```

---

### Task-F02: Port the harmony generator to Dart

**Priority**: High
**Estimated Iterations**: 1-2

**Files**:
- `software/lamp-app-flutter/lib/features/control/domain/color_harmony.dart` (new)
- `software/lamp-app-flutter/test/features/control/domain/color_harmony_test.dart` (new)

**Steps**:
- Read `colorUtils.ts` and `colorHarmony.ts` first, then port them per the
  porting rules in "Shared contract".
- Pure Dart. Import only `lamp_color.dart` (and `dart:math` if the TypeScript
  needs it). No widget or theme imports.
- **Exports:**
  - `class HarmonyScheme` with `id`, `key`, `name`, `description`,
    `naturalCount`;
  - `const List<HarmonyScheme> kHarmonySchemes`, in id order;
  - `HarmonyScheme harmonySchemeById(int? id)`, returning Analogous for null or
    unknown ids;
  - `({double h, double s, double v}) rgbToHsv(int r, int g, int b)` and
    `({int r, int g, int b}) hsvToRgb(double h, double s, double v)`;
  - `List<LampColor> harmonyColors(LampColor primary, int schemeId, int count)`:
    top→bottom; index 0 is the same `primary` value; derived entries copy
    `primary.w`;
  - `List<LampColor> harmonizePalette(List<LampColor> storageOrder, int schemeId, {int? count})`;
  - `List<LampColor> autoPickPalette(List<LampColor> storageOrder, int schemeId, {required bool on})`:
    the R4 rule; `on == false` returns the same list instance;
  - `bool isNeutralColor(LampColor c)`: true when `rgbToHsv` gives `s == 0` or
    `v == 0`;
  - `int? sharedSchemeId({required int shadeScheme, required int baseScheme, required bool shadeEligible, required bool baseEligible})`:
    the R3 selection rule. One eligible surface returns its scheme's id
    (after `harmonySchemeById`); two return the common id, or null when they
    differ; none returns null.
- Store the derived-color table as one const structure in the same shape as the
  TypeScript's.
- **Comments** cite only `docs/dev/color-harmony.md`: one pointer line, plus one
  line on the HSV helpers saying they follow the spec's math exactly so outputs
  match the web client (the reason `HSVColor` isn't used). Never name the
  TypeScript files or other web-client paths in comments. No walkthroughs.
- **Tests:**
  - every hand-checked vector in the doc's "Hand-checked vectors" table, read
    from the doc by the executor at the time of writing (18 rows as of
    2026-10-14);
  - the golden fixture: read `../lamp-ui/src/utils/colorHarmony.golden.json`
    with `dart:io`; fail with a message naming the absolute path if it's
    missing; assert every case, passing its numeric `scheme` straight to
    `harmonyColors` (the doc vectors' Scheme column holds keys, which map
    through `kHarmonySchemes`);
  - count clamping: 0 gives 1 color, 9 gives 5;
  - unknown id 42 behaves as Analogous;
  - `harmonizePalette` keeps the primary at the last index and equals
    `harmonyColors` reversed;
  - `autoPickPalette`: off returns the identical list; on with one color gives
    the natural count (Analogous 3, Square 4); on with four colors keeps four;
  - W is copied: `#FF0000C8` complementary 2 gives `#FF0000C8 #00FFFFC8`;
  - `isNeutralColor` is true for `#FFFFFF00`, `#80808000`, `#000000FF`, and
    false for `#FF000000`;
  - `sharedSchemeId`: (0, 99, both eligible) is 0 since 99 maps to Analogous;
    (4, 5, both eligible) is null; (4, 5, shade only) is 4; (none eligible) is
    null by design (the web twin, `schemeSelection`, returns the shade id
    there; neither UI reaches that case);
  - neutral primaries repeat: for `#FFFFFF00`, `#808080A0`, `#00000000` and
    `#000000FF`, `harmonyColors(p, id, 5)` is five copies of `p` for every
    scheme id (black is the only input that reaches `rgbToHsv`'s `max == 0`
    guard);
  - the scheme table: `(id, key, name, naturalCount)` for all six rows matches
    Shared contract in order, and each description matches the doc;
  - the TypeScript's direct HSV cases: the `rgbToHsv` cases, `hsvToRgb` hue
    normalization and byte clamping, and the RGB-grid round-trip.

**Acceptance Criteria**:
- [ ] All 96 golden cases and all hand-checked vectors pass.
- [ ] `PROGRESS.md` Notes list, for each mod, normalization and rounding site,
      the TypeScript expression the Dart mirrors. The Reviewer diffs the Dart
      and TypeScript functions side by side.
- [ ] `npm run app:analyze` adds no issues.

**Verification**:

```bash
npm run app:test -- test/features/control/domain/color_harmony_test.dart
npm run app:analyze
```

---

### Task-F03: Section fields, `setColorHarmony`, seed support

**Priority**: High
**Estimated Iterations**: 1-2

**Files**:
- `software/lamp-app-flutter/lib/features/control/domain/sections.dart`
- `software/lamp-app-flutter/lib/features/control/application/control_notifier.dart`
- `software/lamp-app-flutter/test/_support/seed.dart`
- `software/lamp-app-flutter/test/features/control/domain/sections_test.dart`
- `software/lamp-app-flutter/test/features/control/set_color_harmony_test.dart` (new)

**Steps**:
- **Models.** `BaseSection` and `ShadeSection` are hand-written (not freezed).
  Each gains `final bool autoColors` (default false) and
  `final int colorScheme` (default 0).
  - `fromJson`: `autoColors` from a bool, else false; `colorScheme` from an int
    in 0–255, else 0.
  - Update the constructors, `copyWith`, `==` and `hashCode`.
  - State the contract once, on the declaration, with a pointer to
    `docs/dev/color-harmony.md`.
- **`clearKnockout`.** `ControlNotifier.clearKnockout` is the only code outside
  `sections.dart` that builds a `BaseSection` field by field. The new fields
  have defaults, so the compiler won't flag it. Replace that `BaseSection(...)`
  with `cur.base.copyWith(knockout: const {})`, so a knockout reset keeps
  `autoColors` and `colorScheme` (and the `drawIdleMa` / `drawFullMa` anchors
  it drops today).
- **Don't add the keys to any existing outbound payload,** such as the Advanced
  LED save. `setColorHarmony` is the only writer.
- **Notifier.** Beside `enum EditSurface` in `control_notifier.dart`, add
  `enum HarmonySurface { shade, base }` and
  `Future<void> setColorHarmony({required Set<HarmonySurface> surfaces, bool? autoColors, int? colorScheme})`.
  One call is one user action and sends **one** settings_blob (P5): `_mutate`
  sets the non-null values on every named surface's section optimistically,
  then calls
  `writeSettingsBlob({for (final s in surfaces) <'shade' or 'base'>: {if (autoColors != null) 'autoColors': autoColors, if (colorScheme != null) 'colorScheme': colorScheme}}, reboot: false)`.
  Follow `setLampSocialMode` for the `_mutate` shape. A chip change passes one
  surface and `autoColors`; a scheme pick (F07) passes every eligible surface
  and `colorScheme`, so `colorScheme` is written only when the user picked one
  (D8). Assert that `surfaces` is non-empty and that at least one of the two
  values is non-null.
- **Eligibility (P2).** Add
  `bool get harmonyEligible => colorsEditable && segments.length <= 1;` to
  `BaseSection` and `ShadeSection`, with the contract on the declaration.
  Beside `HarmonySurface`, add
  `extension HarmonyEligibility on ControlState { bool isHarmonyEligible(HarmonySurface surface) => switch (surface) { HarmonySurface.shade => shade.harmonyEligible, HarmonySurface.base => base.harmonyEligible }; }`.
  A hand-written getter and a top-level extension need no codegen; a member on
  the `@freezed` `ControlState` would, because its generated `_ControlState`
  implements the class. Code that watches one section uses the getter; code
  holding a `ControlState` uses the extension. `color_harmony.dart` stays free
  of both.
- **Seed support.** Give `seedControlBle` optional `bool? shadeAutoColors`,
  `int? shadeColorScheme`, `bool? baseAutoColors`, `int? baseColorScheme`,
  `bool? shadeColorsEditable` and `bool? baseColorsEditable`, each emitted into
  the shade / base JSON (as `autoColors`, `colorScheme`, `colorsEditable`) only
  when non-null, so existing seeds are byte-for-byte unchanged.
- **Tests:**
  - parsing: absent gives false / 0; `true` / `3` parse as `true` / `3`;
    `300`, `-1` and `"2"` load as 0; a non-bool `autoColors` (`1`, `"true"`)
    loads as false; `==`, `hashCode` and `copyWith` cover the new fields.
    Don't add `toJson`;
  - `setColorHarmony(surfaces: {HarmonySurface.shade}, autoColors: true)`
    sends one settingsBlob write whose decoded payload has a `shade` object
    holding only `autoColors: true`, no `base` object, and `reboot` false;
    `setColorHarmony(surfaces: {HarmonySurface.shade, HarmonySurface.base}, colorScheme: 4)`
    sends one write whose `shade` and `base` objects each hold only
    `colorScheme: 4`;
  - state is updated optimistically, and reverted with the error rethrown when
    the write fails (fail it with `ble.scheduleEncryptionFailure`, see Test
    harness notes; assert with the `mutate_helper_test.dart` pattern);
  - `clearKnockout` keeps the flags: seed base with a non-empty knockout,
    `baseAutoColors: true` and `baseColorScheme: 3`, call `clearKnockout`, and
    assert the knockout is empty while `autoColors`, `colorScheme`,
    `drawIdleMa` and `drawFullMa` are kept;
  - eligibility: a three-segment shade (`shadeSegmentsJson`) is not eligible;
    `baseColorsEditable: false` makes Base ineligible; the default seed is
    eligible on both surfaces; `isHarmonyEligible` agrees with each section's
    `harmonyEligible`.

**Acceptance Criteria**:
- [ ] Tests pass. `npm run app:analyze` adds no issues.
- [ ] No codegen run. No generated file changed.

**Verification**:

```bash
npm run app:test -- test/features/control/domain/sections_test.dart
npm run app:test -- test/features/control/set_color_harmony_test.dart
npm run app:test
npm run app:analyze
```

---

### Task-F04: Top-first orientation for the Shade (P1)

**Priority**: High
**Estimated Iterations**: 2

**Files** (all under
`software/lamp-app-flutter/lib/features/control/presentation/widgets/`):
- `color_stops_sheet.dart`, `shade_editor_sheet.dart`, `shade_card.dart`,
  `lamp_preview.dart`
- their tests, plus `base_card_test.dart` and `base_editor_sheet_test.dart`
  for the Base-unchanged assertions

**Steps**:
- **`ColorStopsSheet` gains `topFirst`** (bool, default false). Don't add it to
  `showColorStopsSheet`; no other caller needs it. Only `ShadeEditorSheet`
  passes `true`, for every Shade sheet including segment sheets.
  `BaseEditorSheet` keeps the default (P1).
- **Display mapping when `topFirst` is true.** Rows render storage index n−1
  first. `_colors` stays in storage order. Map every row operation from display
  index to storage index:
  - tap/edit and remove: `storage = n - 1 - display`;
  - reorder: build the display list (`_colors.reversed.toList()`), apply the
    existing display-space move, then store it reversed back. Don't hand-derive
    index arithmetic;
  - Add (auto off): insert at storage index 0, the bottom row, seeded from the
    current storage index 0. New colors appear at the bottom, as in the web UI.
- Row keys (`color-stop-$i`) and picker titles (`Stop ${i + 1}`) use the
  display position.
- **Previews.** `ShadeCard` and the critter preview's shade gradient render
  storage index 0 at the bottom: reverse the shade list before building the
  gradient or the SVG stops. `BaseCard` and the preview's base gradient stay
  as they are (P1).
- **Preview test seam.** The preview's stops live inside `SvgPicture.string`,
  which a test can't read. Move `_stopTag`, `_buildStops` and `_renderSvg` out
  of the State class as top-level functions, and expose the renderer as
  `@visibleForTesting String renderLampPreviewSvg(String template, List<LampColor> shadeStorage, List<LampColor> baseStorage)`,
  which builds the shade stops from the reversed shade list and the base stops
  from the base list as stored. The widget calls it where it called
  `_renderSvg`; its memoization is unchanged.
- **Tests** (see "Test harness notes"):
  - a `topFirst` sheet with storage `[red, green, blue]` shows blue in row
    `color-stop-0` and red in the last row;
  - dragging display row 0 below display row 1 of storage
    `[red, green, blue]` gives `onChanged` `[red, blue, green]`;
  - removing display row 0 (blue) of storage `[red, green, blue]` gives
    `[red, green]`;
  - Add inserts at storage index 0, seeded from it;
  - `ShadeCard` seeded with storage `[red, blue]` renders a `LinearGradient`
    whose colors are `[blue.toSwatch(), red.toSwatch()]` top to bottom;
    `BaseCard` seeded with storage `[red, blue]` keeps
    `[red.toSwatch(), blue.toSwatch()]`;
  - `renderLampPreviewSvg` with a minimal template holding the two gradient
    blocks its patterns match: shade storage `[red, blue]` puts blue's
    `toRgbHex()` at offset 0% and red's at 100% in the shade gradient; base
    storage `[red, blue]` puts red at 0% and blue at 100% in the base gradient;
  - without `topFirst`, behavior is unchanged (existing tests), and the Base
    editor still lists storage index 0 first;
  - update existing Shade editor tests whose expectations depend on row order
    or Add position. Keep what they verify.

**Acceptance Criteria**:
- [ ] Shade sheets list the top of the shade first; the Base sheet is
      unchanged. `ShadeCard` and the preview's shade gradient match the Shade
      list; `BaseCard` and the base gradient are unchanged. Covered by tests.
- [ ] Expression and wisp sheets are unchanged.
- [ ] Tests pass per Working environment. `npm run app:analyze` adds no
      issues.

**Verification**:

```bash
npm run app:test -- test/features/control/presentation/widgets/
npm run app:test
npm run app:analyze
```

---

### Task-F05: Swipe-dismiss reverts like Cancel (P11)

**Priority**: High
**Estimated Iterations**: 1

**Files**:
- `software/lamp-app-flutter/lib/features/control/presentation/widgets/color_stops_sheet.dart`
- `software/lamp-app-flutter/test/features/control/presentation/widgets/color_stops_sheet_test.dart`

**Steps**:
- Write the modified-sheet swipe test first and run it against the unmodified
  sheet. The sheet must close and only the `onChanged` assertion may fail. If
  the discard dialog appears or the sheet stays open, the premise below doesn't
  hold on this SDK: record a blocker and stop. Leave the existing `_close`
  comment unchanged.
- In the `PopScope` `onPopInvokedWithResult` handler: when `didPop` is true,
  `_allowPop` is false, and the colors differ from the opened list, call
  `onChanged` with the opened list before returning. That is the swipe-down
  path, which pops without consulting `canPop`. Cancel, Save, and back with an
  unmodified sheet behave as today.
- No other behavior changes.

**Tests:**
- Open a sheet through `showColorStopsSheet` (no `topFirst`, no `autoPick`),
  make one edit (for example remove a stop), then swipe starting on the sheet
  title, which sits outside the list:
  `await tester.drag(find.text('<title>'), const Offset(0, 400)); await tester.pumpAndSettle();`.
  Assert the title is gone and the last `onChanged` equals the opened list.
  Don't drag from `find.byType(BottomSheet)`: its center lands on the list,
  whose scroll recognizer wins, so the sheet never moves. Don't change the
  list's physics or controller to make a drag work.
- Swipe on an unmodified sheet: no extra `onChanged`.

**Acceptance Criteria**:
- [ ] Both tests and all existing tests pass. `npm run app:analyze` adds no
      issues.
- [ ] The only behavior change is P11, which also reverts the expression-colors
      and wisp manual-palette drafts on swipe, as Cancel does.

**Verification**:

```bash
npm run app:test -- test/features/control/presentation/widgets/color_stops_sheet_test.dart
npm run app:test
npm run app:analyze
```

---

### Task-F06: Auto-pick lock in ColorStopsSheet and the editors (D5, P6)

**Priority**: High
**Estimated Iterations**: 2-3

**Files**:
- `software/lamp-app-flutter/lib/features/control/presentation/widgets/color_stops_sheet.dart`
- `software/lamp-app-flutter/lib/features/control/presentation/widgets/shade_editor_sheet.dart`
- `software/lamp-app-flutter/lib/features/control/presentation/widgets/base_editor_sheet.dart`
- `software/lamp-app-flutter/lib/features/control/domain/color_harmony.dart`
  (exports `kHarmonyMaxCount`)
- tests: `color_stops_sheet_test.dart`, `shade_editor_sheet_test.dart`,
  `base_editor_sheet_test.dart` (same folder under `test/`)

**Steps**:
- **Opt-in API.** `ColorStopsSheet` gains an optional `autoPick` of type
  `AutoPickConfig?`, defined in the same file. Null means the sheet behaves as
  it does after F05. Don't add it to `showColorStopsSheet`.

  ```dart
  /// The surface's Auto-pick is on and uses this scheme. The sheet locks the
  /// derived rows and regenerates them from the primary, the last storage
  /// index; the flags themselves change only on the Color harmony screen.
  class AutoPickConfig {
    const AutoPickConfig({required this.schemeId});
    final int schemeId;
  }
  ```

- **State.** Snapshot the whole config once, as the sheet already snapshots
  `initial`: `late final AutoPickConfig? _autoPick = widget.autoPick;`. The
  lock and the scheme come only from `_autoPick`, never from
  `widget.autoPick`: the Shade/Base parents rebuild and re-pass props on every
  live write and after a reconnect.
- **Primary.** The last storage index (D1): the first row with `topFirst`
  (Shade), the last row without it (Base, P1).
- **Description.** When `autoPick` is non-null, the editors pass a description
  instead of the gradient sentence: Shade "Auto-pick is on. The other colors
  follow the top color; change the scheme in Setup."; Base "Auto-pick is on.
  The other colors follow the bottom color; change the scheme in Setup." The
  sheet itself adds no header row.
- **Regeneration.** One synchronous helper:
  `harmonizePalette(_colors, _schemeId, count: n)`, then `_apply`. Call it only
  from:
  - a primary edit: in both the picker's `onLive` and its Done result, replace
    the primary, then regenerate. Picker Cancel keeps the existing whole-list
    restore (see D3);
  - Add: `n = length + 1`. Hide Add while on and
    `length >= kHarmonyMaxCount` (P7). Export `const int kHarmonyMaxCount = 5;`
    from `color_harmony.dart` and have `harmonyColors`' clamp use it in place
    of its private constant;
  - removing a derived row: `n = length - 1`.
- **While on (D5):**
  - derived rows have `onTap: null`;
  - the primary row has no remove button; put a `SizedBox` the width of the
    remove `IconButton` in its place, so the color bars stay aligned and
    nothing shifts when Auto-pick toggles;
  - derived rows keep their remove button;
  - every row keeps its handle icon; set `enabled: false` on the existing
    `ReorderableDragStartListener` (keep `index: i`) rather than wrapping it
    again.

  Style the disabled state explicitly; `ListTile(enabled: false)` and
  `onPressed: null` don't dim the custom bar or the explicitly colored hex. Dim
  every row's handle icon to 0.4 opacity (all are disabled), and the hex text
  on derived rows only. Keep the color bar at full strength, since it shows
  what the lamp shows. Give derived rows a `Tooltip` and Semantics label:
  "Auto-picked from the top color. Turn off Auto-pick in Setup to edit." with
  `topFirst`, and the same with "bottom color" without it (the primary's row
  position decides the word, so no new API is needed). Keep
  one source of that label so screen readers don't announce it twice. If you
  wrap a row, keep the `ValueKey` on the outermost widget `itemBuilder`
  returns.
- Never regenerate in `initState`, `build`, or on parent rebuilds (D3).
- **Editors.**
  - `ShadeEditorSheet`: `topFirst: true` always (F04). Pass `autoPick` only
    when the shade is eligible (F03) and `state.shade.autoColors` is true:
    `AutoPickConfig(schemeId: state.shade.colorScheme)`. `ColorChannelSpec`
    carries no segment index, and none is needed: eligibility excludes
    multi-segment shades, the only case where a spec above segment 0 exists.
    Don't add a segment field.
  - `BaseEditorSheet` watches only `base`; keep that watch and gate on
    `base.harmonyEligible && base.autoColors`, passing
    `AutoPickConfig(schemeId: base.colorScheme)`.
  - Don't change either editor's `onSave`.
- **Stale comments.** Replace the `_kMaxShadeStops` comment with
  `/// Hard cap on shade gradient stops. Firmware renders at most 8; 6 keeps the sheet scannable.`
  and the `_kMaxBaseStops` comment with the same wording, "row list" in place
  of "sheet".

**Tests** (Shade-style sheet tests use `topFirst: true`; see "Test harness
notes"):
1. A primary edit while on (open row `color-stop-0`, type a hex into the
   picker's hex field, tap Done) regenerates the whole palette; `onLive`
   regenerates too.
2. Without `autoPick`, a primary edit changes only the primary.
3. While on: tapping a derived row opens no picker; the primary row has no
   `Icons.close`; `find.byType(ReorderableDragStartListener)` finds one per
   built row (more than zero), and each has `enabled == false`.
4. Add while on gives count +1, regenerated, and hides at 5. Removing a derived
   row gives count −1, regenerated.
5. Load safety: opening with `autoPick` and colors that don't match the
   generator emits nothing and shows them as saved. Then rebuild with a
   different `initial` and `autoPick: null`, driven from inside the sheet's
   builder (a `ValueListenableBuilder` that supplies `initial`, `title` and
   `autoPick`), so the rebuild reaches the sheet: assert the new title shows,
   no new `onChanged`, the opened colors still show, and the derived rows stay
   locked.
6. The description line reads the Auto-pick sentence while on (the "top color"
   sentence in the Shade editor, the "bottom color" sentence in the Base
   editor) and the gradient sentence while off (editor tests).
7. Shade editor seeded with `shadeAutoColors: true, shadeColorScheme: 4` and a
   colorful two-color `shadeColorsJson`, asserting on
   `ble.writesTo(id, BleUuids.shadeColors)`; Base editor seeded with
   `baseAutoColors: true, baseColorScheme: 4`, asserting on
   `BleUuids.baseColors`. In each, a primary edit produces a colors write
   carrying the regenerated list, and Save produces at least one commit write
   and no settingsBlob write.
8. A shade seed with three segments whose segment 0 has two colors, for
   example
   `'[{"name":"A","px":12,"colors":["#FF0000FF","#0000FFFF"]},{"name":"B","px":12,"colors":["#000000FF"]},{"name":"C","px":14,"colors":["#000000FF"]}]'`,
   and `shadeAutoColors: true` opens with no lock: tapping the derived row
   opens the picker (`find.text('Done')` finds it).
9. Picker Cancel while on: open the primary row, enter a hex so `onLive`
   regenerates the palette, then tap the picker's Cancel
   (`find.text('Cancel').last`). The last `onChanged` equals the list from
   before the picker opened.
10. Without `topFirst` (the Base sheet), the primary is the last row: it has
    no `Icons.close`, the rows above it don't open the picker, their tooltip
    says "bottom color", and a primary edit regenerates the rows above it while
    the primary stays at the last storage index.
11. Existing editor tests stay green. Where keyed finders are needed, add them
    without changing what the tests assert.

**Acceptance Criteria**:
- [ ] Without `autoPick`, the sheet renders and behaves as it did after F05.
- [ ] Generation is synchronous and runs only from the listed handlers.
- [ ] Save writes colors and commit only; the flags never move from the sheet.
- [ ] Tests pass per Working environment. `npm run app:analyze` adds no
      issues.

**Verification**:

```bash
npm run app:test -- test/features/control/presentation/widgets/
npm run app:test
npm run app:analyze
```

---

### Task-F07: Color harmony screen under Setup › LEDs (R2, R3, R5)

**Priority**: High
**Estimated Iterations**: 3-4

**Files**:
- `software/lamp-app-flutter/lib/features/lamp_shell/presentation/color_harmony_screen.dart` (new)
- `software/lamp-app-flutter/lib/features/lamp_shell/presentation/setup_screen.dart`
- `software/lamp-app-flutter/lib/core/routing/routes.dart`
- `software/lamp-app-flutter/lib/core/routing/router.dart`
- `software/lamp-app-flutter/lib/features/control/application/control_notifier.dart`
- tests: `software/lamp-app-flutter/test/features/lamp_shell/color_harmony_screen_test.dart` (new),
  `test/features/lamp_shell/setup_screen_test.dart`,
  `test/features/control/set_color_harmony_test.dart`

**Steps**:
- **Route.** `AppRoutes.colorHarmony(String id) => '/lamp/$id/setup/color-harmony'`
  next to `advancedLeds`. Register a `GoRoute` in `router.dart` beside the
  advanced-leds entry, building `ColorHarmonyScreen(lampId: ...)`. No codegen.
- **Setup row.** Under the `SettingsGroupHeading('LEDs')`, after "LED setup",
  add `SettingsRow(icon: Icons.palette_outlined, title: 'Color harmony', ...)`
  that pushes the route. Show it only when at least one surface is eligible
  (F03). Subtitle: "Auto-pick off" when no eligible surface is on; otherwise
  the on surfaces and the shared scheme name, for example "Shade, Base ·
  Triadic", or "Shade · Triadic, Base · Square" when the on surfaces' schemes
  differ. Decide "differ" with
  `sharedSchemeId(shadeScheme: ..., baseScheme: ..., shadeEligible: shadeOn, baseEligible: baseOn)`,
  so unknown ids compare as Analogous (D8): schemes 0 and 99 read
  "Shade, Base · Analogous".
- **Notifier helpers.** Add to `ControlNotifier`, next to `setColorHarmony`:
  - `Future<void> setAutoPick(HarmonySurface surface, bool on)`: when turning
    on, snapshot that surface's colors, compute
    `autoPickPalette(snapshot, that surface's stored scheme, on: true)`,
    stream it with `setShadeColors` / `setBaseColors`, then
    `setColorHarmony(surfaces: {surface}, autoColors: true)`, then `commit()`.
    When turning off, `setColorHarmony(surfaces: {surface}, autoColors: false)`
    only; colors stay (D4).
  - `Future<void> pickColorScheme(int schemeId)`: first, for every eligible
    surface whose `autoColors` is on, snapshot its colors, regenerate with
    `harmonizePalette(snapshot, schemeId)` and stream it; then one
    `setColorHarmony(surfaces: <every eligible surface>, colorScheme: schemeId)`
    (one settings_blob, P5); then one `commit()` if anything regenerated (R3).

  Both stream colors before the flags and write the flags before the commit
  (P5). If `setColorHarmony` throws, `_mutate` has already reverted the flags;
  stream every snapshot back with `setShadeColors` / `setBaseColors` (the
  stops sheet's Cancel restore), skip `commit()`, and rethrow. The screen
  awaits each helper in a try/catch and, when `context.mounted`, calls
  `AppSnackbar.error(context, "Couldn't update Auto-pick. Try again.")` after
  a chip change or `AppSnackbar.error(context, "Couldn't change the scheme. Try again.")`
  after a scheme pick, in the wisp screens' style.
- **Screen.** `ColorHarmonyScreen` is a `ConsumerStatefulWidget` holding
  `bool _open = false` (R5). Its `Scaffold` copies `AdvancedLedsScreen`:
  `AppBar(leading: const BackButtonLeading(), title: const Text('Color harmony'))`
  and a body of `async.when(loading: () => const SizedBox.expand(), error: (e, _) => FriendlyError.page(...), data: ...)`
  with the same error copy, and a `ListView` for data. It reads
  `controlNotifierProvider(lampId)` and the notifier fresh per tap
  (precedent: `_SetupBody`). Content, top to bottom, loose idiom per
  `FORM_STYLING.md`:
  1. `InfoPanel` with "Auto-pick fills in a surface's other colors from one of
     them: the Shade's top color and the Base's bottom color. Pick the
     surfaces, then the scheme." (P1)
  2. `SectionHeader('Auto-pick colors for')`, then a full-width
     `SegmentedButton<HarmonySurface>` (`Key('auto-pick-surfaces')`) with
     `multiSelectionEnabled: true`, `emptySelectionAllowed: true`,
     `showSelectedIcon: false`, the `AppRadius.card` shape used by
     `_StripTypePicker`, segments "Shade" and "Base" present only when
     eligible, `selected` = the eligible surfaces whose `autoColors` is on.
     `onSelectionChanged` diffs the new set against the old and calls
     `setAutoPick` once per changed surface.
  3. `SectionHeader('Scheme')`, then the summary row (`Key('scheme-summary')`):
     the selected scheme's name, or "Mixed" when `sharedSchemeId` is null; its
     swatch strip (P8), shown only when a scheme is selected (the web UI shows
     none in the Mixed state); and a `TextButton` (`Key('scheme-toggle')`)
     reading "Change" while the list is closed and "Hide" while open. In the
     Mixed state a muted line reads "Shade uses <X>, Base uses <Y>. Picking a
     scheme sets both." Open or closed is `_open`, default closed (R5).
  4. The list, when open: one row per `kHarmonySchemes` entry,
     `Key('harmony-scheme-<key>')`, with the name (bodyMedium), the description
     (bodySmall, `onSurfaceVariant`), the swatch strip, and a trailing
     `Icon(Icons.check, color: colorScheme.primary)` on the selected row
     (precedent: the Web Config picker in `setup_screen.dart`). No row is
     checked in the Mixed state. Tapping a row calls `pickColorScheme` and
     keeps the list open.
  - Swatch strips are a `Row` of `LampColorSwatch(color: c, size: 16, borderRadius: 2)`
    squares (the default radius would round a 16 px swatch into a circle),
    with `// deliberate dimension, not spacing` on the literals.
  - With no eligible surface the body is
    `EmptyStatePane(icon: Icon(Icons.palette_outlined, size: 56, color: colorScheme.onSurfaceVariant), title: 'No Auto-pick on this lamp', subtitle: "This lamp's colors are set by its firmware.")`,
    shaped like the firmware-capability empty state in
    `add_expression_picker_screen.dart`, with
    `// deliberate dimension, not spacing` on the size literal. The Setup row is
    already hidden in that case.
- **Tests** (see Test harness notes for decoding, write failure, navigation
  and viewport):
  - Setup: the row shows with the default seed and is absent with
    `shadeColorsEditable: false, baseColorsEditable: false`; its subtitle reads
    "Auto-pick off", "Shade · Triadic" and "Shade · Triadic, Base · Square" for
    the matching seeds; tapping it reaches the stub route (GoRouter harness).
  - Screen, default seed (both eligible, both off, schemes 0): no segment
    selected; summary "Analogous"; list closed; "Change" opens it, "Hide"
    closes it; the Analogous row is checked.
  - Shade chip on with one colorful shade color (`shadeColorsJson: '["#FF0000C8"]'`):
    the shade colors write equals `harmonizePalette([that color], 0, count: 3)`
    (the original at the last index); one settingsBlob write whose `shade`
    object holds only `autoColors: true`, with `reboot` false; at least one
    commit write; the chip shows selected.
  - Shade chip off: one settingsBlob write with `autoColors: false`, no colors
    write, colors unchanged.
  - Scheme pick with Shade on and Base off: exactly one settingsBlob write,
    whose `shade` and `base` objects both carry `colorScheme`; only the shade
    colors regenerate; one commit; the list stays open, the picked row is
    checked, the summary updates.
  - Mixed seed (`shadeColorScheme: 4`, `baseColorScheme: 5`): summary "Mixed"
    with no swatch strip, the note shows, no row checked; a pick clears the
    Mixed state.
  - Unknown ids: `shadeColorScheme: 0`, `baseColorScheme: 99` shows
    "Analogous" with no note.
  - Ineligible Base (`baseColorsEditable: false`, `baseAutoColors: true`):
    only the Shade segment renders, and the swatch preview uses Shade's
    primary even though Base's chip flag is on.
  - Failure (`setAutoPick` and a scheduled settingsBlob failure, in
    `set_color_harmony_test.dart`): the call throws; state colors and
    `autoColors` equal the seed; after a 60 ms pump the last shadeColors write
    equals the seeded palette; no commit write.
  - Failure on the screen: the chip is unselected after the error and the
    snackbar shows "Couldn't update Auto-pick. Try again."

**Acceptance Criteria**:
- [ ] Chips and scheme write to the lamp immediately and only through
      `setAutoPick` / `pickColorScheme`, one settings_blob per user action.
- [ ] Shade and Base chips are independent; one scheme is shown; the Mixed
      state applies R3 over eligible surfaces (the web UI applies it over
      editable surfaces; they differ only on multi-segment surfaces).
- [ ] The list is collapsed by default and a pick keeps it open.
- [ ] A failed flag write restores the streamed colors and commits nothing.
- [ ] Tests pass per Working environment. `npm run app:analyze` adds no
      issues. No codegen run.

**Verification**:

```bash
npm run app:test -- test/features/lamp_shell/color_harmony_screen_test.dart
npm run app:test -- test/features/lamp_shell/setup_screen_test.dart
npm run app:test
npm run app:analyze
```

---

### Task-F08: Home card note (R6)

**Priority**: High
**Estimated Iterations**: 1-2

**Files**:
- `software/lamp-app-flutter/lib/features/control/presentation/widgets/shade_card.dart`
- `software/lamp-app-flutter/lib/features/control/presentation/widgets/base_card.dart`
- `software/lamp-app-flutter/lib/features/control/presentation/control_screen.dart`
- tests: `shade_card_test.dart`, `base_card_test.dart` (same folder under `test/`)

**Steps**:
- **Slices.** Extend `_ShadeSlice` and `_BaseSlice` with `autoColors`,
  `colorScheme` and eligibility (the F03 `harmonyEligible` getter), so the
  note rebuilds only when those change. Update both slice doc comments to name
  the fields they now carry, and drop `_ShadeSlice`'s "now that shade is a
  gradient too" phrasing.
- **Note.** When the surface is eligible and `autoColors` is on, render a
  second line under the title (bodySmall, `onSurfaceVariant`): "Auto-picked · "
  followed by the scheme name as a `TextButton` (`Key('auto-pick-link')`,
  zero padding, `visualDensity: VisualDensity.compact`) that pushes
  `AppRoutes.colorHarmony(lampId)`. The primary is the last storage index
  (D1). When `isNeutralColor(primary)`, the line reads
  "Auto-picked · pick a more colorful top color" on the shade card and
  "Auto-picked · pick a more colorful bottom color" on the base card (P1), with
  no button. No line while
  `autoColors` is off or the surface is ineligible. The card's own `InkWell`
  tap still opens the editor; the button sits inside the title column so both
  targets stay reachable. The title column is about 200 px wide on a 360 dp
  phone, and "Auto-picked · Split complementary" is wider, so lay the line out
  as a `Wrap(crossAxisAlignment: WrapCrossAlignment.center, ...)` (or put the
  button in a `Flexible`) so a long name wraps instead of overflowing.
- **`ShadeCard`.** `ColorChannelSpec` carries no segment index. Gate the note
  on shade eligibility and `autoColors`: an eligible shade has at most one
  segment, so the only shade card on screen is segment 0. Add no card
  parameter or spec field.
- **Tests** (navigation per Test harness notes):
  - seeded `shadeAutoColors: true`, `shadeColorScheme: 4` and a colorful shade
    (`shadeColorsJson: '["#FF0000C8"]'`; the default `#000000FF` is neutral):
    the shade card shows "Auto-picked · " and a button "Triadic"; tapping the
    button reaches the stub route; tapping the card body still opens the
    editor;
  - seeded with a neutral primary (`shadeColorsJson: '["#FFFFFF00"]'`) and
    auto on: the neutral text shows and there is no button;
  - auto off: no note;
  - three-segment shade with auto on: no note on any shade card;
  - at `tester.view.physicalSize = const Size(360, 800)` and
    `devicePixelRatio = 1` (with `addTearDown(tester.view.reset)`), a shade
    card with auto on, `shadeColorScheme: 3` and a colorful primary renders
    with no RenderFlex overflow;
  - the base card mirrors the first three cases, with the "bottom color"
    neutral line.

**Acceptance Criteria**:
- [ ] On single-segment surfaces the note appears exactly when the web UI
      shows its note, with the web UI's words, except that the base card's
      neutral line says "bottom" where the web UI says "top" (P1). A
      multi-segment surface shows none (P2).
- [ ] Tests pass per Working environment. `npm run app:analyze` adds no
      issues.

**Verification**:

```bash
npm run app:test -- test/features/control/presentation/widgets/
npm run app:test
npm run app:analyze
```

---

### Task-F09: Older-firmware and reconnect behavior

**Priority**: Medium
**Estimated Iterations**: 1

**Files**:
- `software/lamp-app-flutter/test/features/control/reconnect_mirrors_firmware_test.dart`
- `software/lamp-app-flutter/test/features/lamp_shell/color_harmony_screen_test.dart`

**Steps**:
- No production change is expected. This task pins the behavior the design
  relies on and fixes only what the tests reveal.
- **Older firmware.** A seed whose base and shade JSON omit both keys loads
  `autoColors == false` and `colorScheme == 0`; the Setup row shows "Auto-pick
  off"; turning a chip on still regenerates and writes, and the optimistic
  state shows the chip on until the next reconnect.
- **Reconnect mirrors the lamp.** After a settings_blob write, a reconnect that
  re-reads sections with `baseAutoColors: true` keeps true, and one that
  re-reads without the keys drops back to false (the lamp didn't store them).
  Follow `reconnect_mirrors_firmware_test.dart`'s pattern.
- **Web parity.** A seed written the way the web UI saves: `shadeAutoColors: true`,
  `shadeColorScheme: 2` and `baseColorScheme: 2` (a web pick writes the id to
  every editable surface), with a two-color colorful `shadeColorsJson`. The
  Color harmony screen opens with the Shade chip on and the summary reading
  "Complementary"; after tapping `scheme-toggle`, the
  `harmony-scheme-complementary` row has the check. The shade editor opens
  with the derived row locked and no colors write on open (D3).

**Acceptance Criteria**:
- [ ] The three behaviors are covered by tests and pass.
- [ ] Any production change made here is listed in `PROGRESS.md` Notes with
      the test that forced it.

**Verification**:

```bash
npm run app:test -- test/features/control/reconnect_mirrors_firmware_test.dart
npm run app:test -- test/features/lamp_shell/color_harmony_screen_test.dart
npm run app:test
npm run app:analyze
```

---

### Task-F10: Integration, docs, convention sweep

**Priority**: Medium
**Estimated Iterations**: 1

**Steps**:
- Run `npm run app:test` and `npm run app:analyze`. Compare with the F01
  baseline.
- **`docs/dev/color-harmony.md`:**
  - add a "Flutter client" section after "Web UI": where the generator, the
    Color harmony screen, the editor lock and the Home note live; the shared
    golden fixture read in place; Flutter's orientation (the Shade lists
    top-first; the Base lists index 0 first, the top of the base on
    reversed-Base lamps; the primary is the last index on both, so Base copy
    says "bottom color"); eligibility
    (editable and single-segment); immediate persistence through
    settings_blob with `reboot: false` followed by a commit; the regeneration
    triggers as a table like the Web UI section's;
  - in the golden-regen instructions, state that a table change also updates
    the Dart table in the same commit and must pass `npm run app:test` (no CI
    runs `ui:test` or `app:test`);
  - update the intro (lines 3-11): it says the web UI implements the spec and
    that "the last two sections" cover storage and the web UI; describe both
    implementations and the three closing sections. Add
    `lib/features/control/domain/color_harmony.dart` and its test to the Files
    table;
  - apply any TypeScript-vs-doc corrections recorded in F02 (none were found).
- **`docs/dev/README.md`:** rewrite the `color-harmony.md` index entry's
  opening as "color-harmony auto-pick in the web UI and the Flutter app: ..."
  and mention the Flutter client.
- **`software/lamp-app-flutter/test/manual/v1-checklist.md`:** line 42 names a
  hue ring the picker doesn't have; rewrite it as "Open the shade picker, drag
  any slider → the lamp shade changes live while dragging (no need to tap
  Save)." Lines 67 and 72 (4 sliders; R/G/B + Warm White) stay. Delete line 24
  (the editor has no active-stop ring). Rewrite lines 17 and 21 for the stops
  sheet ("Tapping the Shade card opens a list of colors, top row first"; "Add
  inserts a copy of the bottom color at the bottom"), and add the Base
  equivalent ("Tapping the Base card opens a list with Stop 1 first, the top of
  the base on a standard lamp; Add appends a copy of the last color"). Add
  checks for the Color harmony screen, the Home note, the editor lock (the
  Base primary is its last row) and the swipe revert.
- **`docs/dev/color-harmony.md` Web UI section:** record as a known issue that
  on reversed-Base lamps (standard, staff, snafu) the web UI's "Top" label over
  the Base's last index marks the bottom of the base. No `software/lamp-ui`
  change.
- **`software/lamp-app-flutter/docs/FORM_STYLING.md`:** change it only if a new
  idiom shipped. The multi-select `SegmentedButton` and the summary row with a
  Change / Hide button reuse existing primitives, so normally no change.
- **Comment sweep** over the phase-2 diff
  (`git diff <phase-2 base commit>..HEAD -- software/lamp-app-flutter docs`).
  Grep added lines for `Note:`, `Why:`, ` we `, `previously`, `for now`,
  `placeholder`, `Phase `, em-dashes, untagged `TODO`, `/Users/`, `Mirror`,
  `Ported`, `port of`, `.ts`, `lamp-ui/` and `lamp-os/src`. Fix every hit in
  code comments.
- Confirm `dependencies` and `dev_dependencies` in `pubspec.yaml` are
  unchanged and no generated file changed.

**Acceptance Criteria**:
- [ ] Both commands pass against the baseline.
- [ ] Docs updated. Sweep clean. `pubspec.yaml` and generated files unchanged.
- [ ] `PROGRESS.md` shows every F-task complete.

---

## Manual verification on a lamp (for the user, after Task-F10)

Executors can't do this step. The app updates its state optimistically and
re-reads the lamp only on connect, so persistence checks require leaving the
lamp and reconnecting. Use a lamp running firmware built from `dev` after the
phase-1 merge (its sections carry `autoColors` and `colorScheme`).

1. **Install** the app on an Android phone (`npm run app:install` or
   `npm run app:run`; for iOS, run `flutter run` from
   `software/lamp-app-flutter`) and connect to the lamp. Run the parity steps
   on a single-segment lamp (P2).
2. **Orientation (P1).** The top Shade row is the color at the top of the
   shade, and the top Base row (Stop 1; the Base list isn't flipped) is the
   color at the top of the base. The cards and the critter preview agree. On a
   loaf, check the Base too; its mounting isn't recorded.
3. **Setup › Color harmony.**
   - With Analogous selected (pick it first) and one Shade color, turn the
     Shade chip on: the lamp's shade fills in
     three colors at once, Home shows "Auto-picked · Analogous" under Shade,
     and the Setup row's subtitle reads "Shade · Analogous".
   - Turn it off: the colors stay, the note goes away.
   - Shade and Base chips are independent: try each alone and both.
4. **Scheme list.** Collapsed on open; Change opens it, Hide closes it. A pick
   sets both surfaces' scheme; a surface with its chip on updates on the lamp
   at once; the list stays open. The note's scheme link on Home opens this
   screen.
5. **Editor lock.** With the Shade chip on, open the Shade editor: derived rows
   are dimmed and don't open the picker; the top row has no remove button.
   Drag the top row's sliders: every shade color follows live. Add and remove
   regenerate; Add stops at 5. Cancel reverts the lamp. With the Base chip on,
   the Base editor's bottom row is the primary and the rows above it are
   locked.
6. **Persistence and parity.**
   - Back out of the lamp (or power-cycle it), reconnect: chips, scheme and
     colors persist.
   - Open the web UI: Setup shows the same chips and scheme; Home shows the
     same note.
   - Change the scheme in the web UI and save, reconnect the app: it shows the
     web's scheme and the colors exactly as saved, with no regeneration on
     open.
7. **Swipe.** Edit Shade colors, then swipe the sheet down: the lamp reverts.
   Repeat on an expression's Colors sheet: the expression's draft colors revert.
8. **Older firmware.** On a lamp without phase-1 firmware, the chips still
   generate colors. After backing out and reconnecting, both chips read off.
9. **Snafu.** The Setup row shows (Base is eligible); the screen offers only
   the Base chip; shade segment sheets have no lock and no note.

## Known pre-existing issues (out of scope; don't fix here)

- **`bpp` is always 4 after a section read.** Firmware never emits `bpp`, so
  the Warm White slider also shows on GRB/BGR strips, which discard W. D6
  copies W anyway, which is harmless there.
- **Shade segments above 0 open no edit session,** so wisp paint can fight
  their live preview.
- **On-screen gradients understate LED mid-strip darkness.** Firmware blends
  stored bytes and only then applies gamma, so contrast schemes show dim,
  greyish bands between distant hues on the lamp. Phase 1's manual step 8
  covers tuning.
- **The web UI writes one palette to every segment on save.**
- **The web UI's Base "Top" label is upside down on reversed-Base lamps.** It
  lists the Base's last index first as "Top", which is the bottom of the base
  on the standard, staff and snafu lamps (P1). A phase-1 follow-up.
- **On a multi-segment surface with a web UI (the lioness Base),** the web UI
  offers the chip, the note and the Mixed comparison; the app doesn't (P2).

## Technical constraints

- Dart >= 3.12 (Flutter 3.44+), Riverpod 3 (dev pre-release),
  `flutter_test` + `mocktail`.
- Analyzer: `flutter_lints` plus every rule in
  `software/lamp-app-flutter/analysis_options.yaml` (`strict-casts`,
  `strict-raw-types`, `prefer_const_*`, `require_trailing_commas`,
  `unawaited_futures`, `avoid_print`, `use_super_parameters`,
  `cancel_subscriptions`, `close_sinks`). Info-level issues fail
  `flutter analyze`.
- Use the current `Color` API: `.r/.g/.b` doubles with `(c.r * 255).round()`,
  `toARGB32()`, `withValues(alpha:)`. Never `.red`, `.value` or `withOpacity`.
- No `Radio`; its members are deprecated on this SDK.
- Spacing uses `AppSpace` / `AppRadius` tokens. Colors come from `colorScheme`
  / `brandExtras`. Component literals carry
  `// deliberate dimension, not spacing`. The Color harmony screen uses the
  loose idiom (`SectionHeader` + tokenized spacing), never `FormSection` /
  `LampCard`, since each block is a single control.

## Conventions summary (from CLAUDE.md, enforced in review)

- **Comments.**
  - Default to none. Write one only for a non-obvious WHY, in about 2 lines,
    terse.
  - No `we`, no attention labels (`Note:`, `Why:`), no em-dashes.
  - No history narration, no hedges (`for now`, `just in case`), no
    forward-leaning prose (`placeholder`, `reserved for`), no provenance
    (`Mirrors X`, `Ported from Y`).
  - State a contract once, on the declaration. Anything longer belongs in
    `docs/dev/color-harmony.md` behind a one-line pointer.
- **References** point only to in-repo `docs/dev/…` files that exist. Never a
  source path.
- **No speculative API.** `topFirst` exists on `ColorStopsSheet` only because
  the Shade uses it, and `autoPick` because Shade and Base use it. Don't thread
  them through `showColorStopsSheet`. `AutoPickConfig` carries only the scheme.
- **Reuse before you build:** `LampColor`, `LampColorSwatch`, `showAppSheet`,
  `confirmDiscard`, `_mutate`, `writeSettingsBlob`, `SettingsRow`,
  `SectionHeader`, `InfoPanel`, `EmptyStatePane`, `SegmentedButton`.

## Out of scope

- A color wheel or Brightness slider in the picker (R1 removed them from the
  web UI; the Flutter picker never had them).
- Auto-pick for expression colors and the wisp manual palette.
- Per-segment auto-pick on multi-segment shades.
- Any change to firmware, `software/lamp-ui`, protocol constants or
  `kGattLayout`.
- Fixing the pre-existing issues listed above.
- A `VERSION` bump, a `CHANGELOG` entry, or any release.
