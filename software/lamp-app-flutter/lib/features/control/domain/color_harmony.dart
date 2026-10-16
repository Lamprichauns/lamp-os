import 'dart:math' as math;

import 'lamp_color.dart';

// Scheme table, derived-color table, orientation and math rules live in
// docs/dev/color-harmony.md.

class HarmonyScheme {
  const HarmonyScheme({
    required this.id,
    required this.key,
    required this.name,
    required this.description,
    required this.naturalCount,
  });

  final int id;
  final String key;
  final String name;
  final String description;
  final int naturalCount;
}

const List<HarmonyScheme> kHarmonySchemes = [
  HarmonyScheme(
    id: 0,
    key: 'analogous',
    name: 'Analogous',
    description: 'Neighboring hues. Smooth, calm blends.',
    naturalCount: 3,
  ),
  HarmonyScheme(
    id: 1,
    key: 'monochromatic',
    name: 'Monochromatic',
    description: 'One hue, fading toward pastel.',
    naturalCount: 3,
  ),
  HarmonyScheme(
    id: 2,
    key: 'complementary',
    name: 'Complementary',
    description: 'The primary and its opposite. High contrast.',
    naturalCount: 2,
  ),
  HarmonyScheme(
    id: 3,
    key: 'splitComplementary',
    name: 'Split complementary',
    description: 'The primary plus the two hues beside its opposite.',
    naturalCount: 3,
  ),
  HarmonyScheme(
    id: 4,
    key: 'triadic',
    name: 'Triadic',
    description: 'Three hues evenly spaced around the wheel.',
    naturalCount: 3,
  ),
  HarmonyScheme(
    id: 5,
    key: 'square',
    name: 'Square',
    description: 'Four hues evenly spaced around the wheel.',
    naturalCount: 4,
  ),
];

const _maxCount = 5;

typedef _DerivedEntry = (double hueShift, double satScale);

const Map<String, List<List<_DerivedEntry>>> _derivedTable = {
  'analogous': [
    [(30, 1)],
    [(30, 1), (60, 1)],
    [(30, 1), (60, 1), (90, 1)],
    [(30, 1), (60, 1), (90, 1), (120, 1)],
  ],
  'monochromatic': [
    [(0, 1 - 1 / 2)],
    [(0, 1 - 1 / 3), (0, 1 - 2 / 3)],
    [(0, 1 - 1 / 4), (0, 1 - 2 / 4), (0, 1 - 3 / 4)],
    [(0, 1 - 1 / 5), (0, 1 - 2 / 5), (0, 1 - 3 / 5), (0, 1 - 4 / 5)],
  ],
  'complementary': [
    [(180, 1)],
    [(0, 0.5), (180, 1)],
    [(0, 0.5), (180, 0.5), (180, 1)],
    [(0, 0.5), (0, 0), (180, 0.5), (180, 1)],
  ],
  'splitComplementary': [
    [(150, 1)],
    [(150, 1), (210, 1)],
    [(0, 0.5), (150, 1), (210, 1)],
    [(0, 0.5), (150, 1), (210, 1), (210, 0.5)],
  ],
  'triadic': [
    [(120, 1)],
    [(120, 1), (240, 1)],
    [(0, 0.5), (120, 1), (240, 1)],
    [(0, 0.5), (120, 1), (240, 1), (240, 0.5)],
  ],
  'square': [
    [(180, 1)],
    [(90, 1), (180, 1)],
    [(90, 1), (180, 1), (270, 1)],
    [(0, 0.5), (90, 1), (180, 1), (270, 1)],
  ],
};

HarmonyScheme harmonySchemeById(int? id) => kHarmonySchemes.firstWhere(
      (scheme) => scheme.id == id,
      orElse: () => kHarmonySchemes.first,
    );

double _mod(double x, double m) => (x.remainder(m) + m).remainder(m);

int _unitToByte(double value) =>
    math.min(255, math.max(0, (value * 255).round()));

// The HSV helpers follow the spec's math exactly so outputs match the web client.
({double h, double s, double v}) rgbToHsv(int red, int green, int blue) {
  final r = red / 255;
  final g = green / 255;
  final b = blue / 255;
  final max = math.max(r, math.max(g, b));
  final d = max - math.min(r, math.min(g, b));
  var h = 0.0;
  if (d != 0) {
    if (max == r) {
      h = 60 * _mod((g - b) / d, 6);
    } else if (max == g) {
      h = 60 * ((b - r) / d + 2);
    } else {
      h = 60 * ((r - g) / d + 4);
    }
  }
  return (h: _mod(h, 360), s: max == 0 ? 0.0 : d / max, v: max);
}

({int r, int g, int b}) hsvToRgb(double h, double s, double v) {
  final c = v * s;
  final hPrime = _mod(h, 360) / 60;
  final x = c * (1 - (_mod(hPrime, 2) - 1).abs());
  final m = v - c;
  final (r, g, b) = [
    (c, x, 0.0),
    (x, c, 0.0),
    (0.0, c, x),
    (0.0, x, c),
    (x, 0.0, c),
    (c, 0.0, x),
  ][hPrime.floor()];
  return (r: _unitToByte(r + m), g: _unitToByte(g + m), b: _unitToByte(b + m));
}

List<LampColor> harmonyColors(LampColor primary, int schemeId, int count) {
  final size = math.min(_maxCount, math.max(1, count));
  if (size == 1) return [primary];

  final (:h, :s, :v) = rgbToHsv(primary.r, primary.g, primary.b);
  final derived = _derivedTable[harmonySchemeById(schemeId).key]![size - 2]
      .map((entry) {
    final (hueShift, satScale) = entry;
    final rgb = hsvToRgb(h + hueShift, s * satScale, v);
    return LampColor(r: rgb.r, g: rgb.g, b: rgb.b, w: primary.w);
  });
  return [primary, ...derived];
}

List<LampColor> harmonizePalette(
  List<LampColor> storageOrder,
  int schemeId, {
  int? count,
}) =>
    harmonyColors(storageOrder.last, schemeId, count ?? storageOrder.length)
        .reversed
        .toList();

List<LampColor> autoPickPalette(
  List<LampColor> storageOrder,
  int schemeId, {
  required bool on,
}) {
  if (!on) return storageOrder;
  final count = storageOrder.length == 1
      ? harmonySchemeById(schemeId).naturalCount
      : storageOrder.length;
  return harmonizePalette(storageOrder, schemeId, count: count);
}

bool isNeutralColor(LampColor c) {
  final hsv = rgbToHsv(c.r, c.g, c.b);
  return hsv.s == 0 || hsv.v == 0;
}

int? sharedSchemeId({
  required int shadeScheme,
  required int baseScheme,
  required bool shadeEligible,
  required bool baseEligible,
}) {
  final shadeId = harmonySchemeById(shadeScheme).id;
  final baseId = harmonySchemeById(baseScheme).id;
  if (!shadeEligible && !baseEligible) return null;
  if (!baseEligible) return shadeId;
  if (!shadeEligible) return baseId;
  return shadeId == baseId ? shadeId : null;
}
