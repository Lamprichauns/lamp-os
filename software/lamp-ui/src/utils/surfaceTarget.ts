// Values match Expression.target.
export const TARGET_SHADE = 1
export const TARGET_BASE = 2
export const TARGET_BOTH = 3

export type Surface = typeof TARGET_SHADE | typeof TARGET_BASE

export const SURFACE_TARGET_OPTIONS = [
  { value: TARGET_SHADE, label: 'Shade' },
  { value: TARGET_BASE, label: 'Base' },
  { value: TARGET_BOTH, label: 'Both' },
]
