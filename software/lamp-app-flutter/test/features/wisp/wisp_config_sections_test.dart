import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/core/ble/ble_client_provider.dart';
import 'package:lamp_app/core/ble/uuids.dart';
import 'package:lamp_app/features/wisp/application/wisp_notifier.dart';
import 'package:lamp_app/features/wisp/data/wisp_repository.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../_support/in_memory_ble_client.dart';

const _lampId = 'test-lamp';
const _mac = 'AA:BB:CC:DD:EE:FF';

Uint8List _b(String s) => Uint8List.fromList(utf8.encode(s));

String _config({int px = 60, int brightness = 80, int opSeq = 0}) =>
    '{"char":"wispConfig","ledType":"GRBW","px":$px,"brightness":$brightness,'
    '"driftIntervalMs":90000,"driftFadePct":40,"shuffleSeed":7,"opSeq":$opSeq}';

/// Section-vs-fallback selection and the config merge. A lamp that serves
/// `wispstatus` answers it with at least `"{}"`; the fake's unseeded section
/// returns 0 bytes, the same as a lamp that predates it.
void main() {
  TestWidgetsFlutterBinding.ensureInitialized();
  setUp(() => SharedPreferences.setMockInitialValues({}));

  Future<InMemoryBleClient> connectedBle({String? directStatus}) async {
    final ble = InMemoryBleClient();
    await ble.connect(_lampId);
    if (directStatus != null) {
      await ble.write(
        _lampId,
        BleUuids.controlService,
        BleUuids.wispStatus,
        _b(directStatus),
      );
    }
    return ble;
  }

  ProviderContainer makeContainer(InMemoryBleClient ble) {
    final c = ProviderContainer(
      overrides: [bleClientProvider.overrideWithValue(ble)],
    );
    addTearDown(c.dispose);
    return c;
  }

  Future<void> settle() => Future<void>.delayed(Duration.zero);

  // Build, then let build's trailing claims read finish so a re-read the
  // test triggers is not skipped as busy.
  Future<void> built(ProviderContainer c) async {
    await c.read(wispNotifierProvider(_lampId).future);
    await settle();
  }

  group('WispRepository section selection', () {
    test('reads the wispstatus section when the lamp serves it', () async {
      final ble = await connectedBle(
        directStatus: '{"wispMac":"$_mac","name":"direct"}',
      );
      ble.seedSection(
        _lampId,
        'wispstatus',
        _b('{"wispMac":"$_mac","name":"section"}'),
      );
      final repo = WispRepository(ble, _lampId);

      final s = await repo.readStatus();

      expect(s.name, 'section');
      expect(repo.wispSectionsSupported, isTrue);
    });

    test('falls back to CHAR_WISP_STATUS on a lamp without the section',
        () async {
      final ble = await connectedBle(
        directStatus: '{"wispMac":"$_mac","name":"direct"}',
      );
      final repo = WispRepository(ble, _lampId);

      final s = await repo.readStatus();

      expect(s.name, 'direct');
      expect(repo.wispSectionsSupported, isFalse);
      expect(await repo.readConfig(), isNull);
    });

    test('"{}" from wispconfig (none cached yet) reads as no config',
        () async {
      final ble = await connectedBle();
      ble.seedSection(_lampId, 'wispstatus', _b('{}'));
      ble.seedSection(_lampId, 'wispconfig', _b('{}'));
      final repo = WispRepository(ble, _lampId);

      await repo.readStatus();

      expect(repo.wispSectionsSupported, isTrue);
      expect(await repo.readConfig(), isNull);
    });

    test('pollConfig writes a plaintext pollConfig wispOp even with a password',
        () async {
      final ble = await connectedBle();
      final repo = WispRepository(ble, _lampId)..updatePassword('secret');

      await repo.pollConfig();

      final written = ble.writesTo(_lampId, BleUuids.wispOp).single;
      final decoded = jsonDecode(utf8.decode(written)) as Map<String, dynamic>;
      expect(decoded['char'], 'wispOp');
      expect(decoded['op'], 'pollConfig');
    });
  });

  group('WispNotifier config merge', () {
    test('build merges the wispconfig section over the status', () async {
      final ble = await connectedBle();
      ble.seedSection(
        _lampId,
        'wispstatus',
        _b('{"wispMac":"$_mac","configSplit":true,"source":"osc",'
            '"configLastSeenMs":100}'),
      );
      ble.seedSection(_lampId, 'wispconfig', _b(_config()));
      final c = makeContainer(ble);

      final s = await c.read(wispNotifierProvider(_lampId).future);

      expect(s.configKnown, isTrue);
      expect(s.pixelCount, 60);
      expect(s.brightness, 80);
      expect(s.ledType, 'GRBW');
    });

    // The status says its config lives elsewhere and the lamp has none yet:
    // the forms must see "unknown", not the parser's defaults.
    test('split status with no config cached stays unknown', () async {
      final ble = await connectedBle();
      ble.seedSection(
        _lampId,
        'wispstatus',
        _b('{"wispMac":"$_mac","configSplit":true,"source":"osc"}'),
      );
      ble.seedSection(_lampId, 'wispconfig', _b('{}'));
      final c = makeContainer(ble);

      final s = await c.read(wispNotifierProvider(_lampId).future);

      expect(s.present, isTrue);
      expect(s.configKnown, isFalse);
    });

    test('a new configLastSeenMs on a notify re-reads the section', () async {
      final ble = await connectedBle();
      ble.seedSection(
        _lampId,
        'wispstatus',
        _b('{"wispMac":"$_mac","source":"osc","configLastSeenMs":100}'),
      );
      ble.seedSection(_lampId, 'wispconfig', _b(_config(brightness: 80)));
      final c = makeContainer(ble);
      await built(c);

      ble.seedSection(_lampId, 'wispconfig', _b(_config(brightness: 25)));
      ble.simulateNotify(
        _lampId,
        BleUuids.controlService,
        BleUuids.wispStatus,
        _b('{"wispMac":"$_mac","source":"osc","configLastSeenMs":200}'),
      );
      await settle();
      await settle();

      expect(c.read(wispNotifierProvider(_lampId)).value!.brightness, 25);
    });

    test('the held config wins over the status copy of the same wisp',
        () async {
      final ble = await connectedBle();
      ble.seedSection(
        _lampId,
        'wispstatus',
        _b('{"wispMac":"$_mac","source":"osc","configLastSeenMs":100}'),
      );
      ble.seedSection(_lampId, 'wispconfig', _b(_config(px: 60)));
      final c = makeContainer(ble);
      await built(c);

      ble.simulateNotify(
        _lampId,
        BleUuids.controlService,
        BleUuids.wispStatus,
        _b('{"wispMac":"$_mac","source":"osc","px":30,'
            '"configLastSeenMs":100}'),
      );
      await settle();

      expect(c.read(wispNotifierProvider(_lampId)).value!.pixelCount, 60);
    });

    // A rival wisp took the slot: the old wisp's config must not dress it.
    test('a held config never merges onto another wisp', () async {
      final ble = await connectedBle();
      ble.seedSection(
        _lampId,
        'wispstatus',
        _b('{"wispMac":"$_mac","source":"osc","configLastSeenMs":100}'),
      );
      ble.seedSection(_lampId, 'wispconfig', _b(_config(px: 60)));
      final c = makeContainer(ble);
      await built(c);

      ble.seedSection(_lampId, 'wispconfig', _b('{}'));
      ble.simulateNotify(
        _lampId,
        BleUuids.controlService,
        BleUuids.wispStatus,
        _b('{"wispMac":"11:22:33:44:55:66","configSplit":true,'
            '"source":"osc"}'),
      );
      await settle();

      final s = c.read(wispNotifierProvider(_lampId)).value!;
      expect(s.wispMac, '11:22:33:44:55:66');
      expect(s.configKnown, isFalse);
    });

    // An empty or MTU-truncated notify on a lamp with the section: the
    // update is recovered from the section instead of dropped.
    test('an unparseable notify re-reads the wispstatus section', () async {
      final ble = await connectedBle();
      ble.seedSection(
        _lampId,
        'wispstatus',
        _b('{"wispMac":"$_mac","source":"off"}'),
      );
      final c = makeContainer(ble);
      await built(c);

      ble.seedSection(
        _lampId,
        'wispstatus',
        _b('{"wispMac":"$_mac","source":"manual","name":"whole"}'),
      );
      ble.simulateNotify(
        _lampId,
        BleUuids.controlService,
        BleUuids.wispStatus,
        _b('{"wispMac":"$_mac","sour'),
      );
      await settle();
      await settle();

      final s = c.read(wispNotifierProvider(_lampId)).value!;
      expect(s.name, 'whole');
    });

    // opSeq rides the config: a sealed op confirms off a config re-read.
    test('a sealed password op confirms from the config section', () async {
      final ble = await connectedBle();
      ble.seedSection(
        _lampId,
        'wispstatus',
        _b('{"wispMac":"$_mac","configSplit":true,"source":"osc",'
            '"hasPassword":true,"configLastSeenMs":100}'),
      );
      ble.seedSection(_lampId, 'wispconfig', _b(_config(opSeq: 4)));
      final c = makeContainer(ble);
      await built(c);
      final n = c.read(wispNotifierProvider(_lampId).notifier);

      final future = n.setPassword('newpass', currentPassword: 'oldpass');
      await settle();
      ble.seedSection(_lampId, 'wispconfig', _b(_config(opSeq: 5)));
      ble.simulateNotify(
        _lampId,
        BleUuids.controlService,
        BleUuids.wispStatus,
        _b('{"wispMac":"$_mac","configSplit":true,"source":"osc",'
            '"hasPassword":true,"configLastSeenMs":200}'),
      );

      expect(await future, isTrue);
    });

    test('pollConfig writes once inside the rate-limit window', () async {
      final ble = await connectedBle(directStatus: '{"wispMac":"$_mac"}');
      final c = makeContainer(ble);
      await built(c);
      final n = c.read(wispNotifierProvider(_lampId).notifier);

      await n.pollConfig();
      await n.pollConfig();

      final ops = ble
          .writesTo(_lampId, BleUuids.wispOp)
          .map(
            (w) => (jsonDecode(utf8.decode(w)) as Map<String, dynamic>)['op'],
          )
          .toList();
      expect(ops, ['pollConfig']);
    });
  });
}
