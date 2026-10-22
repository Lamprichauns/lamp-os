import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/features/inventory/domain/lamp_presence.dart';

import 'package:lamp_app/core/widgets/status_dot.dart';
import 'package:lamp_app/features/lamp_shell/application/lamp_status.dart';
import 'package:lamp_app/features/nearby/domain/nearby_lamp.dart';

NearbyLamp _seenLamp(String id) => NearbyLamp(
      id: id,
      name: 'x',
      rssi: -50,
      serviceUuids: const [],
      baseRgb: 0,
      shadeRgb: 0,
      lastSeenEpochMs: 1,
    );

void main() {
  group('statusFor', () {
    test('connected → mesh, even if not in nearby list', () {
      expect(
        statusFor(lampId: 'a', nearby: const [], connected: true),
        StatusKind.mesh,
      );
    });

    test('in nearby + not connected → bluetooth', () {
      expect(
        statusFor(
          lampId: 'a',
          nearby: [_seenLamp('a')],
          connected: false,
        ),
        StatusKind.bluetooth,
      );
    });

    test('absent from nearby + not connected → offline', () {
      expect(
        statusFor(
          lampId: 'a',
          nearby: [_seenLamp('b')],
          connected: false,
        ),
        StatusKind.offline,
      );
    });

    test('receiving → otaReceiving, winning over the held connection', () {
      expect(
        statusFor(
          lampId: 'a',
          nearby: const [],
          connected: true,
          receiving: true,
        ),
        StatusKind.otaReceiving,
      );
    });

    test('sending → otaSending, winning over the held connection', () {
      expect(
        statusFor(
          lampId: 'a',
          nearby: const [],
          connected: true,
          sending: true,
        ),
        StatusKind.otaSending,
      );
    });

    test('not updating leaves the mapping unchanged', () {
      expect(
        statusFor(lampId: 'a', nearby: const [], connected: true),
        StatusKind.mesh,
      );
    });

    test('nearby lamp advertising OTA-distributing does not read as busy', () {
      final busy = _seenLamp('a').copyWith(otaDistributing: true);
      expect(
        statusFor(lampId: 'a', nearby: [busy], connected: false),
        StatusKind.bluetooth,
      );
    });
  });

  group('statusForPresence', () {
    test('receiving → otaReceiving', () {
      expect(
        statusForPresence(
          presence: LampPresence.reachable,
          connected: true,
          isMesh: true,
          receiving: true,
        ),
        StatusKind.otaReceiving,
      );
    });

    test('sending → otaSending', () {
      expect(
        statusForPresence(
          presence: LampPresence.reachable,
          connected: true,
          isMesh: true,
          sending: true,
        ),
        StatusKind.otaSending,
      );
    });

    test('connected → mesh even for a lamp last known BT-only', () {
      expect(
        statusForPresence(
          presence: LampPresence.unreachable,
          connected: true,
          isMesh: false,
        ),
        StatusKind.mesh,
      );
    });

    test('reachable follows the mesh capability', () {
      expect(
        statusForPresence(
            presence: LampPresence.reachable, connected: false, isMesh: true),
        StatusKind.mesh,
      );
      expect(
        statusForPresence(
            presence: LampPresence.reachable, connected: false, isMesh: false),
        StatusKind.bluetooth,
      );
    });

    test('unreachable is offline, or searching inside scan grace', () {
      expect(
        statusForPresence(
            presence: LampPresence.unreachable, connected: false, isMesh: true),
        StatusKind.offline,
      );
      expect(
        statusForPresence(
          presence: LampPresence.unreachable,
          connected: false,
          isMesh: true,
          inScanGrace: true,
        ),
        StatusKind.searching,
      );
    });
  });
}
