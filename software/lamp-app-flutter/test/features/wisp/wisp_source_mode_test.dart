import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/features/wisp/domain/wisp_source_mode.dart';

void main() {
  group('parseWispSourceMode', () {
    test('"off" → off', () {
      expect(parseWispSourceMode('off'), WispSourceMode.off);
    });

    test('"manual" → manual', () {
      expect(parseWispSourceMode('manual'), WispSourceMode.manual);
    });

    test('"osc" → osc', () {
      expect(parseWispSourceMode('osc'), WispSourceMode.osc);
    });

    test('"aurora" from an older wisp → off', () {
      expect(parseWispSourceMode('aurora'), WispSourceMode.off);
    });

    test('null defaults to off (legacy / missing key)', () {
      // Off is safe: the wisp only emits paint frames in Manual or Osc
      // mode, so Off can never accidentally override the lamps.
      expect(parseWispSourceMode(null), WispSourceMode.off);
    });

    test('unknown string defaults to off', () {
      expect(parseWispSourceMode('rainbow'), WispSourceMode.off);
      expect(parseWispSourceMode(''), WispSourceMode.off);
    });
  });

  group('wispSourceModeWire', () {
    test('round-trips each mode through the wire encoding', () {
      for (final m in WispSourceMode.values) {
        expect(parseWispSourceMode(wispSourceModeWire(m)), m,
            reason: 'mode $m should round-trip');
      }
    });

    test('osc encodes as "osc"', () {
      expect(wispSourceModeWire(WispSourceMode.osc), 'osc');
    });
  });

  group('OscRole', () {
    test('wire strings match the wisp', () {
      expect(OscRole.values.map(oscRoleWire),
          ['primary', 'secondary', 'tertiary', 'fill']);
    });

    test('round-trips each role', () {
      for (final r in OscRole.values) {
        expect(parseOscRole(oscRoleWire(r)), r);
      }
    });

    test('absent or unknown → primary', () {
      expect(parseOscRole(null), OscRole.primary);
      expect(parseOscRole(''), OscRole.primary);
      expect(parseOscRole('bass'), OscRole.primary);
    });
  });
}
