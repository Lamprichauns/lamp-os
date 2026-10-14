import { readFileSync, writeFileSync } from 'node:fs'
import { dirname, join } from 'node:path'
import { fileURLToPath } from 'node:url'
import { describe, expect, it } from 'vitest'
import {
  COLOR_SCHEMES,
  autoPickPalette,
  harmonizePalette,
  harmonyColors,
  schemeById,
} from './colorHarmony'
import { formatHexww, hsvToRgb, parseHexww, rgbToHsv } from './colorUtils'

const SCHEME_ID = Object.fromEntries(COLOR_SCHEMES.map(({ key, id }) => [key, id]))
const GOLDEN_PATH = join(dirname(fileURLToPath(import.meta.url)), 'colorHarmony.golden.json')
const GOLDEN_PRIMARIES = ['#FF000000', '#3A7BD5C8', '#F4A6B800', '#FFFFFF00']

const VECTORS: [primary: string, scheme: string, count: number, expected: string][] = [
  ['#FF000000', 'complementary', 2, '#FF000000 #00FFFF00'],
  ['#FF000000', 'triadic', 3, '#FF000000 #00FF0000 #0000FF00'],
  ['#FF000000', 'analogous', 3, '#FF000000 #FF800000 #FFFF0000'],
  ['#FF000000', 'square', 4, '#FF000000 #80FF0000 #00FFFF00 #8000FF00'],
  ['#FF000000', 'splitComplementary', 3, '#FF000000 #00FF8000 #0080FF00'],
  ['#FF000000', 'monochromatic', 2, '#FF000000 #FF808000'],
  ['#FF000000', 'complementary', 5, '#FF000000 #FF808000 #FFFFFF00 #80FFFF00 #00FFFF00'],
  ['#FF0000C8', 'complementary', 2, '#FF0000C8 #00FFFFC8'],
  ['#FFFFFF00', 'triadic', 3, '#FFFFFF00 #FFFFFF00 #FFFFFF00'],
  ['#23000500', 'analogous', 2, '#23000500 #230C0000'],
  ['#07010000', 'analogous', 2, '#07010000 #07050000'],
  ['#00010B00', 'complementary', 4, '#00010B00 #06060B00 #0B0A0600 #0B0A0000'],
  ['#000B0100', 'analogous', 2, '#000B0100 #000B0700'],
  ['#00010500', 'splitComplementary', 2, '#00010500 #05010000'],
  ['#01011700', 'monochromatic', 4, '#01011700 #06061700 #0C0C1700 #12121700'],
]

interface GoldenCase {
  primary: string
  scheme: number
  count: number
  expected: string[]
}

const buildGoldenCases = (): GoldenCase[] =>
  GOLDEN_PRIMARIES.flatMap((primary) =>
    COLOR_SCHEMES.flatMap(({ id }) =>
      [2, 3, 4, 5].map((count) => ({
        primary,
        scheme: id,
        count,
        expected: harmonyColors(primary, id, count),
      })),
    ),
  )

describe('COLOR_SCHEMES', () => {
  it('pins the stored ids, keys, and natural counts', () => {
    expect(COLOR_SCHEMES.map(({ id, key, naturalCount }) => [id, key, naturalCount])).toEqual([
      [0, 'analogous', 3],
      [1, 'monochromatic', 3],
      [2, 'complementary', 2],
      [3, 'splitComplementary', 3],
      [4, 'triadic', 3],
      [5, 'square', 4],
    ])
  })
})

describe('schemeById', () => {
  it('returns the scheme for a known id', () => {
    expect(schemeById(4).name).toBe('Triadic')
  })

  it('falls back to analogous for an unknown or missing id', () => {
    expect(schemeById(99).key).toBe('analogous')
    expect(schemeById(-1).key).toBe('analogous')
    expect(schemeById(undefined).key).toBe('analogous')
  })
})

describe('harmonyColors', () => {
  it.each(VECTORS)('hand-checked vector %s %s %i', (primary, scheme, count, expected) => {
    expect(harmonyColors(primary, SCHEME_ID[scheme], count)).toEqual(expected.split(' '))
  })

  it('clamps count to 1 through 5', () => {
    const id = SCHEME_ID.triadic
    expect(harmonyColors('#3A7BD5C8', id, 0)).toEqual(['#3A7BD5C8'])
    expect(harmonyColors('#3A7BD5C8', id, -4)).toEqual(['#3A7BD5C8'])
    expect(harmonyColors('#3A7BD5C8', id, 9)).toHaveLength(5)
    expect(harmonyColors('#3A7BD5C8', id, 9)).toEqual(harmonyColors('#3A7BD5C8', id, 5))
  })

  it('returns exactly count colors for every scheme', () => {
    for (const { id } of COLOR_SCHEMES) {
      for (const count of [1, 2, 3, 4, 5]) {
        expect(harmonyColors('#3A7BD5C8', id, count)).toHaveLength(count)
      }
    }
  })

  it('treats an unknown scheme id as analogous', () => {
    const analogous = harmonyColors('#3A7BD5C8', SCHEME_ID.analogous, 4)
    expect(harmonyColors('#3A7BD5C8', 99, 4)).toEqual(analogous)
    expect(harmonyColors('#3A7BD5C8', -1, 4)).toEqual(analogous)
  })

  it('leads with the normalized primary', () => {
    expect(harmonyColors('#ff0000', SCHEME_ID.analogous, 1)).toEqual(['#FF000000'])
    expect(harmonyColors('#3a7bd5', SCHEME_ID.triadic, 3)[0]).toBe('#3A7BD500')
  })

  it('copies the warm white byte to every derived color', () => {
    for (const color of harmonyColors('#3A7BD5C8', SCHEME_ID.square, 5)) {
      expect(parseHexww(color).warmWhite).toBe(0xc8)
    }
  })

  it.each(['#FFFFFF00', '#808080A0', '#00000000', '#000000FF'])(
    'repeats the neutral primary %s',
    (primary) => {
      for (const { id } of COLOR_SCHEMES) {
        expect(harmonyColors(primary, id, 5)).toEqual(Array(5).fill(primary))
      }
    },
  )
})

describe('harmonizePalette', () => {
  const palette = ['#00FF0000', '#0000FF00', '#3A7BD5C8']

  it('keeps the primary at the last index and reverses harmonyColors', () => {
    const out = harmonizePalette(palette, SCHEME_ID.triadic, 3)
    expect(out[out.length - 1]).toBe('#3A7BD5C8')
    expect(out).toEqual(harmonyColors('#3A7BD5C8', SCHEME_ID.triadic, 3).reverse())
  })

  it('defaults count to the palette length', () => {
    expect(harmonizePalette(palette, SCHEME_ID.square)).toHaveLength(3)
  })

  it('resizes to the requested count with the primary still last', () => {
    const grown = harmonizePalette(palette, SCHEME_ID.square, 4)
    const shrunk = harmonizePalette(palette, SCHEME_ID.square, 2)
    expect(grown).toHaveLength(4)
    expect(shrunk).toHaveLength(2)
    expect(grown[3]).toBe('#3A7BD5C8')
    expect(shrunk[1]).toBe('#3A7BD5C8')
  })
})

describe('autoPickPalette', () => {
  const blue = '#3A7BD5C8'

  it('raises a single color to the scheme natural count with the primary last', () => {
    const out = autoPickPalette([blue], SCHEME_ID.square, true)
    expect(out).toEqual(harmonizePalette([blue], SCHEME_ID.square, 4))
    expect(out[3]).toBe(blue)
  })

  it('keeps the count when there is more than one color', () => {
    const manual = ['#111111FF', '#222222FF', blue]
    expect(autoPickPalette(manual, SCHEME_ID.complementary, true)).toEqual(
      harmonizePalette(manual, SCHEME_ID.complementary, 3),
    )
  })

  it('keeps the colors when the chip turns off', () => {
    const manual = ['#111111FF', '#222222FF', blue]
    const single = [blue]
    expect(autoPickPalette(manual, SCHEME_ID.complementary, false)).toBe(manual)
    expect(autoPickPalette(single, SCHEME_ID.square, false)).toBe(single)
  })

  it('treats an unknown scheme id as Analogous', () => {
    expect(autoPickPalette([blue], 99, true)).toEqual(harmonizePalette([blue], 0, 3))
  })
})

describe('formatHexww', () => {
  it('pads and uppercases every byte', () => {
    expect(formatHexww({ red: 0, green: 10, blue: 255, warmWhite: 5 })).toBe('#000AFF05')
  })

  it('normalizes a parsed color to eight uppercase digits', () => {
    expect(formatHexww(parseHexww('#3a7bd5'))).toBe('#3A7BD500')
  })
})

describe('rgbToHsv', () => {
  const cases: { name: string; rgb: number[]; hsv: number[] }[] = [
    { name: 'red', rgb: [255, 0, 0], hsv: [0, 1, 1] },
    { name: 'yellow', rgb: [255, 255, 0], hsv: [60, 1, 1] },
    { name: 'green', rgb: [0, 255, 0], hsv: [120, 1, 1] },
    { name: 'cyan', rgb: [0, 255, 255], hsv: [180, 1, 1] },
    { name: 'blue', rgb: [0, 0, 255], hsv: [240, 1, 1] },
    { name: 'magenta', rgb: [255, 0, 255], hsv: [300, 1, 1] },
    { name: 'rose', rgb: [255, 0, 128], hsv: [329.88235294117647, 1, 1] },
    { name: 'white', rgb: [255, 255, 255], hsv: [0, 0, 1] },
    { name: 'grey', rgb: [128, 128, 128], hsv: [0, 0, 128 / 255] },
    { name: 'black', rgb: [0, 0, 0], hsv: [0, 0, 0] },
  ]

  it.each(cases)('maps $name', ({ rgb: [red, green, blue], hsv: [h, s, v] }) => {
    const out = rgbToHsv(red, green, blue)
    expect(out.h).toBeCloseTo(h, 9)
    expect(out.s).toBeCloseTo(s, 9)
    expect(out.v).toBeCloseTo(v, 9)
  })
})

describe('hsvToRgb', () => {
  it('maps the primary hues', () => {
    expect(hsvToRgb(0, 1, 1)).toEqual({ red: 255, green: 0, blue: 0 })
    expect(hsvToRgb(120, 1, 1)).toEqual({ red: 0, green: 255, blue: 0 })
    expect(hsvToRgb(240, 1, 1)).toEqual({ red: 0, green: 0, blue: 255 })
  })

  it('normalizes a hue outside [0, 360)', () => {
    expect(hsvToRgb(360, 1, 1)).toEqual(hsvToRgb(0, 1, 1))
    expect(hsvToRgb(390, 1, 1)).toEqual(hsvToRgb(30, 1, 1))
    expect(hsvToRgb(-30, 1, 1)).toEqual(hsvToRgb(330, 1, 1))
  })

  it('clamps channels to bytes', () => {
    expect(hsvToRgb(0, 1, 2)).toEqual({ red: 255, green: 0, blue: 0 })
    expect(hsvToRgb(0, 1, -1)).toEqual({ red: 0, green: 0, blue: 0 })
  })

  it.each(GOLDEN_PRIMARIES)('round-trips %s', (primary) => {
    const { red, green, blue } = parseHexww(primary)
    const { h, s, v } = rgbToHsv(red, green, blue)
    expect(hsvToRgb(h, s, v)).toEqual({ red, green, blue })
  })

  it('round-trips a coarse grid of the RGB cube', () => {
    for (let red = 0; red <= 255; red += 17) {
      for (let green = 0; green <= 255; green += 17) {
        for (let blue = 0; blue <= 255; blue += 17) {
          const { h, s, v } = rgbToHsv(red, green, blue)
          expect(hsvToRgb(h, s, v)).toEqual({ red, green, blue })
        }
      }
    }
  })
})

describe('golden fixture', () => {
  it('matches the generator', () => {
    const cases = buildGoldenCases()
    if (process.env.UPDATE_HARMONY_GOLDEN) {
      writeFileSync(GOLDEN_PATH, `${JSON.stringify({ cases }, null, 2)}\n`)
      return
    }
    const golden: { cases: GoldenCase[] } = JSON.parse(readFileSync(GOLDEN_PATH, 'utf-8'))
    expect(golden.cases).toHaveLength(96)
    expect(golden).toEqual({ cases })
  })
})
