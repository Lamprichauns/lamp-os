import 'dart:async';
import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../../_support/in_memory_ble_client.dart';
import '../../../_support/scan_grace_override.dart';
import '../../../_support/seed.dart';
import 'package:lamp_app/core/ble/ble_client_provider.dart';
import 'package:lamp_app/core/ble/ble_scanner.dart';
import 'package:lamp_app/core/utils/clock.dart';
import 'package:lamp_app/core/widgets/status_dot.dart';
import 'package:lamp_app/features/control/application/control_notifier.dart';
import 'package:lamp_app/features/inventory/application/inventory_notifier.dart';
import 'package:lamp_app/features/inventory/domain/inventory_lamp.dart';
import 'package:lamp_app/features/inventory/domain/lamp_presence.dart';
import 'package:lamp_app/features/inventory/presentation/lamp_grid_tile.dart';
import 'package:lamp_app/features/inventory/presentation/my_lamps_screen.dart';
import 'package:lamp_app/features/nearby/application/nearby_lamps_notifier.dart';

/// Counts every connect and disconnect the app asks for. With [gate] set, the
/// next connect waits on it and then fails, like a slow attempt that loses.
class _RecordingBleClient extends InMemoryBleClient {
  final connects = <String>[];
  final disconnects = <String>[];
  Completer<void>? gate;

  @override
  Future<void> connect(String deviceId) async {
    connects.add(deviceId);
    final g = gate;
    if (g != null) {
      gate = null;
      await g.future;
      throw StateError('gated connect failed');
    }
    return super.connect(deviceId);
  }

  @override
  Future<void> disconnect(String deviceId) {
    disconnects.add(deviceId);
    return super.disconnect(deviceId);
  }
}

BleAdvertisement _adv(String id, String name) => BleAdvertisement(
      id: id,
      name: name,
      serviceUuids: const [],
      baseRgb: 0x300783,
      shadeRgb: 0x112233,
      rssi: -55,
      isMesh: true,
      configured: true,
    );

class _Rig {
  _Rig(this.container, this.ble, this.scanner, this._setNow, this._getNow);
  final ProviderContainer container;
  final _RecordingBleClient ble;
  final FakeBleScanner scanner;
  final void Function(DateTime) _setNow;
  final DateTime Function() _getNow;

  DateTime get now => _getNow();
  set now(DateTime t) => _setNow(t);
}

final _t0 = DateTime.fromMillisecondsSinceEpoch(1700000000000);

Future<_Rig> _mount(
  WidgetTester tester, {
  required List<InventoryLamp> lamps,
  String? activeId,
  void Function(_RecordingBleClient ble)? before,
}) async {
  SharedPreferences.setMockInitialValues({
    'inventory.v1': jsonEncode([for (final l in lamps) l.toJson()]),
    'active_lamp.v1': ?activeId,
  });
  var now = _t0;
  final ble = _RecordingBleClient();
  before?.call(ble);
  final scanner = FakeBleScanner();
  final c = ProviderContainer(overrides: [
    bleClientProvider.overrideWithValue(ble),
    bleScannerProvider.overrideWithValue(scanner),
    scanGraceTestOverride,
    clockProvider.overrideWithValue(() => now),
  ]);
  addTearDown(c.dispose);
  await c.read(inventoryNotifierProvider.future);
  await tester.pumpWidget(UncontrolledProviderScope(
    container: c,
    child: const MaterialApp(home: MyLampsScreen()),
  ));
  await tester.pump();
  return _Rig(c, ble, scanner, (t) => now = t, () => now);
}

/// Steps the fake clock to [at] and lets the list's ticker fire.
Future<void> _advanceTo(WidgetTester tester, _Rig rig, DateTime at) async {
  rig.now = at;
  await tester.pump(const Duration(seconds: 1));
}

Future<void> _hear(WidgetTester tester, _Rig rig, String id, String name) async {
  rig.scanner.emit(_adv(id, name));
  await tester.pump();
  await tester.pump(const Duration(milliseconds: 1100));
}

LampGridTile? _tile(WidgetTester tester, String name) {
  final f = find.byWidgetPredicate((w) => w is LampGridTile && w.name == name);
  return f.evaluate().isEmpty ? null : tester.widget<LampGridTile>(f);
}

/// Unmounts the list and tears the providers down so the ticker and the scan
/// roster's timers stop before the test's pending-timer check.
Future<void> _unmount(WidgetTester tester, _Rig rig) async {
  await tester.pumpWidget(const SizedBox());
  rig.container.dispose();
  await tester.pump(const Duration(seconds: 2));
}

void main() {
  const ms = Duration(milliseconds: 1);

  testWidgets(
      'a lamp the phone stops hearing is reachable, then unreachable, then '
      'hidden, with no new scan result to trigger it', (tester) async {
    final rig = await _mount(tester, lamps: const [
      InventoryLamp(id: 'a', name: 'alpha', lastBaseColor: [1, 2, 3, 4]),
    ]);
    expect(_tile(tester, 'Alpha'), isNull, reason: 'never seen yet');

    await _hear(tester, rig, 'a', 'alpha');
    expect(_tile(tester, 'Alpha')?.status, StatusKind.mesh);
    // Heard again inside the stored sighting's write step, so only the scan
    // roster knows this later time until the lamp ages out of it.
    final heardAt = rig.now.add(const Duration(seconds: 10));
    await _advanceTo(tester, rig, heardAt);
    await _hear(tester, rig, 'a', 'alpha');

    await _advanceTo(tester, rig, heardAt.add(kCurrentlySeenWindow - ms));
    expect(_tile(tester, 'Alpha')?.status, StatusKind.mesh);

    await _advanceTo(tester, rig, heardAt.add(kCurrentlySeenWindow));
    expect(_tile(tester, 'Alpha')?.status, StatusKind.offline);

    await _advanceTo(tester, rig, heardAt.add(kUnreachableListingWindow));
    expect(_tile(tester, 'Alpha')?.status, StatusKind.offline);

    await _advanceTo(tester, rig, heardAt.add(kUnreachableListingWindow + ms));
    expect(_tile(tester, 'Alpha'), isNull);

    final stored = rig.container
        .read(inventoryNotifierProvider)
        .value!
        .single;
    expect(stored.lastBaseColor, [1, 2, 3, 4],
        reason: 'hidden lamps stay paired with their data');
    expect(stored.lastSeenEpochMs, heardAt.millisecondsSinceEpoch,
        reason: 'the stored sighting is the exact last time it was heard');

    await _hear(tester, rig, 'a', 'alpha');
    expect(_tile(tester, 'Alpha')?.status, StatusKind.mesh,
        reason: 'seen again, listed again');
    await _unmount(tester, rig);
  });

  testWidgets(
      'on restart, a lamp last seen more than the listing window ago stays '
      'off the list until it is heard', (tester) async {
    final rig = await _mount(tester, lamps: [
      InventoryLamp(
        id: 'a',
        name: 'alpha',
        lastSeenEpochMs: _t0
            .subtract(kUnreachableListingWindow + ms)
            .millisecondsSinceEpoch,
      ),
      InventoryLamp(
        id: 'b',
        name: 'bravo',
        lastSeenEpochMs:
            _t0.subtract(kUnreachableListingWindow).millisecondsSinceEpoch,
      ),
    ]);
    expect(_tile(tester, 'Alpha'), isNull);
    expect(_tile(tester, 'Bravo')?.status, StatusKind.offline);

    await _hear(tester, rig, 'a', 'alpha');
    expect(_tile(tester, 'Alpha')?.status, StatusKind.mesh);
    await _unmount(tester, rig);
  });

  testWidgets('a connected lamp counts as seen however old its sighting',
      (tester) async {
    final rig = await _mount(
      tester,
      lamps: [
        InventoryLamp(
          id: 'c',
          name: 'charlie',
          lastSeenEpochMs:
              _t0.subtract(const Duration(hours: 1)).millisecondsSinceEpoch,
        ),
      ],
      before: (ble) => ble.connect('c'),
    );
    expect(_tile(tester, 'Charlie')?.status, StatusKind.mesh);
    await _unmount(tester, rig);
  });

  testWidgets(
      'time on My Lamps never connects to a lamp, the active one included',
      (tester) async {
    final rig = await _mount(
      tester,
      lamps: const [
        InventoryLamp(id: 'a', name: 'alpha', controlPassword: ''),
        InventoryLamp(id: 'b', name: 'bravo', controlPassword: ''),
      ],
      activeId: 'a',
      before: (ble) {
        seedControlBle(ble, deviceId: 'a', name: 'alpha');
        seedControlBle(ble, deviceId: 'b', name: 'bravo');
      },
    );
    for (var s = 0; s < 120; s++) {
      rig.scanner.emit(_adv('a', 'alpha'));
      rig.scanner.emit(_adv('b', 'bravo'));
      await _advanceTo(tester, rig, rig.now.add(const Duration(seconds: 1)));
    }
    expect(_tile(tester, 'Alpha')?.status, StatusKind.mesh);
    expect(rig.ble.connects, isEmpty);
    expect(rig.ble.isConnected('a') || rig.ble.isConnected('b'), isFalse);
    await _unmount(tester, rig);
  });

  testWidgets(
      'nothing on My Lamps disconnects a lamp the user opened',
      (tester) async {
    final gate = Completer<void>();
    final rig = await _mount(
      tester,
      lamps: const [
        InventoryLamp(id: 'a', name: 'alpha', controlPassword: ''),
        InventoryLamp(id: 'b', name: 'bravo', controlPassword: ''),
      ],
      activeId: 'a',
      before: (ble) {
        seedControlBle(ble, deviceId: 'a', name: 'alpha');
        ble.gate = gate;
      },
    );
    // The lamp is heard before the user opens it, so any connect started off
    // its adv is in flight, and loses, while the user's own connect lands.
    await _hear(tester, rig, 'a', 'alpha');
    final open = rig.container
        .listen(controlNotifierProvider('a'), (_, _) {});
    await tester.pump(const Duration(milliseconds: 100));
    gate.complete();
    await tester.pump(const Duration(seconds: 3));
    expect(rig.container.read(controlNotifierProvider('a')).value?.connected,
        isTrue);

    for (var s = 0; s < 120; s++) {
      rig.scanner.emit(_adv('a', 'alpha'));
      rig.scanner.emit(_adv('b', 'bravo'));
      await _advanceTo(tester, rig, rig.now.add(const Duration(seconds: 1)));
    }
    expect(rig.ble.disconnects, isEmpty);
    expect(rig.ble.isConnected('a'), isTrue);
    expect(_tile(tester, 'Alpha')?.status, StatusKind.mesh);
    open.close();
    await _unmount(tester, rig);
  });

  testWidgets('leaving My Lamps stores each heard lamp\'s exact last sighting',
      (tester) async {
    final rig = await _mount(tester, lamps: const [
      InventoryLamp(id: 'a', name: 'alpha'),
    ]);
    await _hear(tester, rig, 'a', 'alpha');
    final heardAt = rig.now.add(const Duration(seconds: 10));
    await _advanceTo(tester, rig, heardAt);
    await _hear(tester, rig, 'a', 'alpha');

    // Leave the list but keep the scope, which drives Riverpod's disposal.
    await tester.pumpWidget(UncontrolledProviderScope(
      container: rig.container,
      child: const MaterialApp(home: SizedBox()),
    ));
    await tester.pump(const Duration(seconds: 1));
    final stored = rig.container.read(inventoryNotifierProvider).value!.single;
    expect(stored.lastSeenEpochMs, heardAt.millisecondsSinceEpoch);
    await _unmount(tester, rig);
  });
}
