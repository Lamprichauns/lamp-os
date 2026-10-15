# Feature: Color Harmony Auto-Pick, phase 2 (Flutter app)

> **Status: not active.** Phase 1 is `prd/PRD-color-harmony.md` (web UI +
> firmware storage). Activate this PRD only when all of these hold:
>
> 1. Phase 1 Task-007 has a Reviewer PASS.
> 2. The user has run the P1 bench check (below) and confirmed P1.
> 3. A Flutter SDK whose bundled Dart is >= 3.12.0 is on PATH (Flutter 3.44 or
>    newer). `pubspec.yaml` requires `sdk: ^3.12.0`; Flutter 3.38 ships Dart
>    3.10 and fails package resolution.
>
> **To activate,** edit `PROGRESS.md`:
>
> 1. Change the `Active PRD:` line to `prd/PRD-color-harmony-flutter.md`.
> 2. Add a `## Phase 2` heading under `## Completed`.
> 3. Under `## Notes`, add one line starting `- P1 confirmed by the user on
>    <YYYY-MM-DD>:` followed by the Shade and Base results. Write it only from
>    the user's own bench-check result; never infer it.
>
> Never edit `prd/PRD-color-harmony.md`.

## Overview

Phase 1 adds a color wheel, an "Auto-pick colors" toggle, a scheme picker and
two firmware keys to the lamp's web UI. Phase 2 brings the same feature to the
Flutter app's Shade and Base color editors. It uses the same generator, golden
fixture and firmware keys, so both clients agree:

1. **Color wheel + Brightness slider** in the shared `ColorPickerSheet`. Every
   picker gets it: shade, base, expression colors, wisp manual palette, wisp
   Off color.
2. **"Auto-pick colors" switch** in the Shade and Base color sheets. When on,
   the primary (the top color) drives colors 2–N through the shared harmony
   spec. Derived rows stay visible but can't be edited. Turning it off keeps the
   current colors.
3. **A scheme picker sub-sheet** per surface.
4. **Read and write the firmware keys** `autoColors` and `colorScheme`, so the
   toggle and scheme follow the lamp between the app and the web UI.

Two supporting changes ride along. The Flutter Shade/Base presentation flips to
top-first (P1) so the primary is the first row. A pre-existing swipe-to-dismiss
bug is fixed (P11), because auto-pick would otherwise leave whole generated
palettes uncommitted in lamp RAM.

## Success Criteria

- [ ] Tasks F01–F10 complete, each with a Reviewer PASS.
- [ ] `npm run app:test` passes.
- [ ] `npm run app:analyze` reports no issues beyond the F01 baseline.
- [ ] The Dart generator passes phase 1's golden fixture (96 cases) and every
      hand-checked vector in `docs/dev/color-harmony.md`, and mirrors the
      TypeScript generator operation for operation.
- [ ] No pub dependency added. No change to firmware, `software/lamp-ui`,
      protocol constants or `kGattLayout`.
- [ ] Manual verification on a lamp (end of this PRD) signed off by the user.

## Working environment (read before every task)

- **Repo and branch.** Main checkout
  `/Users/mikebartlett/Developer/GitHub/lamp-os`, branch
  `feature/color-wheel-addition`. It is NOT a worktree. Before any build, test,
  or commit, run `git rev-parse --show-toplevel` and
  `git rev-parse --abbrev-ref HEAD` and confirm both. If either is wrong, record
  a blocker and stop.
- **Flutter SDK.** The bundled Dart must be >= 3.12.0 (`flutter --version`
  prints both versions). If `flutter` is missing or too old, record a blocker
  and stop. Never install toolchains yourself.
- **Commands.** Use only the root npm tasks: `npm run app:test` and
  `npm run app:analyze`. Both resolve packages, so don't run raw `flutter pub`
  or `dart` commands. Run a single file with
  `npm run app:test -- test/features/control/domain/color_harmony_test.dart`
  (paths are relative to `software/lamp-app-flutter`). No CI job builds or
  tests the app, so these local runs are the only gate.
- **`pubspec.lock`.** If an SDK newer than the one that wrote the lock makes
  these commands rewrite `software/lamp-app-flutter/pubspec.lock`, don't stage
  it. Note it once in `PROGRESS.md` Notes and leave it out of every commit.
- **No codegen.** Nothing here touches `@freezed`, `@Riverpod` or
  `@JsonSerializable` sources. Don't run `npm run app:codegen` or
  `npm run app:clean`; they regenerate 33 tracked files. If a task seems to need
  codegen, record a blocker and stop.
- **Staging.** Stage files by path. Never `git add -A` / `git add .`. Never
  stage `.codegraph/` or the untracked root `package-lock.json`.
- **Commits.** One commit per task, conventional style, with the task ID:
  `feat(app): Task-F02 port the color harmony generator`. Include
  `PROGRESS.md` in the commit.
- **Read `CLAUDE.md` first.** Its comment and code conventions apply to every
  line. A summary is at the end of this PRD.
- **Don't improvise design changes.** If the code contradicts a task in a way
  that forces a design decision, record it under Blockers in `PROGRESS.md` and
  stop.

## Inherited decisions (from phase 1; binding)

Phase 1 settled these with the user. Phase 2 adapts them to Flutter and does
not change them.

- **D1. The primary is the last stored index** (the top of the lamp).
  Generation reads it from the last index and keeps it there.
- **D2. Generation is client-side only.** Firmware stores `autoColors` and
  `colorScheme`. Generated colors are ordinary colors.
- **D3. Regenerate only from user-event handlers:** primary edits (every live
  tick and Done), toggle on, scheme pick, and add or remove while on. Never in
  `initState`, `build`, `didUpdateWidget`, or on provider rebuilds. Opening a
  sheet shows the saved colors exactly, even when they don't match the
  generator. Phase 1's "Cancel/close revert" trigger maps to the picker's
  existing whole-list restore: Cancel puts back the palette from before the
  picker opened. It matches regenerating from the reverted primary, except when
  the palette was loaded out of sync with the generator; then the restore keeps
  the saved colors, as this decision's load rule requires.
- **D4.** Turning auto on with one color raises the count to the scheme's
  natural count, then overwrites colors 2–N. Turning it off keeps the current
  colors.
- **D5. While auto is on:** derived rows stay visible but don't open the
  picker; the primary row has no remove button; remove on a derived row lowers
  the count by one and regenerates; Add raises the count by one (max 5) and
  regenerates. Phase 1's Clone has no Flutter equivalent.
- **D6.** Derived colors copy the primary's warm-white byte. The wheel and all
  hue math use RGB only.
- **D7.** One scheme per surface (Shade, Base).
- **D8.** An unknown scheme id is treated as Analogous for generation and
  display. The stored id is not rewritten unless the user picks a scheme.
- **D10.** One shared wheel, in the shared picker.
- **D11 (Flutter form).** No new pub dependencies.
- **D12.** No `VERSION` bump and no `docs/CHANGELOG.md` entry.
- **D13.** No protocol or GATT change. The keys ride the existing settings_blob
  characteristic.

## Phase-2 decisions

Defaults chosen during planning. **P1 must be confirmed by the user before
activation.** The rest stand unless the user changes them.

- **P1. Top-first orientation (CONFIRM).** Today the Flutter app lists index 0
  first ("Stop 1") and draws index 0 at the top of the Shade/Base cards and the
  critter preview. The web UI does the opposite: it lists the last index first,
  labeled "Top". Under D1 the primary is the last index, so for the primary to
  be the first row, the Flutter Shade/Base sheets and previews flip to
  top-first.

  **Bench check before activating.** On a standard lamp, in the current app:
  1. Set Shade to two stops: Stop 1 red, Stop 2 blue. Note which end of the
     shade is red.
  2. Set Base the same way. Note which end of the base is red. Check both:
     firmware declares each strip's `reversed` flag per lamp variant, and the
     standard Base strip is reversed while the Shade is not, so the Shade result
     says nothing about the Base.
  3. Red at the bottom of both confirms the web UI's orientation. Keep P1.
  4. Red at the top of the shade, or Shade and Base disagreeing, means D1 doesn't
     hold for that surface. Stop and revisit D1 with phase 1 before running
     this PRD.

  If other lamp variants are at hand, repeat the Base check on each.
- **P2. Single-segment surfaces only.** The keys are per surface, but a
  multi-segment shade (snafu has three shade segments, each with its own sheet)
  has one palette per segment. Auto-pick is offered only when the surface has
  at most one segment. The P1 flip still applies to every Shade/Base sheet.
- **P3. Placement.** The auto-pick row is the stops list's `header`, directly
  above the primary row, and it scrolls with the list. Placing it between rows
  would break `ReorderableListView` index mapping.
- **P4. Control and copy.** A plain Material `Switch`, the app's only on/off
  idiom (`lib/` has no Radio). Label "Auto-pick colors", matching the web UI.
- **P5. Scheme picker.** A sub-sheet opened from the auto-pick row's
  "‹Scheme› · Change" link, which stays visible in every state. It edits the
  current surface only.
- **P6. Persistence timing.** Flags are written only on Save, via settings_blob
  with `reboot: false`, before the editor's existing save-and-commit. Cancel,
  discard and swipe-dismiss write no flags.
- **P7. Count cap.** Auto-pick follows the shared clamp of 5. Turning auto on
  with 6 colors leaves 5. While on, Add hides at 5. With auto off, the manual
  cap stays 6.
- **P8. Wheel.** A polar disc (hue by angle, saturation by radius), red at the
  top, hue increasing clockwise: the same geometry as the web wheel. A
  separate "Brightness" slider (0–100%) sets HSV value. Flutter addition:
  touching the wheel while Brightness is 0 first sets Brightness to 100%, so the
  wheel never looks dead on a black start color.
- **P9. Picker layout.** The picker body scrolls. Title and Cancel/Done stay
  pinned. Order: preview swatch + hex row, wheel, Brightness, Red, Green, Blue,
  Warm White (bpp 4 only).
- **P10. Custom wheel, no package.** `flex_color_picker` 4.x needs Flutter
  >= 3.47 plus `material_ui`, and its wheel is a ring + square with no marker
  API. `flutter_colorpicker` has had no release since 2024-05 and uses
  deprecated `Color` APIs. None of the reviewed packages ship Semantics.
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
then `docs/dev/color-harmony.md` (created by phase 1 Task-002), then this
summary. If the TypeScript and the doc disagree, mirror the TypeScript, record
the difference in `PROGRESS.md` Notes, and correct the doc in F10.
`software/lamp-ui` stays untouched.

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
  `harmonyColors(...)` reversed.
- **Golden fixture:** `software/lamp-ui/src/utils/colorHarmony.golden.json`.
  From the Flutter package root (the working directory of `flutter test`) it is
  `../lamp-ui/src/utils/colorHarmony.golden.json`. Read it in place. Don't copy
  it into the app.
- **Firmware keys,** inside both the `base` and `shade` objects:
  - `autoColors`: bool, default false;
  - `colorScheme`: integer, default 0; out-of-range values load as 0.

  Section reads emit both. Settings_blob applies each key only when present, so
  a write carrying only these keys leaves colors, px and byteOrder alone.
  Firmware older than phase 1 drops the keys silently.

**Porting rules for bit-exact parity.** They apply to the generator (F02)
only; the wheel geometry (F03) needn't be bit-exact and may use plain Dart
math. The golden's four primaries don't exercise every arithmetic choice, so a
passing golden is necessary but not sufficient.

- Port operation for operation from the TypeScript: `rgbToHsv` / `hsvToRgb` in
  `software/lamp-ui/src/utils/colorUtils.ts`; `harmonyColors`,
  `harmonizePalette` and the table in
  `software/lamp-ui/src/utils/colorHarmony.ts`. Read them before writing Dart.
- Mirror every floating-point expression with the same operands in the same
  order: hue normalization (helper, `if (h < 0)` branch, or none), channel
  assembly (`(ch + m) * 255` and `ch * 255 + m * 255` give different bytes),
  branch order, sector selection, and round-then-clamp order. Write each table
  factor as the same expression the TypeScript uses (`2 / 3` vs `1 - k / n`).
- Where the TypeScript uses `((x % m) + m) % m`, write
  `((x.remainder(m)) + m).remainder(m)`. Where it uses bare `%`, write
  `x.remainder(m)`. Never use Dart's `%` on doubles; it is a different
  operation.
- Byte rounding: `(value).round()` matches `Math.round` for the non-negative
  values involved.
- Compare outputs with `LampColor.toHex()` (uppercase `#RRGGBBWW`).

## Test harness notes (F06–F09)

- `topFirst` and `autoPick` aren't on `showColorStopsSheet`, so tests host the
  sheet with
  `showAppSheet(ctx, builder: (_) => FractionallySizedBox(heightFactor: 0.6, child: ColorStopsSheet(...)))`,
  copying `showColorStopsSheet`'s body. A real modal route is needed for Save,
  Cancel and pop behavior.
- The list viewport in a 0.6-height sheet on the 800×600 test surface fits the
  header plus about two rows. Before tapping or asserting on a row, call
  `await tester.ensureVisible(finder); await tester.pumpAndSettle();`.
  Alternatively set `tester.view.physicalSize = const Size(800, 1600)` and
  `tester.view.devicePixelRatio = 1`, with `addTearDown(tester.view.reset)`.
- Simulate system back with `await tester.binding.handlePopRoute();`, as the
  existing editor tests do.
- With a picker open over a stops sheet, both routes stay onstage, so
  `find.text('Cancel')` can match twice. Use keys or `.last`.
- Live writes leave 60 ms and 500 ms timers. Pump at least 60 ms before
  asserting a BLE write, and drain about 600 ms before a test ends.

## Tasks

Order: F01 → F02 → F03 → F04 → F05 → F06 → F07 → F08 → F09 → F10.
F03 needs only F02's HSV helpers. F05 is independent of F03 and F04.

---

### Task-F01: Toolchain and prerequisite gate, baseline

**Priority**: High
**Estimated Iterations**: 1

**Steps**:
- Confirm repo and branch (see Working environment).
- `flutter --version` reports Dart >= 3.12.0. If not, record the blocker
  "install a Flutter SDK with Dart >= 3.12 (Flutter 3.44+), e.g.
  `brew install --cask flutter`" and stop.
- Confirm phase 1's outputs exist on this branch:
  - `docs/dev/color-harmony.md`;
  - `software/lamp-ui/src/utils/colorHarmony.golden.json` with 96 cases;
  - `software/lamp-ui/src/utils/colorHarmony.ts` and the HSV helpers in
    `colorUtils.ts`;
  - `software/lamp-os/src/config/config.cpp` emits `autoColors` and
    `colorScheme` in both `Config::asBaseJson()` and `Config::asShadeJson()`.

  If any is missing, record a blocker and stop.
- `PROGRESS.md` Notes must contain a line starting `P1 confirmed by the user`
  that covers both Shade and Base. If none does, record the blocker "P1 bench
  check not recorded (see P1 and the activation block)" and stop.
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
  - `bool isNeutralColor(LampColor c)`: true when `rgbToHsv` gives `s == 0` or
    `v == 0`.
- Store the derived-color table as one const structure in the same shape as the
  TypeScript's.
- **Comments** cite only `docs/dev/color-harmony.md`: one pointer line, plus one
  line on the HSV helpers saying they follow the spec's math exactly so outputs
  match the web client (the reason `HSVColor` isn't used). Never name the
  TypeScript files or other web-client paths in comments. No walkthroughs.
- **Tests:**
  - every hand-checked vector in the doc;
  - the golden fixture: read `../lamp-ui/src/utils/colorHarmony.golden.json`
    with `dart:io`; fail with a message naming the path if it's missing; assert
    every case, mapping each case's scheme field (id or key) through
    `kHarmonySchemes`;
  - count clamping: 0 gives 1 color, 9 gives 5;
  - unknown id 42 behaves as Analogous;
  - `harmonizePalette` keeps the primary at the last index and equals
    `harmonyColors` reversed;
  - W is copied: `#FF0000C8` complementary 2 gives `#FF0000C8 #00FFFFC8`;
  - `isNeutralColor` is true for `#FFFFFF00`, `#80808000`, `#000000FF`, and
    false for `#FF000000`.

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

### Task-F03: Color wheel widget

**Priority**: High
**Estimated Iterations**: 2-3

**Files**:
- `software/lamp-app-flutter/lib/features/control/presentation/widgets/color_wheel.dart` (new)
- `software/lamp-app-flutter/test/features/control/presentation/widgets/color_wheel_test.dart` (new)

**Steps**:
- **Geometry,** as top-level functions in `color_wheel.dart`, with the same
  formulas as phase 1 Task-004. `dx`, `dy` are relative to the center, y down.
  - `({double hue, double saturation}) pointToHueSat(double dx, double dy, double radius)`:
    hue = `(atan2(dx, -dy)` in degrees `+ 360) % 360`;
    saturation = `min(1, distance / radius)`.
  - `Offset hueSatToPoint(double hue, double saturation, double radius)`.
- **Widget API.**
  `ColorWheel({Key? key, required double hue, required double saturation, required void Function(double hue, double saturation) onChanged})`.
  Stateless. The parent owns H and S. No `enabled` flag: no caller needs a
  disabled wheel.
- **Size.** `LayoutBuilder`; diameter = `min(maxWidth, _kWheelMaxDiameter)`
  with `static const double _kWheelMaxDiameter = 240; // deliberate dimension, not spacing`.
  Return `Center(child: SizedBox.square(dimension: diameter, child: ...))`, so
  a stretching parent can't widen the box, and compute center and radius from
  that square. Every paint-geometry literal (thumb radius, ring stroke) carries
  the same comment.
- **Painter.** A public `ColorWheelPainter` with public fields (`hue`,
  `saturation`, `diameter`, `thumbRingColor`, `borderColor`) and a
  field-comparing `shouldRepaint` (precedent: `motion_picker.dart`).
  - Hue: a `SweepGradient` over red, yellow, green, cyan, blue, magenta, red,
    rotated with `GradientRotation(-pi / 2)` so red sits at the top and hue
    increases clockwise.
  - Saturation: a `RadialGradient` from opaque white at the center to
    `Color(0x00FFFFFF)` at the edge. Not `Colors.transparent`, which is
    transparent black and draws a grey ring.
  - Thumb: a ring at `hueSatToPoint`, filled with the full-brightness color at
    (hue, saturation).
  - Chrome colors come from `Theme.of(context).colorScheme`. The hue gradient
    is data.
  - `hitTest` uses `diameter` (a painter receives no size there) and returns
    true only inside the disc, so drags that start in the corners still move
    the sheet.
  - Wrap the painter in a `RepaintBoundary`.
- **Gestures.** The wheel lives in a drag-to-dismiss modal sheet, inside a
  scroll view (F04). Use `RawGestureDetector` with a `PanGestureRecognizer`
  subclass that calls `resolve(GestureDisposition.accepted)` right after
  `super.addAllowedPointer(event)`. The wheel then wins the gesture arena on
  pointer-down, and the sheet neither moves nor scrolls. On start and update,
  map `localPosition` (not global) through `pointToHueSat` and call
  `onChanged`. A tap also sets the value.
- **Semantics.** Label "Color wheel". Value "Hue N°, saturation N%".
  `onIncrease` / `onDecrease` move hue by ±5°, wrapping. Saturation stays
  reachable through the RGB sliders and hex field.

**Tests**:
- Geometry: top gives 0°, right 90°, bottom 180°, left 270°; the center gives
  saturation 0; beyond the edge gives 1; `hueSatToPoint` round-trips
  `pointToHueSat` within 1e-9.
- A tap at `tester.getCenter(find.byKey(const Key('color-wheel'))) + Offset(r - 2, 0)`,
  with `r` half the side of the wheel's `CustomPaint` (`tester.getSize`), calls
  `onChanged` with hue ≈ 90.
- Inside `showAppSheet`, a vertical drag that starts inside the wheel calls
  `onChanged` more than once, and the sheet stays open.
- The Semantics label is present.

**Acceptance Criteria**:
- [ ] Tests pass. `npm run app:analyze` adds no issues.
- [ ] No pub dependency added.

**Verification**:

```bash
npm run app:test -- test/features/control/presentation/widgets/color_wheel_test.dart
npm run app:analyze
```

---

### Task-F04: Wheel and Brightness in ColorPickerSheet

**Priority**: High
**Estimated Iterations**: 2-3

**Files**:
- `software/lamp-app-flutter/lib/features/control/presentation/widgets/color_picker_sheet.dart`
- `software/lamp-app-flutter/test/features/control/presentation/widgets/color_picker_sheet_test.dart`

**Steps**:
- **Layout (P9).** Title pinned at the top, Cancel/Done pinned at the bottom,
  everything between in a `SingleChildScrollView`: the preview swatch + hex
  row, `ColorWheel`, Brightness, Red, Green, Blue, Warm White (bpp 4 only).
  Keep the keyboard-inset padding. Gaps use `AppSpace` tokens.
- **Keys.** Keep `preview-swatch` and `ww-slider`. Add `color-wheel` (on the
  `ColorWheel`), `brightness-slider`, `red-slider`, `green-slider`,
  `blue-slider`, and `hex-field`.
- **Brightness reuses `_ChannelSlider`.** Give `_ChannelSlider` a `max`
  parameter (default 255) and a value-label builder (default `'$value'`).
  Brightness is
  `_ChannelSlider(key: const Key('brightness-slider'), label: 'Brightness', max: 100, valueLabel: (v) => '$v%', ...)`.
  Don't add a second slider-row class.
- **State.** Hold `_hue`, `_sat`, `_brightness` next to `_r/_g/_b/_w`, all
  initialized from `initial` in field initializers or `initState`.
- **Sync rules** (same as phase 1 Task-004):
  - A wheel or Brightness change sets RGB to
    `hsvToRgb(_hue, _sat, _brightness / 100)` from F02, leaves W unchanged,
    then runs the existing path (`_syncHexField`, `_emitLive`).
  - An RGB slider or hex change recomputes H, S and brightness with
    `rgbToHsv`. When `s == 0` or `v == 0`, keep the previous hue. When
    `v == 0`, also keep the previous saturation, so the thumb never jumps to
    red.
  - Re-derive H/S/V from RGB only in those handlers, never on rebuild.
  - P8: if the wheel is touched while `_brightness == 0`, set `_brightness` to
    100 before applying the wheel value.
- **Unchanged:** bpp 3 forces W to 0; Done pops the color; Cancel pops null;
  `onLive` fires on every change.
- **Tests.** Update `color_picker_sheet_test.dart` line 97: drag
  `find.byKey(const Key('red-slider'))` after `tester.ensureVisible`, instead
  of `find.byType(Slider).first`. Keep the label, swatch and `ww-slider`
  assertions. Add:
  - opening the picker on the default 800×600 surface throws no overflow
    (`tester.takeException()` is null);
  - a wheel tap changes RGB, keeps W, and fires `onLive`;
  - Brightness from 100 to 50 turns `#FF000000` into `#80000000`, W kept;
  - from `#00000000`, a wheel tap sets Brightness to 100 and yields a visible
    color;
  - typing grey `#808080` into `hex-field` keeps the previous hue (the
    `ColorWheelPainter.hue` is unchanged).

**Acceptance Criteria**:
- [ ] All picker tests pass, old and new. `npm run app:analyze` adds no issues.
- [ ] Every caller (stops sheet, wisp Off color) shows the wheel with no
      call-site change.

**Verification**:

```bash
npm run app:test -- test/features/control/presentation/widgets/color_picker_sheet_test.dart
npm run app:test
npm run app:analyze
```

---

### Task-F05: Section fields, `setColorHarmony`, seed support

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
- **Notifier.** Add `enum HarmonySurface { shade, base }` and
  `Future<void> setColorHarmony({required HarmonySurface surface, required bool autoColors, int? colorScheme})`.
  Follow `setLampSocialMode`: `_mutate` updates the section optimistically,
  then calls
  `writeSettingsBlob({<'shade' or 'base'>: {'autoColors': autoColors, if (colorScheme != null) 'colorScheme': colorScheme}}, reboot: false)`.
  Callers pass `colorScheme` only when the user picked one (D8).
- **Seed support.** Give `seedControlBle` optional `bool? shadeAutoColors`,
  `int? shadeColorScheme`, `bool? baseAutoColors`, `int? baseColorScheme`,
  emitted into the section JSON only when non-null, so existing seeds are
  byte-for-byte unchanged.
- **Tests:**
  - parsing: absent gives false / 0; `true` / `3` round-trip; `300`, `-1` and
    `"2"` load as 0; `==` and `copyWith` cover the new fields;
  - `setColorHarmony` sends one settingsBlob write whose payload holds the
    surface object with only the passed keys and `reboot: false`. Inspect it
    the way `write_settings_blob_test.dart` does; with a password-protected
    seed, decrypt with `LampCrypto.decryptOpForTesting` and
    `uuidSaltLE16(BleUuids.settingsBlob)`;
  - state is updated, and reverted with the error rethrown when the write fails
    (`mutate_helper_test.dart` pattern);
  - `clearKnockout` keeps the flags: seed base with a non-empty knockout,
    `baseAutoColors: true` and `baseColorScheme: 3`, call `clearKnockout`, and
    assert the knockout is empty while `autoColors`, `colorScheme`,
    `drawIdleMa` and `drawFullMa` are kept.

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

### Task-F06: Top-first orientation for Shade and Base (P1)

**Priority**: High
**Estimated Iterations**: 2

**Files** (all under
`software/lamp-app-flutter/lib/features/control/presentation/widgets/`):
- `color_stops_sheet.dart`, `shade_editor_sheet.dart`, `base_editor_sheet.dart`
- `shade_card.dart`, `base_card.dart`, `lamp_preview.dart`
- their tests

**Steps**:
- **`ColorStopsSheet` gains `topFirst`** (bool, default false). Don't add it to
  `showColorStopsSheet`; no other caller needs it. `ShadeEditorSheet` and
  `BaseEditorSheet` pass `true`.
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
- **Previews.** `ShadeCard`, `BaseCard` and `LampPreview` render storage index 0
  at the bottom: reverse the list before building the gradient or the SVG
  stops. They show Shade/Base only, so flip them unconditionally.
- **Tests** (see "Test harness notes"):
  - a `topFirst` sheet with storage `[red, green, blue]` shows blue in row
    `color-stop-0` and red in the last row;
  - dragging display row 0 below display row 1 yields the expected storage list
    in `onChanged`;
  - Add inserts at storage index 0, seeded from it;
  - `ShadeCard` and `BaseCard` seeded with storage `[red, blue]` render a
    `LinearGradient` whose colors are `[blue.toSwatch(), red.toSwatch()]` top
    to bottom; `LampPreview` emits the stops in the same order;
  - without `topFirst`, behavior is unchanged (existing tests);
  - update existing Shade/Base editor tests whose expectations depend on row
    order or Add position. Keep what they verify.

**Acceptance Criteria**:
- [ ] Shade and Base sheets list the top of the lamp first. Cards and the
      critter preview match the lists, covered by tests.
- [ ] Expression and wisp sheets are unchanged.
- [ ] All tests pass. `npm run app:analyze` adds no issues.

**Verification**:

```bash
npm run app:test -- test/features/control/presentation/widgets/
npm run app:analyze
```

---

### Task-F07: Swipe-dismiss reverts like Cancel (P11)

**Priority**: High
**Estimated Iterations**: 1

**Files**:
- `software/lamp-app-flutter/lib/features/control/presentation/widgets/color_stops_sheet.dart`
- `software/lamp-app-flutter/test/features/control/presentation/widgets/color_stops_sheet_test.dart`

**Steps**:
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

### Task-F08: Auto-pick in ColorStopsSheet

**Priority**: High
**Estimated Iterations**: 3-4

**Files**:
- `software/lamp-app-flutter/lib/features/control/presentation/widgets/color_stops_sheet.dart`
- `software/lamp-app-flutter/test/features/control/presentation/widgets/color_stops_sheet_test.dart`

**Steps**:
- **Opt-in API.** `ColorStopsSheet` gains an optional `autoPick` of type
  `AutoPickConfig?`, defined in the same file. Null means no auto-pick UI or
  behavior. Don't add it to `showColorStopsSheet`.

  ```dart
  class AutoPickConfig {
    const AutoPickConfig({
      required this.initialAuto,
      required this.initialScheme,
      required this.persist,
    });
    final bool initialAuto;
    final int initialScheme;
    /// Writes the flags. `colorScheme` is non-null only when the user picked
    /// one. Throws on failure.
    final Future<void> Function({required bool autoColors, int? colorScheme})
        persist;
  }
  ```

- **State.** `_auto`, `_schemeId`, and `_schemePicked = false`, initialized in
  `initState` from the config. Never re-read from props: the Shade/Base parents
  rebuild and re-pass props on every live write.
- **Primary.** The last storage index (D1), which F06 shows as the first row.
- **Header row (P3, P4),** passed as the `ReorderableListView` `header`, one
  compact row:
  - left: "Auto-pick colors" (bodyMedium) over the scheme name (bodySmall,
    `onSurfaceVariant`). F09 turns the scheme name into a link;
  - right: a `Switch` with `Key('auto-pick-switch')`;
  - when auto is on and `isNeutralColor(primary)`, add the muted line "Pick a
    more colorful primary to vary the other colors." under the scheme name.
    Keep the scheme name; it is the only way into the scheme sheet;
  - hidden when the list is empty.
- **Regeneration.** One synchronous helper:
  `harmonizePalette(_colors, _schemeId, count: n)`, then `_apply`. Call it only
  from:
  - toggle on: `n = length == 1 ? naturalCount : min(length, 5)` (D4, P7);
  - a primary edit while on: in both the picker's `onLive` and its Done
    result, replace the primary, then regenerate. Picker Cancel keeps the
    existing whole-list restore (see D3);
  - Add while on: `n = length + 1`. Hide Add while on and `length >= 5`;
  - removing a derived row while on: `n = length - 1`.
- **Toggle off:** `setState` only. Colors unchanged; rows become editable.
- **While on (D5):**
  - derived rows have `onTap: null`;
  - the primary row has no remove button;
  - derived rows keep their remove button;
  - every row keeps its handle icon, wrapped in
    `ReorderableDragStartListener(enabled: false)`, so nothing shifts when the
    switch flips.

  Style the disabled state explicitly; `ListTile(enabled: false)` and
  `onPressed: null` don't dim the custom bar or the explicitly colored hex. Dim
  the hex text and the handle icon to 0.4 opacity. Keep the color bar at full
  strength, since it shows what the lamp shows. Give derived rows a `Tooltip`
  and Semantics label: "Auto-picked from the top color. Turn off Auto-pick
  colors to edit." If you wrap a row, keep the `ValueKey` on the outermost
  widget `itemBuilder` returns.
- **Dirty check:** colors differ from the opened list, OR
  `_auto != initialAuto`, OR (`_schemePicked` and
  `_schemeId != initialScheme`).
- **Save:** if the flags changed, first
  `await autoPick.persist(autoColors: _auto, colorScheme: _schemePicked ? _schemeId : null)`,
  then the existing `onSave`. Either one throwing keeps today's failure path
  (snackbar, sheet stays open).
- Never regenerate in `initState`, `build`, or on parent rebuilds (D3).

**Tests** (use `topFirst: true`; see "Test harness notes"):
1. Toggle on with 1 color gives the natural count (Analogous: 3), primary at
   the last storage index.
2. Toggle on with 4 manual colors overwrites colors 2–4; count stays 4.
3. Toggle on with 6 colors leaves 5.
4. Toggle off emits no `onChanged`; tapping a non-primary row opens the picker
   again.
5. A primary edit while on (open row `color-stop-0`, type a hex into
   `hex-field`, tap Done) regenerates the whole palette; `onLive` regenerates
   too.
6. A primary edit while off changes only the primary.
7. While on: tapping a derived row opens no picker; the primary row has no
   `Icons.close`; every `ReorderableDragStartListener` in the list has
   `enabled == false`.
8. Add while on gives count +1, regenerated, and hides at 5. Removing a derived
   row gives count −1, regenerated.
9. Load safety: opening with `initialAuto: true` and colors that don't match
   the generator emits nothing and shows them as saved. Rebuilding the parent
   with a different `initial` emits nothing.
10. The flags alone make the sheet dirty: open with `initialAuto: true`, toggle
    off (colors unchanged), then system back shows the discard dialog.
11. A neutral primary with auto on shows the hint line, and the scheme name is
    still shown.
12. Save with changed flags calls `persist` with the right arguments before
    `onSave`. Save with unchanged flags doesn't call it.

**Acceptance Criteria**:
- [ ] All 12 tests and all existing tests pass. `npm run app:analyze` adds no
      issues.
- [ ] Without `autoPick`, the sheet renders and behaves as it did after F07.
- [ ] Generation is synchronous and runs only from the listed handlers.

**Verification**:

```bash
npm run app:test -- test/features/control/presentation/widgets/color_stops_sheet_test.dart
npm run app:test
npm run app:analyze
```

---

### Task-F09: Scheme picker sub-sheet and editor wiring

**Priority**: High
**Estimated Iterations**: 2-3

**Files**:
- `software/lamp-app-flutter/lib/features/control/presentation/widgets/harmony_scheme_sheet.dart` (new)
- `color_stops_sheet.dart`, `shade_editor_sheet.dart`, `base_editor_sheet.dart`
  (same folder)
- tests under `software/lamp-app-flutter/test/features/control/presentation/widgets/`:
  `harmony_scheme_sheet_test.dart` (new), `color_stops_sheet_test.dart`,
  `shade_editor_sheet_test.dart`, `base_editor_sheet_test.dart`

**Steps**:
- **The sub-sheet.**
  `Future<int?> showHarmonySchemeSheet(BuildContext context, {required LampColor primary, required int selectedId})`,
  built on `showAppSheet`.
  - Title "Color harmony". Muted line: "The scheme Auto-pick colors uses for
    this strip."
  - One row per scheme, `Key('harmony-scheme-<key>')`: name (bodyMedium),
    description (bodySmall), and a strip of small swatches from
    `harmonyColors(primary, id, naturalCount)` (reuse `LampColorSwatch` or
    `LampColor.toSwatch()`).
  - The selected row has a trailing
    `Icon(Icons.check, color: colorScheme.primary)` (precedent:
    `setup_screen.dart`). `harmonySchemeById(selectedId)` decides the
    selection, so an unknown id shows Analogous checked.
  - Tapping a row pops its id. Dismissal pops null. The body scrolls. Follow
    `FORM_STYLING.md`.
- **The link.** In `ColorStopsSheet`'s header, the scheme name becomes a
  `TextButton` "‹scheme name› · Change" (`Key('harmony-scheme-button')`). It
  stays visible when the neutral hint shows. It opens the sub-sheet with the
  current primary and `_schemeId`. On a result: set `_schemeId`, set
  `_schemePicked = true`, and, when auto is on, regenerate at the current
  length (D3).
- **Editors.**
  - `ShadeEditorSheet`: `topFirst: true` always (F06). Pass `autoPick` only
    when `state.shade.segments.length <= 1` (P2):
    `initialAuto: state.shade.autoColors`,
    `initialScheme: state.shade.colorScheme`, and
    `persist: ({required autoColors, colorScheme}) => notifier.setColorHarmony(surface: HarmonySurface.shade, autoColors: autoColors, colorScheme: colorScheme)`.
  - `BaseEditorSheet`: the same with `state.base` and `HarmonySurface.base`.
  - Don't change either editor's `onSave`. The Save order is F08's: `persist`
    when the flags changed, then `onSave` (live colors, then `commit()`), so the
    flags land before the commit (P6). Don't move `persist` into `onSave`.
- **Stale comments.** Fix the `_kMaxShadeStops` and `_kMaxBaseStops` comments:
  firmware renders at most 8 stops; it doesn't handle arbitrary N. State the
  limit without a source path.
- **Tests:**
  - sub-sheet: six rows; the selected row has the check; an unknown id checks
    Analogous; tapping a row returns its id;
  - in `color_stops_sheet_test.dart`: a scheme pick while on regenerates at the
    current count; while off, colors don't change; with auto off, picking a
    different scheme alone makes the sheet dirty (system back shows the
    dialog); with a neutral primary and auto on, `harmony-scheme-button` is
    present;
  - Shade and Base editors with the seeded lamp: toggle on, then expect a
    colors write (`ble.writesTo(..., BleUuids.shadeColors)` /
    `BleUuids.baseColors`) carrying the regenerated list. Save, then expect
    exactly one settingsBlob write whose surface object carries `autoColors`
    (and `colorScheme` only when picked) with `reboot: false`, and at least one
    commit write. The shade editor seed uses password `secret`, so decrypt the
    settingsBlob write with `LampCrypto.decryptOpForTesting` and
    `uuidSaltLE16(BleUuids.settingsBlob)`. `writesTo` is per characteristic,
    so don't assert order across characteristics; F08 test 12 pins `persist`
    before `onSave`;
  - a seed with `shadeAutoColors: true` and colors that don't match the
    generator shows the switch on, and opening causes no colors write;
  - a shade seed with three segments (`shadeSegmentsJson`) shows no auto-pick
    switch;
  - existing editor tests stay green. Where the header pushes rows down, switch
    them to keyed finders plus `ensureVisible`, without changing what they
    assert.

**Acceptance Criteria**:
- [ ] Single-segment Shade and Base sheets show auto-pick. Multi-segment
      shades, expression colors and the wisp palette don't.
- [ ] Flags reach the lamp only on Save, before the commit.
- [ ] All tests pass. `npm run app:analyze` adds no issues.

**Verification**:

```bash
npm run app:test -- test/features/control/presentation/widgets/
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
  - add a "Flutter client" section: where the generator, wheel and sheets live;
    the shared golden fixture; Flutter's top-first orientation; auto-pick only
    on single-segment surfaces; flags written on Save via settings_blob with
    `reboot: false`;
  - in the tuning and golden-regen instructions, state that a table change
    also updates the Dart table in the same commit and must pass
    `npm run app:test` (no CI runs `ui:test` or `app:test`);
  - fix any sentence that still describes the Flutter app as not reading or
    writing `autoColors` / `colorScheme`;
  - apply any TypeScript-vs-doc corrections recorded in F02.
- **`software/lamp-app-flutter/test/manual/v1-checklist.md`:** replace lines 42
  ("hue ring"), 67 ("4 sliders") and 72 ("R/G/B + Warm White") with wheel and
  Brightness checks. Rewrite lines 17, 21 and 43 for the stops sheet: Shade
  opens a list of colors, top row first; Add inserts a copy of the bottom color
  at the bottom. Add auto-pick, scheme and swipe-revert checks.
- **`software/lamp-app-flutter/docs/FORM_STYLING.md`:** change it only if a new
  idiom shipped. The header row reuses the Row + Switch idiom, so normally no
  change.
- **Comment sweep** over the phase-2 diff
  (`git diff <phase-2 base commit>..HEAD -- software/lamp-app-flutter docs`).
  Grep added lines for `Note:`, `Why:`, ` we `, `previously`, `for now`,
  `placeholder`, `Phase `, em-dashes, untagged `TODO`, `/Users/`, `Mirror`,
  `Ported`, `port of`, `.ts`, `lamp-ui/` and `lamp-os/src`. Fix every hit in
  code comments.
- Confirm `dependencies` and `dev_dependencies` in `pubspec.yaml` are
  unchanged.

**Acceptance Criteria**:
- [ ] Both commands pass against the baseline.
- [ ] Docs updated. Sweep clean. `pubspec.yaml` unchanged.
- [ ] `PROGRESS.md` shows every F-task complete.

---

## Manual verification on a lamp (for the user, after Task-F10)

Executors can't do this step. The app updates its state optimistically and
re-reads the lamp only on connect, so persistence checks require leaving the
lamp and reconnecting.

1. **Install** the app on a phone (`npm run app:install` or `npm run app:run`)
   and connect to a lamp running phase 1 firmware.
2. **Wheel.** Dragging previews live. Vertical drags on the wheel don't move
   the sheet. Brightness works. Hex, sliders and wheel stay in sync. Cancel
   reverts the lamp.
3. **Orientation.** The top Shade row is the color at the top of the shade, and
   the top Base row is the color at the top of the base. The cards and the
   critter preview agree.
4. **Auto-pick.**
   - Toggle on: colors fill in, derived rows dim.
   - Drag the primary: every color follows live on the lamp.
   - Toggle off: the colors stay.
   - Add / remove while on: regenerates. Add stops at 5.
5. **Schemes.** Change the scheme from the auto-pick row. Shade and Base keep
   separate schemes.
6. **Persistence and parity.**
   - Save, back out of the lamp (or power-cycle it), reconnect, and reopen the
     sheet: toggle, scheme and colors persist.
   - Open the web UI: it shows the same toggle and scheme.
   - Change the scheme in the web UI, reconnect the app, and reopen the sheet:
     it shows the web's scheme and the colors exactly as saved.
7. **Swipe.** Edit Shade colors, then swipe the sheet down: the lamp reverts.
   Repeat on an expression's Colors sheet: the expression's draft colors revert.
8. **Older firmware.** On a lamp without phase 1 firmware, auto-pick still
   generates colors. After backing out and reconnecting, the toggle reads off.
   Reopening the sheet without reconnecting still shows it on.
9. **Snafu.** Shade segment sheets show no auto-pick switch. Base does.

## Known pre-existing issues (out of scope; don't fix here)

- **`bpp` is always 4 after a section read.** Firmware never emits `bpp`, so
  the Warm White slider also shows on GRB/BGR strips, which discard W. D6
  copies W anyway, which is harmless there.
- **Shade segments above 0 open no edit session,** so wisp paint can fight
  their live preview.
- **On-screen gradients understate LED mid-strip darkness.** Firmware blends
  stored bytes and only then applies gamma 2.6, so contrast schemes show dim,
  greyish bands between distant hues on the lamp. Phase 1's manual step 8
  covers tuning.
- **The web UI writes one palette to every segment on save** and serves a
  boot-time settings snapshot.

## Technical constraints

- Dart >= 3.12 (Flutter 3.44+), Riverpod 3 (dev pre-release),
  `flutter_test` + `mocktail`.
- Analyzer: `flutter_lints` plus `require_trailing_commas`, `prefer_const_*`,
  `strict-casts`, `strict-raw-types`, `unawaited_futures`. Info-level issues
  fail `flutter analyze`.
- Use the current `Color` API: `.r/.g/.b` doubles with `(c.r * 255).round()`,
  `toARGB32()`, `withValues(alpha:)`. Never `.red`, `.value` or `withOpacity`.
- No `Radio`; its members are deprecated on this SDK.
- Spacing uses `AppSpace` / `AppRadius` tokens. Colors come from `colorScheme`
  / `brandExtras`. Component literals carry
  `// deliberate dimension, not spacing`. In-sheet layouts use the loose idiom,
  never `FormSection` / `LampCard`.

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
- **No speculative API.** `topFirst` and `autoPick` exist on `ColorStopsSheet`
  only because Shade/Base use them. Don't thread them through
  `showColorStopsSheet`. `ColorWheel` has no `enabled` flag.
- **Reuse before you build:** `LampColor`, `LampColorSwatch`, `showAppSheet`,
  `confirmDiscard`, `_mutate`, `writeSettingsBlob`, `_ChannelSlider`.

## Out of scope

- Auto-pick for expression colors and the wisp manual palette. They get the
  wheel, not the toggle.
- Per-segment auto-pick on multi-segment shades.
- Harmony markers drawn on the wheel.
- Any change to firmware, `software/lamp-ui`, protocol constants or
  `kGattLayout`.
- Fixing the pre-existing issues listed above.
- A `VERSION` bump, a `CHANGELOG` entry, or any release.
