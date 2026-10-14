// The firmware emits/parses base and shade colors as `segments[].colors`, not a
// flat `colors` array, and on PUT it prefers `segments` over any flat `colors`.
// The UI edits one palette per surface; these map that single palette to and
// from the firmware's segment structure (fanning one palette to every segment).
import type { Segment } from '../types'

export const SAFE_COLOR = '#FF000000'

export function paletteFromSegments(
  segments: Segment[] | undefined,
  fallback: string = SAFE_COLOR,
): string[] {
  const colors = segments?.[0]?.colors
  // Uppercase to match the picker and harmony generator. App detects edits by
  // comparing palettes as strings.
  return colors && colors.length ? colors.map((c) => c.toUpperCase()) : [fallback]
}

export function pxFromSegments(segments: Segment[] | undefined): number {
  return segments?.[0]?.px ?? 0
}

// Fan the edited palette to every segment; px only touches the first segment
// (base is single-segment, and the UI never edits shade px).
export function segmentsFromPalette(
  segments: Segment[] | undefined,
  colors: string[],
  px?: number,
): Segment[] {
  const src = segments && segments.length ? segments : [{ px: px ?? 0, colors: [] }]
  return src.map((s, i) => ({
    ...s,
    colors: [...colors],
    px: i === 0 && px !== undefined ? px : s.px,
  }))
}
