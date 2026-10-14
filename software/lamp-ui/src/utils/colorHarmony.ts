import { formatHexww, hsvToRgb, parseHexww, rgbToHsv } from './colorUtils'

// Scheme table, derived-color table, and math rules live in docs/dev/color-harmony.md.

type SchemeKey =
  | 'analogous'
  | 'monochromatic'
  | 'complementary'
  | 'splitComplementary'
  | 'triadic'
  | 'square'

export interface ColorScheme {
  id: number
  key: SchemeKey
  name: string
  description: string
  naturalCount: number
}

export const COLOR_SCHEMES: readonly ColorScheme[] = [
  {
    id: 0,
    key: 'analogous',
    name: 'Analogous',
    description: 'Neighboring hues. Smooth, calm blends.',
    naturalCount: 3,
  },
  {
    id: 1,
    key: 'monochromatic',
    name: 'Monochromatic',
    description: 'One hue, fading toward pastel.',
    naturalCount: 3,
  },
  {
    id: 2,
    key: 'complementary',
    name: 'Complementary',
    description: 'The primary and its opposite. High contrast.',
    naturalCount: 2,
  },
  {
    id: 3,
    key: 'splitComplementary',
    name: 'Split complementary',
    description: 'The primary plus the two hues beside its opposite.',
    naturalCount: 3,
  },
  {
    id: 4,
    key: 'triadic',
    name: 'Triadic',
    description: 'Three hues evenly spaced around the wheel.',
    naturalCount: 3,
  },
  {
    id: 5,
    key: 'square',
    name: 'Square',
    description: 'Four hues evenly spaced around the wheel.',
    naturalCount: 4,
  },
]

const DEFAULT_SCHEME = COLOR_SCHEMES[0]
const MAX_COUNT = 5

type DerivedEntry = [hueShift: number, satScale: number]

function fromRule(entry: (k: number, count: number) => DerivedEntry): DerivedEntry[][] {
  return [2, 3, 4, 5].map((count) =>
    Array.from({ length: count - 1 }, (_, i) => entry(i + 1, count)),
  )
}

const DERIVED_TABLE: Record<SchemeKey, DerivedEntry[][]> = {
  analogous: fromRule((k) => [30 * k, 1]),
  monochromatic: fromRule((k, count) => [0, 1 - k / count]),
  complementary: [
    [[180, 1]],
    [
      [0, 0.5],
      [180, 1],
    ],
    [
      [0, 0.5],
      [180, 0.5],
      [180, 1],
    ],
    [
      [0, 0.5],
      [0, 0],
      [180, 0.5],
      [180, 1],
    ],
  ],
  splitComplementary: [
    [[150, 1]],
    [
      [150, 1],
      [210, 1],
    ],
    [
      [0, 0.5],
      [150, 1],
      [210, 1],
    ],
    [
      [0, 0.5],
      [150, 1],
      [210, 1],
      [210, 0.5],
    ],
  ],
  triadic: [
    [[120, 1]],
    [
      [120, 1],
      [240, 1],
    ],
    [
      [0, 0.5],
      [120, 1],
      [240, 1],
    ],
    [
      [0, 0.5],
      [120, 1],
      [240, 1],
      [240, 0.5],
    ],
  ],
  square: [
    [[180, 1]],
    [
      [90, 1],
      [180, 1],
    ],
    [
      [90, 1],
      [180, 1],
      [270, 1],
    ],
    [
      [0, 0.5],
      [90, 1],
      [180, 1],
      [270, 1],
    ],
  ],
}

export function schemeById(id: number | undefined): ColorScheme {
  return COLOR_SCHEMES.find((scheme) => scheme.id === id) ?? DEFAULT_SCHEME
}

export function harmonyColors(primary: string, schemeId: number, count: number): string[] {
  const base = parseHexww(primary)
  const size = Math.min(MAX_COUNT, Math.max(1, count))
  if (size === 1) return [formatHexww(base)]

  const { h, s, v } = rgbToHsv(base.red, base.green, base.blue)
  const derived = DERIVED_TABLE[schemeById(schemeId).key][size - 2].map(([hueShift, satScale]) =>
    formatHexww({ ...hsvToRgb(h + hueShift, s * satScale, v), warmWhite: base.warmWhite }),
  )
  return [formatHexww(base), ...derived]
}

export function harmonizePalette(
  palette: string[],
  schemeId: number,
  count = palette.length,
): string[] {
  return harmonyColors(palette[palette.length - 1], schemeId, count).reverse()
}
