import 'package:fake_async/fake_async.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/core/ble/connect_with_retry.dart';

import '../../_support/in_memory_ble_client.dart';

class _CountingClient extends InMemoryBleClient {
  _CountingClient(this.failures);
  int failures;
  int connects = 0;
  int disconnects = 0;

  /// A failing attempt still raises the link, as an Android connect whose
  /// MTU request fails does.
  bool failsAfterLinking = false;

  @override
  Future<void> connect(String deviceId) async {
    connects++;
    if (failures > 0) {
      failures--;
      if (failsAfterLinking) await super.connect(deviceId);
      throw StateError('attempt $connects');
    }
    await super.connect(deviceId);
  }

  @override
  Future<void> disconnect(String deviceId) {
    disconnects++;
    return super.disconnect(deviceId);
  }
}

void main() {
  test('retries with backoff until a connect lands', () {
    fakeAsync((async) {
      final ble = _CountingClient(2);
      bool? result;
      connectWithRetry(ble, 'x', owner: Object(), attempts: 4)
          .then((r) => result = r);
      async.elapse(const Duration(milliseconds: 1199));
      expect(ble.connects, 1, reason: 'waits out the backoff');
      async.elapse(const Duration(seconds: 5));
      expect(result, isTrue);
      expect(ble.connects, 3);
      expect(ble.isConnected('x'), isTrue);
    });
  });

  test('stops at the attempt bound and rethrows the last failure', () {
    fakeAsync((async) {
      final ble = _CountingClient(1 << 30);
      Object? error;
      connectWithRetry(ble, 'x', owner: Object(), attempts: 3)
          .catchError((Object e) {
        error = e;
        return false;
      });
      async.elapse(const Duration(minutes: 1));
      expect(ble.connects, 3);
      expect('$error', contains('attempt 3'));
    });
  });

  test('an abort between attempts stops without connecting', () {
    fakeAsync((async) {
      final ble = _CountingClient(1);
      var abort = false;
      bool? result;
      connectWithRetry(ble, 'x', owner: Object(), shouldAbort: () => abort)
          .then((r) => result = r);
      async.flushMicrotasks();
      abort = true;
      async.elapse(const Duration(seconds: 10));
      expect(result, isFalse);
      expect(ble.connects, 1);
      expect(ble.isConnected('x'), isFalse);
    });
  });

  test('a link that lands after the abort is dropped', () {
    fakeAsync((async) {
      final ble = _CountingClient(0);
      var abort = false;
      bool? result;
      // The abort turns true while the attempt is in flight.
      final pending =
          connectWithRetry(ble, 'x', owner: Object(), shouldAbort: () {
        final now = abort;
        abort = true;
        return now;
      });
      pending.then((r) => result = r);
      async.flushMicrotasks();
      expect(result, isFalse);
      expect(ble.disconnects, 1);
      expect(ble.isConnected('x'), isFalse);
    });
  });

  test('a late link stays up when a later connect has claimed the lamp', () {
    fakeAsync((async) {
      final ble = _CountingClient(0);
      var abort = false;
      // The user leaves while the attempt is in flight and re-opens the lamp.
      connectWithRetry(ble, 'x', owner: Object(), shouldAbort: () {
        if (!abort) {
          abort = true;
          return false;
        }
        ble.claimLink('x', Object());
        return true;
      });
      async.flushMicrotasks();
      expect(ble.disconnects, 0);
      expect(ble.isConnected('x'), isTrue);
    });
  });

  test('a failure on every attempt leaves no link behind', () {
    fakeAsync((async) {
      final ble = _CountingClient(1 << 30)..failsAfterLinking = true;
      Object? error;
      connectWithRetry(ble, 'x', owner: Object(), attempts: 3)
          .catchError((Object e) {
        error = e;
        return false;
      });
      async.elapse(const Duration(minutes: 1));
      expect('$error', contains('attempt 3'));
      expect(ble.isConnected('x'), isFalse);
    });
  });

  test('an abort after a failed attempt leaves no link behind', () {
    fakeAsync((async) {
      final ble = _CountingClient(1)..failsAfterLinking = true;
      var abort = false;
      bool? result;
      connectWithRetry(ble, 'x', owner: Object(), shouldAbort: () => abort)
          .then((r) => result = r);
      async.flushMicrotasks();
      expect(ble.isConnected('x'), isTrue);
      abort = true;
      async.elapse(const Duration(seconds: 10));
      expect(result, isFalse);
      expect(ble.connects, 1);
      expect(ble.isConnected('x'), isFalse);
    });
  });

  test('a failed connect keeps a link a later connect has claimed', () {
    fakeAsync((async) {
      final ble = _CountingClient(1 << 30)..failsAfterLinking = true;
      connectWithRetry(ble, 'x', owner: Object(), attempts: 2)
          .catchError((Object e) => false);
      async.flushMicrotasks();
      ble.claimLink('x', Object());
      async.elapse(const Duration(minutes: 1));
      expect(ble.disconnects, 0);
      expect(ble.isConnected('x'), isTrue);
    });
  });

  test('claims are per lamp', () {
    final ble = _CountingClient(0);
    final a = Object();
    ble.claimLink('x', a);
    ble.claimLink('y', Object());
    expect(ble.ownsLink('x', a), isTrue);
    expect(ble.ownsLink('y', a), isFalse);
  });
}
