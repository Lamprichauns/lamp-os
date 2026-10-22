// Opening a lamp connects through the shared retry policy, and the end of
// the user's session records when the phone last saw the lamp.

import 'dart:async';
import 'dart:typed_data';

import 'package:fake_async/fake_async.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../_support/in_memory_ble_client.dart';
import '../../_support/seed.dart';
import 'package:lamp_app/core/ble/ble_client_provider.dart';
import 'package:lamp_app/core/lifecycle/app_lifecycle.dart';
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

/// A first-come lock whose place is taken when [take] is called.
class _FifoLock {
  Future<void> _tail = Future.value();

  Future<void Function()> take() {
    final prior = _tail;
    final done = Completer<void>();
    _tail = done.future;
    return prior.then((_) => done.complete);
  }
}

/// flutter_blue_plus 2.3.4's per-device ordering (bluetooth_device.dart):
/// connect and disconnect share a disconnect lock, every GATT op shares an op
/// lock, both first-come. A connect to a linked lamp returns at once. On
/// Android a disconnect waits until [androidDelay] after the latest connect
/// request, and the link stays up until it lands. [settleTakes] holds the op
/// lock after a link lands, as the MTU request does.
class _FbpModelClient extends InMemoryBleClient {
  _FbpModelClient(this.elapsed);
  final Duration Function() elapsed;
  Duration connectTakes = const Duration(seconds: 1);
  Duration settleTakes = Duration.zero;
  Duration opTakes = const Duration(milliseconds: 50);
  Duration androidDelay = const Duration(seconds: 2);
  int disconnects = 0;
  Duration? _connectRequestedAt;
  final _disconnectLock = _FifoLock();
  final _opLock = _FifoLock();

  @override
  Future<void> connect(String deviceId) async {
    final giveDisconnect = await _disconnectLock.take();
    final giveOp = await _opLock.take();
    _connectRequestedAt = elapsed();
    giveDisconnect();
    try {
      if (isConnected(deviceId)) return;
      await Future<void>.delayed(connectTakes);
      await super.connect(deviceId);
    } finally {
      giveOp();
    }
    if (settleTakes > Duration.zero) {
      await _op(() => Future<void>.delayed(settleTakes));
    }
  }

  @override
  Future<void> disconnect(String deviceId) async {
    disconnects++;
    final giveDisconnect = await _disconnectLock.take();
    final giveOp = await _opLock.take();
    try {
      final requestedAt = _connectRequestedAt;
      if (requestedAt != null) {
        final left = requestedAt + androidDelay - elapsed();
        if (left > Duration.zero) await Future<void>.delayed(left);
      }
      await super.disconnect(deviceId);
      _connectRequestedAt = null;
    } finally {
      giveDisconnect();
      giveOp();
    }
  }

  Future<T> _op<T>(Future<T> Function() body) async {
    final give = await _opLock.take();
    try {
      await Future<void>.delayed(opTakes);
      return await body();
    } finally {
      give();
    }
  }

  @override
  Future<Uint8List> read(String d, String s, String c) =>
      _op(() => super.read(d, s, c));

  @override
  Future<Uint8List> readSection(String deviceId, String name) =>
      _op(() => super.readSection(deviceId, name));

  @override
  Future<void> write(
    String d,
    String s,
    String c,
    Uint8List v, {
    bool withoutResponse = false,
    bool allowLongWrite = false,
  }) =>
      _op(() => super.write(d, s, c, v,
          withoutResponse: withoutResponse, allowLongWrite: allowLongWrite));
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

  test('re-opening a lamp mid-connect keeps the new session connected', () {
    fakeAsync((async) {
      final ble = _FbpModelClient(() => async.elapsed)
        ..connectTakes = const Duration(seconds: 8);
      seedControlBle(ble, deviceId: _devId);
      final c = _open(async, ble);
      final first = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 3));
      first.close();
      async.elapse(const Duration(seconds: 1));
      expect(c.exists(controlNotifierProvider(_devId)), isFalse);

      final second = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 20));

      final state = c.read(controlNotifierProvider(_devId));
      expect(state.hasError, isFalse, reason: '${state.error}');
      expect(state.value?.connected, isTrue);
      expect(ble.disconnects, 0, reason: 'the re-opened link never dropped');
      second.close();
      c.dispose();
      async.flushTimers();
    });
  });

  test('re-opening onto a link still settling keeps the new session', () {
    fakeAsync((async) {
      final ble = _FbpModelClient(() => async.elapsed)
        ..connectTakes = const Duration(seconds: 8)
        ..settleTakes = const Duration(seconds: 2);
      seedControlBle(ble, deviceId: _devId);
      final c = _open(async, ble);
      final first = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 3));
      first.close();
      async.elapse(const Duration(seconds: 6));
      expect(ble.isConnected(_devId), isTrue,
          reason: 'the abandoned connect has linked but not returned');

      final second = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 20));

      final state = c.read(controlNotifierProvider(_devId));
      expect(state.hasError, isFalse, reason: '${state.error}');
      expect(state.value?.connected, isTrue);
      expect(ble.disconnects, 0, reason: 'the re-opened link never dropped');
      second.close();
      c.dispose();
      async.flushTimers();
    });
  });

  test('a mid-connect open nobody re-opens drops its link once it lands', () {
    fakeAsync((async) {
      final ble = _FbpModelClient(() => async.elapsed)
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

  test('re-opening while an abandoned link is being dropped reconnects', () {
    fakeAsync((async) {
      final ble = _FbpModelClient(() => async.elapsed);
      seedControlBle(ble, deviceId: _devId);
      final c = _open(async, ble);
      final opened = async.elapsed;
      final first = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(milliseconds: 500));
      first.close();
      async.elapse(const Duration(milliseconds: 800));
      expect(c.exists(controlNotifierProvider(_devId)), isFalse);
      expect(ble.disconnects, 1, reason: 'the abandoned link is being dropped');
      expect(ble.isConnected(_devId), isTrue,
          reason: 'inside the Android disconnect hold');
      expect(async.elapsed - opened, lessThan(ble.androidDelay));

      final second = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 20));

      final state = c.read(controlNotifierProvider(_devId));
      expect(state.hasError, isFalse, reason: '${state.error}');
      expect(state.value?.connected, isTrue);
      expect(ble.isConnected(_devId), isTrue);
      second.close();
      c.dispose();
      async.flushTimers();
    });
  });

  test('re-opening while a loading session drops its link reconnects', () {
    fakeAsync((async) {
      final ble = _FbpModelClient(() => async.elapsed)
        ..connectTakes = const Duration(milliseconds: 300);
      seedControlBle(ble, deviceId: _devId);
      final c = _open(async, ble);
      final opened = async.elapsed;
      final first = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(milliseconds: 500));
      expect(ble.isConnected(_devId), isTrue);
      expect(c.read(controlNotifierProvider(_devId)).isLoading, isTrue,
          reason: 'linked, still reading the lamp');
      first.close();
      async.elapse(const Duration(milliseconds: 300));
      expect(c.exists(controlNotifierProvider(_devId)), isFalse);
      expect(ble.disconnects, 1, reason: 'the old session drops its link');
      expect(ble.isConnected(_devId), isTrue,
          reason: 'inside the Android disconnect hold');
      expect(async.elapsed - opened, lessThan(ble.androidDelay));

      final second = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 20));

      final state = c.read(controlNotifierProvider(_devId));
      expect(state.hasError, isFalse, reason: '${state.error}');
      expect(state.value?.connected, isTrue);
      expect(ble.isConnected(_devId), isTrue);
      second.close();
      c.dispose();
      async.flushTimers();
    });
  });

  test('leaving while a save waits on the reboot leaves no link behind', () {
    fakeAsync((async) {
      final ble = _FlakyConnectClient(0)..elapsed = () => async.elapsed;
      seedControlBle(ble, deviceId: _devId);
      final c = _open(async, ble);
      final sub = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 1));
      expect(c.read(controlNotifierProvider(_devId)).value?.connected, isTrue);

      ble.connectTakes = const Duration(seconds: 8);
      final attemptsBefore = ble.attemptsAt.length;
      c.read(controlNotifierProvider(_devId).notifier).setLampPassword('pw');
      async.elapse(const Duration(seconds: 3));
      expect(ble.attemptsAt.length, greaterThan(attemptsBefore),
          reason: 'the post-save reconnect is mid-connect');
      sub.close();
      async.elapse(const Duration(minutes: 1));

      expect(ble.isConnected(_devId), isFalse);
      c.dispose();
      async.flushTimers();
    });
  });

  test('backgrounding the app records an open lamp as seen', () {
    fakeAsync((async) {
      var now = DateTime.fromMillisecondsSinceEpoch(1700000000000);
      final ble = InMemoryBleClient();
      seedControlBle(ble, deviceId: _devId);
      final c = _open(async, ble, clock: () => now);
      final sub = c.listen(controlNotifierProvider(_devId), (_, _) {});
      async.elapse(const Duration(seconds: 1));

      now = now.add(const Duration(minutes: 10));
      c
          .read(appLifecycleStateProvider.notifier)
          .set(AppLifecycleState.paused);
      async.flushMicrotasks();

      final lamp = c.read(inventoryNotifierProvider).value!.single;
      expect(lamp.lastSeenEpochMs, now.millisecondsSinceEpoch);
      sub.close();
      c.dispose();
      async.flushTimers();
    });
  });

  test('backgrounding after the link dropped keeps the drop time', () {
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
      c
          .read(appLifecycleStateProvider.notifier)
          .set(AppLifecycleState.paused);
      async.flushMicrotasks();

      final lamp = c.read(inventoryNotifierProvider).value!.single;
      expect(lamp.lastSeenEpochMs, droppedAt.millisecondsSinceEpoch);
      sub.close();
      c.dispose();
      async.flushTimers();
    });
  });
}
