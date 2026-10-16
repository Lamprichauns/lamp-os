import 'dart:convert';

import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/core/ble/ble_client.dart';
import 'package:lamp_app/core/ble/ble_client_provider.dart';
import 'package:lamp_app/core/ble/lamp_crypto.dart';
import 'package:lamp_app/core/ble/uuids.dart';
import 'package:lamp_app/features/control/application/control_notifier.dart';
import 'package:lamp_app/features/control/application/control_state.dart';
import 'package:lamp_app/features/control/domain/color_harmony.dart';
import 'package:lamp_app/features/control/domain/lamp_color.dart';
import 'package:lamp_app/features/inventory/application/inventory_notifier.dart';
import 'package:lamp_app/features/inventory/domain/inventory_lamp.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../_support/in_memory_ble_client.dart';
import '../../_support/seed.dart';

const _devId = 'lamp-a';

Future<(ProviderContainer, InMemoryBleClient)> _build({
  String baseKnockoutJson = '[]',
  String shadeColorsJson = '["#000000FF"]',
  bool? shadeAutoColors,
  bool? baseAutoColors,
  int? baseColorScheme,
  bool? baseColorsEditable,
  String? shadeSegmentsJson,
}) async {
  final ble = InMemoryBleClient();
  await seedControlBle(
    ble,
    deviceId: _devId,
    baseKnockoutJson: baseKnockoutJson,
    shadeColorsJson: shadeColorsJson,
    shadeAutoColors: shadeAutoColors,
    baseAutoColors: baseAutoColors,
    baseColorScheme: baseColorScheme,
    baseColorsEditable: baseColorsEditable,
    shadeSegmentsJson: shadeSegmentsJson,
  );
  final c = ProviderContainer(
    overrides: [bleClientProvider.overrideWithValue(ble)],
  );
  await c.read(inventoryNotifierProvider.future);
  await c.read(inventoryNotifierProvider.notifier).add(const InventoryLamp(
        id: _devId,
        name: 'jacko',
      ));
  c.listen(controlNotifierProvider(_devId), (_, _) {});
  await c.read(controlNotifierProvider(_devId).future);
  return (c, ble);
}

ControlState _state(ProviderContainer c) =>
    c.read(controlNotifierProvider(_devId)).value!;

ControlNotifier _notifier(ProviderContainer c) =>
    c.read(controlNotifierProvider(_devId).notifier);

List<Map<String, dynamic>> _blobs(InMemoryBleClient ble) =>
    ble.writesTo(_devId, BleUuids.settingsBlob).map((w) {
      expect(w[0], LampCrypto.magicPlaintext);
      return jsonDecode(utf8.decode(w.sublist(1))) as Map<String, dynamic>;
    }).toList();

List<LampColor> _lastColors(InMemoryBleClient ble, String charUuid) =>
    (jsonDecode(utf8.decode(ble.writesTo(_devId, charUuid).last)) as List)
        .map((h) => LampColor.fromHex(h as String))
        .toList();

void main() {
  setUp(() => SharedPreferences.setMockInitialValues({}));

  group('setColorHarmony', () {
    test('a chip change writes only the named surface and flag', () async {
      final (c, ble) = await _build();
      addTearDown(c.dispose);

      await _notifier(c).setColorHarmony(
        surfaces: {HarmonySurface.shade},
        autoColors: true,
      );

      final blobs = _blobs(ble);
      expect(blobs, hasLength(1));
      expect(blobs.single, {
        'shade': {'autoColors': true},
        'reboot': false,
      });
    });

    test('a scheme pick writes colorScheme to every named surface', () async {
      final (c, ble) = await _build();
      addTearDown(c.dispose);

      await _notifier(c).setColorHarmony(
        surfaces: {HarmonySurface.shade, HarmonySurface.base},
        colorScheme: 4,
      );

      final blobs = _blobs(ble);
      expect(blobs, hasLength(1));
      expect(blobs.single, {
        'shade': {'colorScheme': 4},
        'base': {'colorScheme': 4},
        'reboot': false,
      });
    });

    test('updates state optimistically on the named surfaces only', () async {
      final (c, _) = await _build();
      addTearDown(c.dispose);
      final before = _state(c);

      final pending = _notifier(c).setColorHarmony(
        surfaces: {HarmonySurface.base},
        autoColors: true,
        colorScheme: 2,
      );
      expect(_state(c).base.autoColors, isTrue);
      expect(_state(c).base.colorScheme, 2);
      expect(_state(c).shade, before.shade);
      await pending;

      expect(_state(c).base, before.base.copyWith(
        autoColors: true,
        colorScheme: 2,
      ));
      expect(_state(c).shade, before.shade);
    });

    test('a null value leaves that field alone', () async {
      final (c, _) = await _build(baseAutoColors: true, baseColorScheme: 3);
      addTearDown(c.dispose);

      await _notifier(c).setColorHarmony(
        surfaces: {HarmonySurface.base},
        colorScheme: 5,
      );

      expect(_state(c).base.autoColors, isTrue);
      expect(_state(c).base.colorScheme, 5);
    });

    test('reverts and rethrows when the write fails', () async {
      final (c, ble) = await _build();
      addTearDown(c.dispose);
      final before = _state(c);

      ble.scheduleEncryptionFailure(
          _devId, BleUuids.controlService, BleUuids.settingsBlob);

      await expectLater(
        () => _notifier(c).setColorHarmony(
          surfaces: {HarmonySurface.shade, HarmonySurface.base},
          autoColors: true,
          colorScheme: 4,
        ),
        throwsA(isA<BleEncryptionRequired>()),
      );

      expect(_state(c), before);
    });
  });

  group('Auto-pick failure', () {
    test('setAutoPick streams the seed back and skips commit', () async {
      final (c, ble) = await _build(shadeColorsJson: '["#FF0000C8"]');
      addTearDown(c.dispose);
      final before = _state(c);

      ble.scheduleEncryptionFailure(
          _devId, BleUuids.controlService, BleUuids.settingsBlob);

      await expectLater(
        () => _notifier(c).setAutoPick(HarmonySurface.shade, true),
        throwsA(isA<BleEncryptionRequired>()),
      );

      expect(_state(c).shade.colors, before.shade.colors);
      expect(_state(c).shade.autoColors, isFalse);
      await Future<void>.delayed(const Duration(milliseconds: 100));
      expect(_lastColors(ble, BleUuids.shadeColors), before.shade.colors);
      expect(ble.writesTo(_devId, BleUuids.commit), isEmpty);
      await Future<void>.delayed(const Duration(milliseconds: 600));
    });

    test('pickColorScheme streams every snapshot back and skips commit',
        () async {
      final (c, ble) = await _build(
        shadeColorsJson: '["#00FF00C8","#0000FFC8","#FF0000C8"]',
        shadeAutoColors: true,
        baseAutoColors: true,
      );
      addTearDown(c.dispose);
      final before = _state(c);
      expect(harmonizePalette(before.shade.colors, 4),
          isNot(before.shade.colors));

      ble.scheduleEncryptionFailure(
          _devId, BleUuids.controlService, BleUuids.settingsBlob);

      await expectLater(
        () => _notifier(c).pickColorScheme(4),
        throwsA(isA<BleEncryptionRequired>()),
      );

      expect(_state(c), before);
      await Future<void>.delayed(const Duration(milliseconds: 100));
      expect(_lastColors(ble, BleUuids.shadeColors), before.shade.colors);
      expect(_lastColors(ble, BleUuids.baseColors), before.base.colors);
      expect(ble.writesTo(_devId, BleUuids.commit), isEmpty);
      await Future<void>.delayed(const Duration(milliseconds: 600));
    });
  });

  test('clearKnockout keeps the harmony flags and draw anchors', () async {
    final (c, _) = await _build(
      baseKnockoutJson: '[100,50,100,75]',
      baseAutoColors: true,
      baseColorScheme: 3,
    );
    addTearDown(c.dispose);
    final before = _state(c).base;
    expect(before.knockout, {1: 50, 3: 75});
    expect(before.drawIdleMa, isNot(0));
    expect(before.drawFullMa, isNot(0));

    await _notifier(c).clearKnockout();

    final after = _state(c).base;
    expect(after.knockout, isEmpty);
    expect(after.autoColors, isTrue);
    expect(after.colorScheme, 3);
    expect(after.drawIdleMa, before.drawIdleMa);
    expect(after.drawFullMa, before.drawFullMa);
  });

  group('Auto-pick eligibility', () {
    test('the default seed is eligible on both surfaces', () async {
      final (c, _) = await _build();
      addTearDown(c.dispose);
      final s = _state(c);

      expect(s.shade.harmonyEligible, isTrue);
      expect(s.base.harmonyEligible, isTrue);
      expect(s.isHarmonyEligible(HarmonySurface.shade), isTrue);
      expect(s.isHarmonyEligible(HarmonySurface.base), isTrue);
    });

    test('a three-segment shade is not eligible', () async {
      final (c, _) = await _build(
        shadeSegmentsJson: '[{"name":"Small Dots","px":12,"colors":["#000000FF"]},'
            '{"name":"Big Dots","px":12,"colors":["#000000FF"]},'
            '{"name":"Ring","px":14,"colors":["#000000FF"]}]',
      );
      addTearDown(c.dispose);
      final s = _state(c);

      expect(s.shade.harmonyEligible, isFalse);
      expect(s.isHarmonyEligible(HarmonySurface.shade), isFalse);
      expect(s.isHarmonyEligible(HarmonySurface.base), isTrue);
    });

    test('baseColorsEditable false makes Base ineligible', () async {
      final (c, _) = await _build(baseColorsEditable: false);
      addTearDown(c.dispose);
      final s = _state(c);

      expect(s.base.harmonyEligible, isFalse);
      expect(s.isHarmonyEligible(HarmonySurface.base), isFalse);
      expect(s.isHarmonyEligible(HarmonySurface.shade), isTrue);
    });
  });
}
