import 'dart:async';

/// Holds BLE scanning off while a connect is in flight. Neither
/// flutter_blue_plus platform plugin pauses its own scan around a connect, and
/// a running scan competes with the connect handshake for the phone's radio.
///
/// A scanner must not start a scan while [isPaused], and should start one
/// again on [released] if it still wants to scan.
class ScanPause {
  int _holds = 0;
  final _released = StreamController<void>.broadcast(sync: true);

  bool get isPaused => _holds > 0;

  /// Fires when the last hold ends, whether its body completed or threw.
  Stream<void> get released => _released.stream;

  /// Runs [body] with scanning held off. Holds nest: scanning resumes only
  /// when every concurrent hold has ended.
  Future<T> during<T>(Future<T> Function() body) async {
    _holds++;
    try {
      return await body();
    } finally {
      _holds--;
      if (_holds == 0) _released.add(null);
    }
  }
}

/// The phone has one BLE scan, so there is one pause shared by every scanner
/// and client.
final scanPause = ScanPause();
