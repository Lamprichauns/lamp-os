import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/features/wisp/domain/wisp_config.dart';

Uint8List _b(String s) => Uint8List.fromList(utf8.encode(s));

const _full = '{"char":"wispConfig","ledType":"GRBW","px":60,"brightness":80,'
    '"driftIntervalMs":90000,"driftFadePct":40,"shuffleSeed":7,"opSeq":3}';

void main() {
  group('WispConfig.fromBytes', () {
    test('parses every field of a wispConfig', () {
      final c = WispConfig.fromBytes(_b(_full))!;
      expect(c.ledType, 'GRBW');
      expect(c.pixelCount, 60);
      expect(c.brightness, 80);
      expect(c.driftIntervalMs, 90000);
      expect(c.driftFadePct, 40);
      expect(c.shuffleSeed, 7);
      expect(c.opSeq, 3);
    });

    test('empty bytes (a lamp without the section) → null', () {
      expect(WispConfig.fromBytes(Uint8List(0)), isNull);
    });

    test('"{}" (no config cached yet) → null', () {
      expect(WispConfig.fromBytes(_b('{}')), isNull);
    });

    test('a payload that is not a wispConfig → null', () {
      expect(
        WispConfig.fromBytes(_b(_full.replaceFirst('wispConfig', 'wispStatus'))),
        isNull,
      );
    });

    // A missing field must never be filled with a default a form could save.
    test('a missing field → null, never a default', () {
      expect(
        WispConfig.fromBytes(_b(_full.replaceFirst(',"brightness":80', ''))),
        isNull,
      );
    });

    test('malformed JSON → null', () {
      expect(WispConfig.fromBytes(_b('{"char":"wispConfig"')), isNull);
    });
  });
}
