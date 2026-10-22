import 'dart:async';

import 'ble_client.dart';

/// The app's connect retry policy, shared by opening a lamp from My Lamps and
/// by onboarding. Android throws GATT_ERROR(133) / a deviceIsDisconnected race
/// intermittently on a first connect in a crowded BLE environment, and the
/// stack needs about a second to release the slot after a failed attempt.
/// Makes at most [attempts] tries and rethrows the last failure so the caller
/// can map it to a user-visible error. [shouldAbort] lets a caller bail
/// between attempts (the pulse controller's stop()).
///
/// The post-reboot reconnect loop in AddLampNotifier is a different policy:
/// it disconnects a stale handle, waits out the firmware reboot, and wraps a
/// per-attempt timeout.
Future<void> connectWithRetry(
  BleClient ble,
  String deviceId, {
  int attempts = 4,
  Duration backoff = const Duration(milliseconds: 1200),
  bool Function()? shouldAbort,
}) async {
  for (var i = 0; i < attempts; i++) {
    if (shouldAbort?.call() ?? false) return;
    try {
      await ble.connect(deviceId);
      return;
    } catch (_) {
      if (i == attempts - 1) rethrow;
      await Future<void>.delayed(backoff);
    }
  }
}
