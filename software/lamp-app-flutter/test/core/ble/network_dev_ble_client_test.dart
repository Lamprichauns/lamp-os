import 'dart:async';
import 'dart:convert';
import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/core/ble/bridge_transport.dart';
import 'package:lamp_app/core/ble/ble_client.dart';
import 'package:lamp_app/core/ble/network_dev_ble_client.dart';
import '../../_support/fake_bridge_transport.dart';

/// Holds the bridge's answer to a disconnect until [answerDisconnect].
class _SlowDisconnectTransport extends FakeBridgeTransport {
  final _disconnectAnswered = Completer<void>();
  void answerDisconnect() => _disconnectAnswered.complete();

  @override
  Future<BridgeResponse> request(String method, String path,
      {Object? body}) async {
    calls.add('$method $path');
    if (path.startsWith('/disconnect/')) await _disconnectAnswered.future;
    return const BridgeResponse(200, {});
  }
}

void main() {
  test('a connect issued during a pending disconnect runs after it', () async {
    final t = _SlowDisconnectTransport();
    final c = NetworkDevBleClient(t);
    await c.connect('dev');
    final dropped = c.disconnect('dev');
    final reopened = c.connect('dev');
    await Future<void>.delayed(Duration.zero);
    expect(t.calls, ['POST /connect/dev', 'POST /disconnect/dev'],
        reason: 'the connect waits for the disconnect');
    t.answerDisconnect();
    await dropped;
    await reopened;
    expect(t.calls.last, 'POST /connect/dev');
    expect(c.isConnected('dev'), isTrue);
  });

  test('connect marks device connected; isConnected reflects it', () async {
    final t = FakeBridgeTransport();
    final c = NetworkDevBleClient(t);
    await c.connect('AA:BB:CC:DD:EE:FF');
    expect(c.isConnected('AA:BB:CC:DD:EE:FF'), isTrue);
    expect(t.calls, contains('POST /connect/AA:BB:CC:DD:EE:FF'));
  });

  test('read decodes base64 payload from the bridge', () async {
    final t = FakeBridgeTransport();
    t.responses['GET /read/dev/svc/chr'] =
        BridgeResponse(200, {'data': base64Encode(utf8.encode('hi'))});
    final c = NetworkDevBleClient(t);
    final bytes = await c.read('dev', 'svc', 'chr');
    expect(utf8.decode(bytes), 'hi');
  });

  test('non-2xx maps the wire error code to a typed exception', () async {
    final t = FakeBridgeTransport();
    t.responses['GET /read/dev/svc/chr'] =
        const BridgeResponse(500, {'error': 'ENCRYPTION_REQUIRED'});
    final c = NetworkDevBleClient(t);
    expect(() => c.read('dev', 'svc', 'chr'), throwsA(isA<BleEncryptionRequired>()));
  });
}
