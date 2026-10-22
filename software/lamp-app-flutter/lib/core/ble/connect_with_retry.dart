import 'dart:async';

import 'ble_client.dart';

/// The app's connect retry policy, shared by opening a lamp from My Lamps and
/// by onboarding. Android throws GATT_ERROR(133) / a deviceIsDisconnected race
/// intermittently on a first connect in a crowded BLE environment, and the
/// stack needs about a second to release the slot after a failed attempt.
/// Makes at most [attempts] tries and rethrows the last failure so the caller
/// can map it to a user-visible error.
///
/// Returns true once connected. Returns false when [shouldAbort] turns true,
/// checked before each attempt and after a successful one; a link that lands
/// after its owner gave up is dropped so nothing holds the lamp.
///
/// The post-reboot reconnect loop in AddLampNotifier is a different policy. It
/// disconnects a stale handle, waits out the firmware reboot, and wraps a
/// per-attempt timeout.
Future<bool> connectWithRetry(
  BleClient ble,
  String deviceId, {
  int attempts = 4,
  Duration backoff = const Duration(milliseconds: 1200),
  bool Function()? shouldAbort,
}) async {
  for (var i = 0; i < attempts; i++) {
    if (shouldAbort?.call() ?? false) return false;
    try {
      await ble.connect(deviceId);
    } catch (_) {
      if (i == attempts - 1) rethrow;
      await Future<void>.delayed(backoff);
      continue;
    }
    if (shouldAbort?.call() ?? false) {
      await ble.disconnect(deviceId);
      return false;
    }
    return true;
  }
  return false;
}
