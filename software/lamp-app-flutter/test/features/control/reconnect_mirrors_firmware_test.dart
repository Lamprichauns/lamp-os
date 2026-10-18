// A reconnect re-reads every section, so live state mirrors what the lamp
// stored: an OTA-bumped firmware version reaches inventory, and an
// optimistic Auto-pick flag survives only if the firmware kept it.

import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../_support/in_memory_ble_client.dart';
import '../../_support/seed.dart';
import 'package:lamp_app/core/ble/ble_client_provider.dart';
import 'package:lamp_app/core/ble/lamp_crypto.dart';
import 'package:lamp_app/core/ble/uuids.dart';
import 'package:lamp_app/features/control/application/control_notifier.dart';
import 'package:lamp_app/features/control/application/control_state.dart';
import 'package:lamp_app/features/inventory/application/inventory_notifier.dart';
import 'package:lamp_app/features/inventory/domain/inventory_lamp.dart';

const _devId = 'lamp-reconnect-fw';

int _invFwVersion(ProviderContainer c) => c
    .read(inventoryNotifierProvider)
    .value!
    .firstWhere((l) => l.id == _devId)
    .fwVersion!;

ControlState _state(ProviderContainer c) =>
    c.read(controlNotifierProvider(_devId)).value!;

Future<void> _pump() async {
  for (var i = 0; i < 20; i++) {
    await Future<void>.delayed(Duration.zero);
  }
}

/// Connects a passwordless lamp, so settings_blob writes are plaintext.
Future<ProviderContainer> _connect(InMemoryBleClient ble) async {
  final c = ProviderContainer(
    overrides: [bleClientProvider.overrideWithValue(ble)],
  );
  addTearDown(c.dispose);
  await c.read(inventoryNotifierProvider.future);
  await c.read(inventoryNotifierProvider.notifier).add(const InventoryLamp(
        id: _devId,
        name: 'test-lamp',
        controlPassword: '',
      ));
  c.listen(controlNotifierProvider(_devId), (_, _) {});
  await c.read(controlNotifierProvider(_devId).future);
  await _pump();
  return c;
}

/// Drops then restores the link; the restore edge kicks _tryReconnect.
Future<void> _reconnect(InMemoryBleClient ble) async {
  await ble.disconnect(_devId);
  await _pump();
  await ble.connect(_devId);
  await _pump();
}

/// Turns the Base chip on and lets the colors coalescer and the inventory
/// "seen" flush run out before the link drops.
Future<void> _autoPickBase(ProviderContainer c, InMemoryBleClient ble) async {
  await c
      .read(controlNotifierProvider(_devId).notifier)
      .setAutoPick(HarmonySurface.base, true);
  await Future<void>.delayed(const Duration(milliseconds: 600));
  final blobs = ble.writesTo(_devId, BleUuids.settingsBlob);
  expect(blobs, hasLength(1));
  expect(blobs.single[0], LampCrypto.magicPlaintext);
  expect(jsonDecode(utf8.decode(blobs.single.sublist(1))), {
    'base': {'autoColors': true},
    'reboot': false,
  });
  expect(_state(c).base.autoColors, isTrue);
}

void main() {
  setUp(() => SharedPreferences.setMockInitialValues({}));

  test('reconnect after a version bump refreshes inventory + live state',
      () async {
    final ble = InMemoryBleClient();
    await seedControlBle(ble,
        deviceId: _devId, fwVersion: 10205, fwChannel: 'beta');
    final c = await _connect(ble);

    expect(_invFwVersion(c), 10205);

    // Lamp gets flashed to a newer version while the link is down.
    ble.seedSection(
      _devId,
      'lamp',
      Uint8List.fromList(utf8.encode(
        '{"name":"test-lamp","brightness":50,"advancedEnabled":false,'
        '"brightnessCeiling":170,"fwVersion":10206,"fwChannel":"beta"}',
      )),
    );

    await _reconnect(ble);

    final live = c.read(controlNotifierProvider(_devId)).value!.lamp.fwVersion;
    expect(live, 10206, reason: 'Info page (live section) should be fresh');
    expect(_invFwVersion(c), 10206,
        reason: 'Social self-card (inventory) should match live version');
  });

  group('Auto-pick after a reconnect', () {
    test('stays on when the lamp stored autoColors', () async {
      final ble = InMemoryBleClient();
      await seedControlBle(ble, deviceId: _devId);
      final c = await _connect(ble);
      await _autoPickBase(c, ble);

      await seedControlBle(ble, deviceId: _devId, baseAutoColors: true);
      await _reconnect(ble);

      expect(_state(c).connected, isTrue);
      expect(_state(c).base.autoColors, isTrue);
    });

    test('drops back to off when older firmware omits the keys', () async {
      final ble = InMemoryBleClient();
      await seedControlBle(ble, deviceId: _devId);
      final c = await _connect(ble);
      await _autoPickBase(c, ble);

      await seedControlBle(ble, deviceId: _devId);
      await ble.disconnect(_devId);
      await _pump();
      expect(_state(c).base.autoColors, isTrue);
      await ble.connect(_devId);
      await _pump();

      expect(_state(c).connected, isTrue);
      expect(_state(c).base.autoColors, isFalse);
      expect(_state(c).base.colorScheme, 0);
    });
  });
}
