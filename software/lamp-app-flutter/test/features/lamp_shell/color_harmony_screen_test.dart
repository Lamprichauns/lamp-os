import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/core/ble/ble_client_provider.dart';
import 'package:lamp_app/core/ble/lamp_crypto.dart';
import 'package:lamp_app/core/ble/uuids.dart';
import 'package:lamp_app/core/theme/app_theme.dart';
import 'package:lamp_app/core/widgets/app_snackbar.dart';
import 'package:lamp_app/features/control/application/control_notifier.dart';
import 'package:lamp_app/features/control/application/control_state.dart';
import 'package:lamp_app/features/control/domain/color_harmony.dart';
import 'package:lamp_app/features/control/domain/lamp_color.dart';
import 'package:lamp_app/features/control/presentation/widgets/lamp_color_swatch.dart';
import 'package:lamp_app/features/inventory/application/inventory_notifier.dart';
import 'package:lamp_app/features/inventory/domain/inventory_lamp.dart';
import 'package:lamp_app/features/lamp_shell/presentation/color_harmony_screen.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../_support/in_memory_ble_client.dart';
import '../../_support/seed.dart';

const _devId = 'lamp-x';
const _red = '#FF0000C8';

Future<(ProviderContainer, InMemoryBleClient)> _build({
  String shadeColorsJson = '["#000000FF"]',
  bool? shadeAutoColors,
  int? shadeColorScheme,
  bool? shadeColorsEditable,
  bool? baseAutoColors,
  int? baseColorScheme,
  bool? baseColorsEditable,
}) async {
  SharedPreferences.setMockInitialValues({});
  final ble = InMemoryBleClient();
  await seedControlBle(
    ble,
    deviceId: _devId,
    shadeColorsJson: shadeColorsJson,
    shadeAutoColors: shadeAutoColors,
    shadeColorScheme: shadeColorScheme,
    shadeColorsEditable: shadeColorsEditable,
    baseAutoColors: baseAutoColors,
    baseColorScheme: baseColorScheme,
    baseColorsEditable: baseColorsEditable,
  );
  final c = ProviderContainer(
    overrides: [bleClientProvider.overrideWithValue(ble)],
  );
  await c.read(inventoryNotifierProvider.future);
  // No control password, so settings_blob writes are plaintext and decodable.
  await c.read(inventoryNotifierProvider.notifier).add(const InventoryLamp(
        id: _devId,
        name: 'jacko',
      ));
  return (c, ble);
}

Future<void> _pumpScreen(WidgetTester tester, ProviderContainer c) async {
  tester.view.physicalSize = const Size(800, 1600);
  tester.view.devicePixelRatio = 1;
  addTearDown(tester.view.reset);
  await tester.pumpWidget(UncontrolledProviderScope(
    container: c,
    child: MaterialApp(
      theme: appTheme,
      home: const ColorHarmonyScreen(lampId: _devId),
    ),
  ));
  for (var i = 0; i < 30; i++) {
    await tester.pump(const Duration(milliseconds: 16));
    if (find.text('AUTO-PICK COLORS FOR').evaluate().isNotEmpty ||
        find.text('No Auto-pick on this lamp').evaluate().isNotEmpty) {
      return;
    }
  }
}

/// Lets the colors coalescer and the inventory "seen" flush run out.
Future<void> _drain(WidgetTester tester) =>
    tester.pump(const Duration(milliseconds: 600));

ControlState _state(ProviderContainer c) =>
    c.read(controlNotifierProvider(_devId)).value!;

Set<HarmonySurface> _selected(WidgetTester tester) => tester
    .widget<SegmentedButton<HarmonySurface>>(
        find.byKey(const Key('auto-pick-surfaces')))
    .selected;

Finder _inSummary(Finder finder) =>
    find.descendant(of: find.byKey(const Key('scheme-summary')), matching: finder);

Finder _checkIn(String schemeKey) => find.descendant(
      of: find.byKey(Key('harmony-scheme-$schemeKey')),
      matching: find.byIcon(Icons.check),
    );

List<Map<String, dynamic>> _blobs(InMemoryBleClient ble) =>
    ble.writesTo(_devId, BleUuids.settingsBlob).map((w) {
      expect(w[0], LampCrypto.magicPlaintext);
      return jsonDecode(utf8.decode(w.sublist(1))) as Map<String, dynamic>;
    }).toList();

List<LampColor> _lastColors(InMemoryBleClient ble, String charUuid) =>
    (jsonDecode(utf8.decode(ble.writesTo(_devId, charUuid).last)) as List)
        .map((h) => LampColor.fromHex(h as String))
        .toList();

Future<void> _tapAndWrite(WidgetTester tester, Finder finder) async {
  await tester.tap(finder);
  await tester.pump();
  await tester.pump(const Duration(milliseconds: 100));
}

void main() {
  testWidgets('default seed: nothing selected, Analogous, list collapsed',
      (tester) async {
    final (c, _) = await _build();
    addTearDown(c.dispose);
    await _pumpScreen(tester, c);

    expect(_selected(tester), isEmpty);
    expect(find.text('Shade'), findsOneWidget);
    expect(find.text('Base'), findsOneWidget);
    expect(_inSummary(find.text('Analogous')), findsOneWidget);
    expect(_inSummary(find.byType(LampColorSwatch)), findsNWidgets(3));
    expect(find.byKey(const Key('harmony-scheme-analogous')), findsNothing);
    expect(find.text('Change'), findsOneWidget);

    await tester.tap(find.byKey(const Key('scheme-toggle')));
    await tester.pumpAndSettle();
    expect(find.text('Hide'), findsOneWidget);
    for (final scheme in kHarmonySchemes) {
      expect(find.byKey(Key('harmony-scheme-${scheme.key}')), findsOneWidget);
      expect(find.text(scheme.description), findsOneWidget);
    }
    expect(_checkIn('analogous'), findsOneWidget);
    expect(find.byIcon(Icons.check), findsOneWidget);

    await tester.tap(find.byKey(const Key('scheme-toggle')));
    await tester.pumpAndSettle();
    expect(find.text('Change'), findsOneWidget);
    expect(find.byKey(const Key('harmony-scheme-analogous')), findsNothing);
  });

  testWidgets(
      'Shade chip on streams the auto-picked palette, writes the flag, '
      'commits', (tester) async {
    final (c, ble) = await _build(shadeColorsJson: '["$_red"]');
    addTearDown(c.dispose);
    await _pumpScreen(tester, c);

    await _tapAndWrite(tester, find.text('Shade'));

    final expected =
        harmonizePalette([LampColor.fromHex(_red)], 0, count: 3);
    expect(expected.last, LampColor.fromHex(_red));
    expect(_lastColors(ble, BleUuids.shadeColors), expected);
    expect(_state(c).shade.colors, expected);
    expect(_blobs(ble), [
      {
        'shade': {'autoColors': true},
        'reboot': false,
      },
    ]);
    expect(ble.writesTo(_devId, BleUuids.commit), isNotEmpty);
    expect(_selected(tester), {HarmonySurface.shade});
    await _drain(tester);
  });

  testWidgets('Shade chip off writes the flag only and keeps the colors',
      (tester) async {
    const seeded = '["#00FF00C8","$_red"]';
    final (c, ble) =
        await _build(shadeColorsJson: seeded, shadeAutoColors: true);
    addTearDown(c.dispose);
    await _pumpScreen(tester, c);
    final before = _state(c).shade.colors;
    expect(_selected(tester), {HarmonySurface.shade});

    await _tapAndWrite(tester, find.text('Shade'));

    expect(_blobs(ble), [
      {
        'shade': {'autoColors': false},
        'reboot': false,
      },
    ]);
    expect(ble.writesTo(_devId, BleUuids.shadeColors), isEmpty);
    expect(_state(c).shade.colors, before);
    expect(_selected(tester), isEmpty);
    await _drain(tester);
  });

  testWidgets(
      'a scheme pick writes both surfaces once and regenerates only the '
      'on surface', (tester) async {
    const seeded = '["#00FF00C8","#0000FFC8","$_red"]';
    final (c, ble) =
        await _build(shadeColorsJson: seeded, shadeAutoColors: true);
    addTearDown(c.dispose);
    await _pumpScreen(tester, c);
    final before = _state(c).shade.colors;

    await tester.tap(find.byKey(const Key('scheme-toggle')));
    await tester.pumpAndSettle();
    await _tapAndWrite(tester, find.byKey(const Key('harmony-scheme-triadic')));

    expect(_blobs(ble), [
      {
        'shade': {'colorScheme': 4},
        'base': {'colorScheme': 4},
        'reboot': false,
      },
    ]);
    final expected = harmonizePalette(before, 4);
    expect(expected, isNot(before));
    expect(_lastColors(ble, BleUuids.shadeColors), expected);
    expect(ble.writesTo(_devId, BleUuids.baseColors), isEmpty);
    expect(ble.writesTo(_devId, BleUuids.commit), hasLength(1));

    await tester.pumpAndSettle();
    expect(find.text('Hide'), findsOneWidget);
    expect(find.byKey(const Key('harmony-scheme-analogous')), findsOneWidget);
    expect(_checkIn('triadic'), findsOneWidget);
    expect(find.byIcon(Icons.check), findsOneWidget);
    expect(_inSummary(find.text('Triadic')), findsOneWidget);
    await _drain(tester);
  });

  testWidgets('differing schemes show Mixed until a pick', (tester) async {
    final (c, _) = await _build(shadeColorScheme: 4, baseColorScheme: 5);
    addTearDown(c.dispose);
    await _pumpScreen(tester, c);

    const note =
        'Shade uses Triadic, Base uses Square. Picking a scheme sets both.';
    expect(_inSummary(find.text('Mixed')), findsOneWidget);
    expect(_inSummary(find.byType(LampColorSwatch)), findsNothing);
    expect(find.text(note), findsOneWidget);

    await tester.tap(find.byKey(const Key('scheme-toggle')));
    await tester.pumpAndSettle();
    expect(find.byIcon(Icons.check), findsNothing);

    await _tapAndWrite(tester, find.byKey(const Key('harmony-scheme-analogous')));
    await tester.pumpAndSettle();
    expect(_inSummary(find.text('Analogous')), findsOneWidget);
    expect(find.text(note), findsNothing);
    expect(_checkIn('analogous'), findsOneWidget);
    await _drain(tester);
  });

  testWidgets('an unknown scheme id compares as Analogous', (tester) async {
    final (c, _) = await _build(shadeColorScheme: 0, baseColorScheme: 99);
    addTearDown(c.dispose);
    await _pumpScreen(tester, c);

    expect(_inSummary(find.text('Analogous')), findsOneWidget);
    expect(find.textContaining('Picking a scheme sets both'), findsNothing);
  });

  testWidgets(
      'an ineligible Base has no segment and does not drive the preview',
      (tester) async {
    final (c, _) = await _build(
      shadeColorsJson: '["$_red"]',
      baseColorsEditable: false,
      baseAutoColors: true,
    );
    addTearDown(c.dispose);
    await _pumpScreen(tester, c);

    expect(find.text('Shade'), findsOneWidget);
    expect(find.text('Base'), findsNothing);
    expect(_selected(tester), isEmpty);
    final swatches = tester
        .widgetList<LampColorSwatch>(_inSummary(find.byType(LampColorSwatch)))
        .map((w) => w.color)
        .toList();
    expect(
      swatches,
      harmonyColors(LampColor.fromHex(_red), 0, 3).reversed.toList(),
    );
  });

  testWidgets('a failed chip write reverts the chip and shows a snackbar',
      (tester) async {
    final (c, ble) = await _build(shadeColorsJson: '["$_red"]');
    addTearDown(c.dispose);
    await _pumpScreen(tester, c);
    ble.scheduleEncryptionFailure(
        _devId, BleUuids.controlService, BleUuids.settingsBlob);

    await _tapAndWrite(tester, find.text('Shade'));
    await tester.pump();

    expect(_selected(tester), isEmpty);
    expect(find.text("Couldn't update Auto-pick. Try again."), findsOneWidget);
    expect(ble.writesTo(_devId, BleUuids.commit), isEmpty);
    await _drain(tester);
    await tester.pump(AppSnackbar.errorDuration);
    await tester.pumpAndSettle();
  });

  testWidgets('no eligible surface shows the empty state', (tester) async {
    final (c, _) = await _build(
      shadeColorsEditable: false,
      baseColorsEditable: false,
    );
    addTearDown(c.dispose);
    await _pumpScreen(tester, c);

    expect(find.text('No Auto-pick on this lamp'), findsOneWidget);
    expect(find.byKey(const Key('auto-pick-surfaces')), findsNothing);
  });
}
