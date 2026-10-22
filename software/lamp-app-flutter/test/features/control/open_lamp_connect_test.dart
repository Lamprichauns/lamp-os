// Opening a lamp connects through the shared retry policy, and the end of
// the user's session records when the phone last saw the lamp.

import 'package:fake_async/fake_async.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../_support/in_memory_ble_client.dart';
import '../../_support/seed.dart';
import 'package:lamp_app/core/ble/ble_client_provider.dart';
import 'package:lamp_app/core/utils/clock.dart';
import 'package:lamp_app/features/control/application/control_notifier.dart';
import 'package:lamp_app/features/inventory/application/inventory_notifier.dart';
import 'package:lamp_app/features/inventory/domain/inventory_lamp.dart';

const _devId = 'lamp-open';

/// Fails the first [failures] connects, then behaves like the in-memory link.
class _FlakyConnectClient extends InMemoryBleClient {
  _FlakyConnectClient(this.failures);
  int failures;
  Duration connectTakes = Duration.zero;
  final attemptsAt = <Duration>[];
  Duration Function() elapsed = () => Duration.zero;

  @override
  Future<void> connect(String deviceId) async {
    attemptsAt.add(elapsed());
    if (connectTakes > Duration.zero) {
      await Future<void>.delayed(connectTakes);
    }
    if (failures > 0) {
      failures--;
      throw StateError('connect failed #${attemptsAt.length}');
    }
    await super.connect(deviceId);
  }
}

/// Opens [_devId] on [ble] inside fake time and returns the container.
ProviderContainer _open(FakeAsync async, InMemoryBleClient ble,
    {DateTime Function()? clock}) {
  final c = ProviderContainer(overrides: [
    bleClientProvider.overrideWithValue(ble),
    if (clock != null) clockProvider.overrideWithValue(clock),
  ]);
  c.read(inventoryNotifierProvider.future);
  async.flushMicrotasks();
  c.read(inventoryNotifierProvider.notifier).add(const InventoryLamp(
        id: _devId,
        name: 'test-lamp',
        controlPassword: '',
      ));
  async.flushMicrotasks();
  return c;
}

void main() {
  setUp(() => SharedPreferences.setMockInitialValues({}));

  test('a connect that fails three times then succeeds opens the lamp', () {
    fakeAsync((async) {
      final ble = _FlakyConnectClient(3)..elapsed = () => async.elapsed;
      seedControlBle(ble, deviceId: _devId);
      final c = _open(async, ble);
      final sub = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 10));

      final state = c.read(controlNotifierProvider(_devId));
      expect(state.hasError, isFalse, reason: '${state.error}');
      expect(state.value?.connected, isTrue);
      expect(ble.attemptsAt, hasLength(4));
      for (var i = 1; i < ble.attemptsAt.length; i++) {
        expect(ble.attemptsAt[i] - ble.attemptsAt[i - 1],
            greaterThanOrEqualTo(const Duration(milliseconds: 1000)),
            reason: 'attempts back off instead of hammering the stack');
      }
      sub.close();
      c.dispose();
      async.flushTimers();
    });
  });

  test('a lamp that never answers fails with the last error, bounded', () {
    fakeAsync((async) {
      final ble = _FlakyConnectClient(1 << 30)..elapsed = () => async.elapsed;
      seedControlBle(ble, deviceId: _devId);
      final c = _open(async, ble);
      final sub = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 30));
      final attempts = ble.attemptsAt.length;
      async.elapse(const Duration(minutes: 5));

      final state = c.read(controlNotifierProvider(_devId));
      expect(state.hasError, isTrue);
      expect(state.error, isA<StateError>());
      expect('${state.error}', contains('#$attempts'),
          reason: 'the user sees the final attempt\'s failure');
      expect(attempts, greaterThan(2));
      expect(attempts, lessThanOrEqualTo(4));
      expect(ble.attemptsAt, hasLength(attempts),
          reason: 'no attempts after the policy gives up');
      expect(ble.isConnected(_devId), isFalse);
      sub.close();
      c.dispose();
      async.flushTimers();
    });
  });

  test('closing the lamp records the moment the phone last saw it', () {
    fakeAsync((async) {
      var now = DateTime.fromMillisecondsSinceEpoch(1700000000000);
      final ble = InMemoryBleClient();
      seedControlBle(ble, deviceId: _devId);
      final c = _open(async, ble, clock: () => now);
      final sub = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 1));
      expect(c.read(controlNotifierProvider(_devId)).value?.connected, isTrue);

      now = now.add(const Duration(minutes: 20));
      sub.close();
      async.elapse(const Duration(seconds: 1));

      final lamp = c.read(inventoryNotifierProvider).value!.single;
      expect(lamp.lastSeenEpochMs, now.millisecondsSinceEpoch);
      c.dispose();
      async.flushTimers();
    });
  });

  test('a dropped link records the moment the phone last saw the lamp', () {
    fakeAsync((async) {
      var now = DateTime.fromMillisecondsSinceEpoch(1700000000000);
      final ble = InMemoryBleClient();
      seedControlBle(ble, deviceId: _devId);
      final c = _open(async, ble, clock: () => now);
      final sub = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 1));

      now = now.add(const Duration(minutes: 20));
      ble.disconnect(_devId);
      async.flushMicrotasks();

      final lamp = c.read(inventoryNotifierProvider).value!.single;
      expect(lamp.lastSeenEpochMs, now.millisecondsSinceEpoch);
      sub.close();
      c.dispose();
      async.flushTimers();
    });
  });

  test('closing a lamp whose link already dropped keeps the drop time', () {
    fakeAsync((async) {
      var now = DateTime.fromMillisecondsSinceEpoch(1700000000000);
      final ble = _FlakyConnectClient(0);
      seedControlBle(ble, deviceId: _devId);
      final c = _open(async, ble, clock: () => now);
      final sub = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 1));

      final droppedAt = now.add(const Duration(minutes: 1));
      now = droppedAt;
      ble.failures = 1 << 30;
      ble.disconnect(_devId);
      async.flushMicrotasks();
      now = now.add(const Duration(minutes: 10));
      sub.close();
      async.elapse(const Duration(seconds: 1));

      final lamp = c.read(inventoryNotifierProvider).value!.single;
      expect(lamp.lastSeenEpochMs, droppedAt.millisecondsSinceEpoch);
      c.dispose();
      async.flushTimers();
    });
  });

  test('leaving while the open is still connecting leaves no link behind', () {
    fakeAsync((async) {
      final ble = _FlakyConnectClient(1)
        ..elapsed = (() => async.elapsed)
        ..connectTakes = const Duration(seconds: 8);
      seedControlBle(ble, deviceId: _devId);
      final c = _open(async, ble);
      final sub = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 3));
      sub.close();
      async.elapse(const Duration(minutes: 1));

      expect(ble.isConnected(_devId), isFalse);
      c.dispose();
      async.flushTimers();
    });
  });

  test('leaving while a reconnect is connecting leaves no link behind', () {
    fakeAsync((async) {
      final ble = _FlakyConnectClient(0)..elapsed = () => async.elapsed;
      seedControlBle(ble, deviceId: _devId);
      final c = _open(async, ble);
      final sub = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 1));
      expect(c.read(controlNotifierProvider(_devId)).value?.connected, isTrue);

      ble.connectTakes = const Duration(seconds: 8);
      ble.disconnect(_devId);
      async.elapse(const Duration(seconds: 2));
      expect(ble.attemptsAt.length, greaterThan(1),
          reason: 'the reconnect ladder is mid-connect');
      sub.close();
      async.elapse(const Duration(minutes: 1));

      expect(ble.isConnected(_devId), isFalse);
      c.dispose();
      async.flushTimers();
    });
  });
}
