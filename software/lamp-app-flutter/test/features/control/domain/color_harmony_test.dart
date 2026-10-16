import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/features/control/domain/color_harmony.dart';
import 'package:lamp_app/features/control/domain/lamp_color.dart';

const _goldenPath = '../lamp-ui/src/utils/colorHarmony.golden.json';
const _goldenPrimaries = ['#FF000000', '#3A7BD5C8', '#F4A6B800', '#FFFFFF00'];

const _vectors = <(String, String, int, String)>[
  ('#FF000000', 'complementary', 2, '#FF000000 #00FFFF00'),
  ('#FF000000', 'triadic', 3, '#FF000000 #00FF0000 #0000FF00'),
  ('#FF000000', 'analogous', 3, '#FF000000 #FF800000 #FFFF0000'),
  ('#FF000000', 'square', 4, '#FF000000 #80FF0000 #00FFFF00 #8000FF00'),
  ('#FF000000', 'splitComplementary', 3, '#FF000000 #00FF8000 #0080FF00'),
  ('#FF000000', 'monochromatic', 2, '#FF000000 #FF808000'),
  (
    '#FF000000',
    'complementary',
    5,
    '#FF000000 #FF808000 #FFFFFF00 #80FFFF00 #00FFFF00',
  ),
  ('#FF0000C8', 'complementary', 2, '#FF0000C8 #00FFFFC8'),
  ('#FFFFFF00', 'triadic', 3, '#FFFFFF00 #FFFFFF00 #FFFFFF00'),
  ('#23000500', 'analogous', 2, '#23000500 #230C0000'),
  ('#07010000', 'analogous', 2, '#07010000 #07050000'),
  (
    '#00010B00',
    'complementary',
    4,
    '#00010B00 #06060B00 #0B0A0600 #0B0A0000',
  ),
  ('#000B0100', 'analogous', 2, '#000B0100 #000B0700'),
  ('#00010500', 'splitComplementary', 2, '#00010500 #05010000'),
  (
    '#01011700',
    'monochromatic',
    4,
    '#01011700 #06061700 #0C0C1700 #12121700',
  ),
  ('#000A2100', 'analogous', 2, '#000A2100 #07002100'),
  (
    '#00001A00',
    'monochromatic',
    4,
    '#00001A00 #06061A00 #0D0D1A00 #14141A00',
  ),
  ('#01012000', 'analogous', 2, '#01012000 #10012000'),
];

LampColor _c(String hex) => LampColor.fromHex(hex);

List<String> _hex(List<LampColor> colors) =>
    colors.map((c) => c.toHex()).toList();

int _idForKey(String key) =>
    kHarmonySchemes.firstWhere((scheme) => scheme.key == key).id;

void main() {
  group('kHarmonySchemes', () {
    test('pins id, key, name and natural count in id order', () {
      expect(
        kHarmonySchemes
            .map((s) => (s.id, s.key, s.name, s.naturalCount))
            .toList(),
        [
          (0, 'analogous', 'Analogous', 3),
          (1, 'monochromatic', 'Monochromatic', 3),
          (2, 'complementary', 'Complementary', 2),
          (3, 'splitComplementary', 'Split complementary', 3),
          (4, 'triadic', 'Triadic', 3),
          (5, 'square', 'Square', 4),
        ],
      );
    });

    test('descriptions match the spec one-liners', () {
      expect(kHarmonySchemes.map((s) => s.description).toList(), [
        'Neighboring hues. Smooth, calm blends.',
        'One hue, fading toward pastel.',
        'The primary and its opposite. High contrast.',
        'The primary plus the two hues beside its opposite.',
        'Three hues evenly spaced around the wheel.',
        'Four hues evenly spaced around the wheel.',
      ]);
    });
  });

  group('harmonySchemeById', () {
    test('returns the scheme for a known id', () {
      expect(harmonySchemeById(4).name, 'Triadic');
    });

    test('falls back to Analogous for an unknown or null id', () {
      expect(harmonySchemeById(99).key, 'analogous');
      expect(harmonySchemeById(-1).key, 'analogous');
      expect(harmonySchemeById(null).key, 'analogous');
    });
  });

  group('harmonyColors', () {
    for (final (primary, scheme, count, expected) in _vectors) {
      test('hand-checked vector $primary $scheme $count', () {
        expect(
          _hex(harmonyColors(_c(primary), _idForKey(scheme), count)),
          expected.split(' '),
        );
      });
    }

    test('clamps count to 1 through 5', () {
      final primary = _c('#3A7BD5C8');
      final id = _idForKey('triadic');
      expect(harmonyColors(primary, id, 0), [primary]);
      expect(harmonyColors(primary, id, -4), [primary]);
      expect(harmonyColors(primary, id, 9), hasLength(5));
      expect(harmonyColors(primary, id, 9), harmonyColors(primary, id, 5));
    });

    test('returns exactly count colors for every scheme', () {
      for (final scheme in kHarmonySchemes) {
        for (final count in [1, 2, 3, 4, 5]) {
          expect(
            harmonyColors(_c('#3A7BD5C8'), scheme.id, count),
            hasLength(count),
          );
        }
      }
    });

    test('treats unknown id 42 as Analogous', () {
      final primary = _c('#3A7BD5C8');
      final analogous = harmonyColors(primary, 0, 4);
      expect(harmonyColors(primary, 42, 4), analogous);
      expect(harmonyColors(primary, -1, 4), analogous);
    });

    test('leads with the primary instance itself', () {
      final primary = _c('#3A7BD5C8');
      expect(harmonyColors(primary, 0, 1).single, same(primary));
      expect(harmonyColors(primary, 4, 3).first, same(primary));
    });

    test('copies W: #FF0000C8 complementary 2', () {
      expect(
        _hex(harmonyColors(_c('#FF0000C8'), _idForKey('complementary'), 2)),
        ['#FF0000C8', '#00FFFFC8'],
      );
      for (final color in harmonyColors(_c('#3A7BD5C8'), 5, 5)) {
        expect(color.w, 0xC8);
      }
    });

    for (final hex in ['#FFFFFF00', '#808080A0', '#00000000', '#000000FF']) {
      test('repeats the neutral primary $hex', () {
        final primary = _c(hex);
        for (final scheme in kHarmonySchemes) {
          expect(
            harmonyColors(primary, scheme.id, 5),
            List.filled(5, primary),
          );
        }
      });
    }
  });

  group('harmonizePalette', () {
    final palette = [_c('#00FF0000'), _c('#0000FF00'), _c('#3A7BD5C8')];

    test('keeps the primary last and equals harmonyColors reversed', () {
      final out = harmonizePalette(palette, 4, count: 3);
      expect(out.last, _c('#3A7BD5C8'));
      expect(out, harmonyColors(_c('#3A7BD5C8'), 4, 3).reversed.toList());
    });

    test('defaults count to the palette length', () {
      expect(harmonizePalette(palette, 5), hasLength(3));
    });

    test('resizes to the requested count with the primary still last', () {
      final grown = harmonizePalette(palette, 5, count: 4);
      final shrunk = harmonizePalette(palette, 5, count: 2);
      expect(grown, hasLength(4));
      expect(shrunk, hasLength(2));
      expect(grown.last, _c('#3A7BD5C8'));
      expect(shrunk.last, _c('#3A7BD5C8'));
    });
  });

  group('autoPickPalette', () {
    final blue = _c('#3A7BD5C8');

    test('off returns the identical list', () {
      final manual = [_c('#111111FF'), _c('#222222FF'), blue];
      final single = [blue];
      expect(autoPickPalette(manual, 2, on: false), same(manual));
      expect(autoPickPalette(single, 5, on: false), same(single));
    });

    test('on with one color raises to the natural count', () {
      final analogous = autoPickPalette([blue], 0, on: true);
      expect(analogous, hasLength(3));
      expect(analogous, harmonizePalette([blue], 0, count: 3));
      final square = autoPickPalette([blue], 5, on: true);
      expect(square, hasLength(4));
      expect(square, harmonizePalette([blue], 5, count: 4));
      expect(square.last, blue);
    });

    test('on with four colors keeps four', () {
      final manual = [_c('#111111FF'), _c('#222222FF'), _c('#333333FF'), blue];
      final out = autoPickPalette(manual, 2, on: true);
      expect(out, hasLength(4));
      expect(out, harmonizePalette(manual, 2, count: 4));
    });

    test('treats an unknown scheme id as Analogous', () {
      expect(
        autoPickPalette([blue], 99, on: true),
        harmonizePalette([blue], 0, count: 3),
      );
    });
  });

  group('isNeutralColor', () {
    test('is true for white, grey and black', () {
      expect(isNeutralColor(_c('#FFFFFF00')), isTrue);
      expect(isNeutralColor(_c('#80808000')), isTrue);
      expect(isNeutralColor(_c('#000000FF')), isTrue);
    });

    test('is false for red', () {
      expect(isNeutralColor(_c('#FF000000')), isFalse);
    });
  });

  group('sharedSchemeId', () {
    test('maps unknown ids to Analogous before comparing', () {
      expect(
        sharedSchemeId(
          shadeScheme: 0,
          baseScheme: 99,
          shadeEligible: true,
          baseEligible: true,
        ),
        0,
      );
    });

    test('is null when both are eligible and differ', () {
      expect(
        sharedSchemeId(
          shadeScheme: 4,
          baseScheme: 5,
          shadeEligible: true,
          baseEligible: true,
        ),
        isNull,
      );
    });

    test('returns the one eligible surface scheme', () {
      expect(
        sharedSchemeId(
          shadeScheme: 4,
          baseScheme: 5,
          shadeEligible: true,
          baseEligible: false,
        ),
        4,
      );
      expect(
        sharedSchemeId(
          shadeScheme: 4,
          baseScheme: 5,
          shadeEligible: false,
          baseEligible: true,
        ),
        5,
      );
      expect(
        sharedSchemeId(
          shadeScheme: 42,
          baseScheme: 5,
          shadeEligible: true,
          baseEligible: false,
        ),
        0,
      );
    });

    test('is null when no surface is eligible', () {
      expect(
        sharedSchemeId(
          shadeScheme: 4,
          baseScheme: 4,
          shadeEligible: false,
          baseEligible: false,
        ),
        isNull,
      );
    });
  });

  group('rgbToHsv', () {
    const cases = <(String, (int, int, int), (double, double, double))>[
      ('red', (255, 0, 0), (0, 1, 1)),
      ('yellow', (255, 255, 0), (60, 1, 1)),
      ('green', (0, 255, 0), (120, 1, 1)),
      ('cyan', (0, 255, 255), (180, 1, 1)),
      ('blue', (0, 0, 255), (240, 1, 1)),
      ('magenta', (255, 0, 255), (300, 1, 1)),
      ('rose', (255, 0, 128), (329.88235294117647, 1, 1)),
      ('white', (255, 255, 255), (0, 0, 1)),
      ('grey', (128, 128, 128), (0, 0, 128 / 255)),
      ('black', (0, 0, 0), (0, 0, 0)),
    ];

    for (final (name, (red, green, blue), (h, s, v)) in cases) {
      test('maps $name', () {
        final out = rgbToHsv(red, green, blue);
        expect(out.h, closeTo(h, 5e-10));
        expect(out.s, closeTo(s, 5e-10));
        expect(out.v, closeTo(v, 5e-10));
      });
    }
  });

  group('hsvToRgb', () {
    test('maps the primary hues', () {
      expect(hsvToRgb(0, 1, 1), (r: 255, g: 0, b: 0));
      expect(hsvToRgb(120, 1, 1), (r: 0, g: 255, b: 0));
      expect(hsvToRgb(240, 1, 1), (r: 0, g: 0, b: 255));
    });

    test('normalizes a hue outside [0, 360)', () {
      expect(hsvToRgb(360, 1, 1), hsvToRgb(0, 1, 1));
      expect(hsvToRgb(390, 1, 1), hsvToRgb(30, 1, 1));
      expect(hsvToRgb(-30, 1, 1), hsvToRgb(330, 1, 1));
    });

    test('clamps channels to bytes', () {
      expect(hsvToRgb(0, 1, 2), (r: 255, g: 0, b: 0));
      expect(hsvToRgb(0, 1, -1), (r: 0, g: 0, b: 0));
    });

    for (final hex in _goldenPrimaries) {
      test('round-trips $hex', () {
        final c = _c(hex);
        final hsv = rgbToHsv(c.r, c.g, c.b);
        expect(hsvToRgb(hsv.h, hsv.s, hsv.v), (r: c.r, g: c.g, b: c.b));
      });
    }

    test('round-trips a coarse grid of the RGB cube', () {
      for (var red = 0; red <= 255; red += 17) {
        for (var green = 0; green <= 255; green += 17) {
          for (var blue = 0; blue <= 255; blue += 17) {
            final hsv = rgbToHsv(red, green, blue);
            expect(
              hsvToRgb(hsv.h, hsv.s, hsv.v),
              (r: red, g: green, b: blue),
            );
          }
        }
      }
    });
  });

  group('golden fixture', () {
    test('matches every case', () {
      final file = File(_goldenPath);
      if (!file.existsSync()) {
        fail('Golden fixture not found at ${file.absolute.path}');
      }
      final golden =
          jsonDecode(file.readAsStringSync()) as Map<String, dynamic>;
      final cases = golden['cases'] as List<dynamic>;
      expect(cases, hasLength(96));
      for (final raw in cases) {
        final entry = raw as Map<String, dynamic>;
        final primary = entry['primary'] as String;
        final scheme = entry['scheme'] as int;
        final count = entry['count'] as int;
        final expected = (entry['expected'] as List<dynamic>)
            .map((e) => e as String)
            .toList();
        expect(
          _hex(harmonyColors(_c(primary), scheme, count)),
          expected,
          reason: '$primary scheme $scheme count $count',
        );
      }
    });
  });
}
