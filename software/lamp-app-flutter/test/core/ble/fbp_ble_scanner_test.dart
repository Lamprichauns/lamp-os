import 'dart:async';
import 'package:fake_async/fake_async.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart' as fbp;
import 'package:flutter_blue_plus_platform_interface/flutter_blue_plus_platform_interface.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/core/ble/ble_scanner.dart';
import 'package:lamp_app/core/ble/scan_pause.dart';

/// A platform whose next startScan / stopScan can be held open.
final class _Fake extends FlutterBluePlusPlatform {
  Completer<bool>? startGate;
  Completer<bool>? stopGate;
  int starts = 0;
  @override
  Future<bool> startScan(BmScanSettings request) {
    starts++;
    final g = startGate;
    startGate = null;
    return g?.future ?? Future.value(true);
  }
  @override
  Future<bool> stopScan(BmStopScanRequest request) {
    final g = stopGate;
    stopGate = null;
    return g?.future ?? Future.value(true);
  }
}

void main() {
  test('a hold ending while a restart stops its scan still leaves a scan running', () {
    fakeAsync((async) {
      final f = _Fake();
      FlutterBluePlusPlatform.instance = f;
      final s = FbpBleScanner();
      s.start();
      async.flushMicrotasks();
      expect(fbp.FlutterBluePlus.isScanningNow, isTrue);
      // An unexpected radio stop starts a restart.
      fbp.FlutterBluePlus.stopScan();
      async.flushMicrotasks();
      expect(fbp.FlutterBluePlus.isScanningNow, isFalse);
      final startGate = Completer<bool>();
      f.startGate = startGate;
      async.elapse(const Duration(seconds: 3));
      expect(f.starts, 2, reason: 'the restart is inside startScan');
      // A connect takes a hold while that startScan is in flight.
      final hold = Completer<void>();
      scanPause.during(() => hold.future);
      final stopGate = Completer<bool>();
      f.stopGate = stopGate;
      startGate.complete(true);
      async.flushMicrotasks();
      // The restart now stops its scan, and the hold ends inside that stop.
      hold.complete();
      async.flushMicrotasks();
      stopGate.complete(true);
      async.flushMicrotasks();
      expect(fbp.FlutterBluePlus.isScanningNow, isFalse);
      async.elapse(const Duration(seconds: 10));
      expect(fbp.FlutterBluePlus.isScanningNow, isTrue,
          reason: 'the hold-end trigger was not lost');
      expect(f.starts, 3);
      s.stop();
      async.flushMicrotasks();
      async.flushTimers();
    });
  });
}
