<template>
  <div class="color-wheel" :class="{ disabled }">
    <div
      class="color-wheel-disc"
      role="slider"
      aria-label="Color wheel"
      aria-valuemin="0"
      aria-valuemax="360"
      :aria-valuenow="hueDegrees"
      :aria-valuetext="valueText"
      :aria-disabled="disabled"
      :tabindex="disabled ? -1 : 0"
      @keydown="onKeydown"
      @pointerdown="onPointerDown"
      @pointermove="onPointerMove"
      @pointerup="onPointerEnd"
      @pointercancel="onPointerEnd"
    >
      <div class="color-wheel-thumb" :style="thumbStyle"></div>
    </div>
  </div>
</template>

<script lang="ts">
export interface HueSat {
  hue: number
  saturation: number
}

// dx and dy are screen offsets from the disc center, so y grows downward.
// Hue 0 points up and increases clockwise.
export function pointToHueSat(dx: number, dy: number, radius: number): HueSat {
  return {
    hue: ((Math.atan2(dx, -dy) * 180) / Math.PI + 360) % 360,
    saturation: Math.min(1, Math.hypot(dx, dy) / radius),
  }
}

export function hueSatToPoint(hue: number, saturation: number, radius: number) {
  const angle = (hue * Math.PI) / 180
  const distance = saturation * radius
  return { x: distance * Math.sin(angle), y: -distance * Math.cos(angle) }
}
</script>

<script setup lang="ts">
import { computed } from 'vue'
import { hsvToRgb } from '../utils/colorUtils'

interface Props {
  hue: number
  saturation: number
  disabled?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  disabled: false,
})

const emit = defineEmits<{
  change: [value: HueSat]
}>()

const HUE_STEP = 5
const SATURATION_STEP = 0.05
const DISC_RADIUS_PERCENT = 50

let activePointer: number | null = null

const hueDegrees = computed(() => Math.round(props.hue) % 360)

const valueText = computed(
  () => `Hue ${hueDegrees.value}°, saturation ${Math.round(props.saturation * 100)}%`,
)

const thumbStyle = computed(() => {
  const { x, y } = hueSatToPoint(props.hue, props.saturation, DISC_RADIUS_PERCENT)
  const { red, green, blue } = hsvToRgb(props.hue, props.saturation, 1)
  return {
    left: `${DISC_RADIUS_PERCENT + x}%`,
    top: `${DISC_RADIUS_PERCENT + y}%`,
    backgroundColor: `rgb(${red}, ${green}, ${blue})`,
  }
})

const emitFromPointer = (event: PointerEvent) => {
  if (props.disabled) return
  const rect = (event.currentTarget as HTMLElement).getBoundingClientRect()
  const radius = rect.width / 2
  emit(
    'change',
    pointToHueSat(event.clientX - rect.left - radius, event.clientY - rect.top - radius, radius),
  )
}

const onPointerDown = (event: PointerEvent) => {
  if (props.disabled || event.button !== 0) return
  activePointer = event.pointerId
  const disc = event.currentTarget as HTMLElement
  disc.setPointerCapture(event.pointerId)
  emitFromPointer(event)
}

const onPointerMove = (event: PointerEvent) => {
  if (event.pointerId === activePointer) emitFromPointer(event)
}

const onPointerEnd = (event: PointerEvent) => {
  if (event.pointerId !== activePointer) return
  activePointer = null
  const disc = event.currentTarget as HTMLElement
  disc.releasePointerCapture(event.pointerId)
}

const onKeydown = (event: KeyboardEvent) => {
  if (props.disabled) return
  let { hue, saturation } = props
  if (event.key === 'ArrowRight') hue = (hue + HUE_STEP) % 360
  else if (event.key === 'ArrowLeft') hue = (hue - HUE_STEP + 360) % 360
  else if (event.key === 'ArrowUp') saturation = Math.min(1, saturation + SATURATION_STEP)
  else if (event.key === 'ArrowDown') saturation = Math.max(0, saturation - SATURATION_STEP)
  else return
  event.preventDefault()
  emit('change', { hue, saturation })
}
</script>

<style scoped>
.color-wheel {
  --wheel-size: min(240px, 64vw);
  width: var(--wheel-size);
  height: var(--wheel-size);
  margin: 0 auto;
}

.color-wheel-disc {
  position: relative;
  width: 100%;
  height: 100%;
  border-radius: 50%;
  background:
    radial-gradient(circle closest-side, #fff, rgba(255, 255, 255, 0)),
    conic-gradient(#f00, #ff0, #0f0, #0ff, #00f, #f0f, #f00);
  cursor: crosshair;
  touch-action: none;
  user-select: none;
  -webkit-user-select: none;
  outline: none;
}

.color-wheel-disc:focus-visible {
  outline: 2px solid var(--brand-aurora-blue);
  outline-offset: 4px;
}

.color-wheel-thumb {
  position: absolute;
  width: 28px;
  height: 28px;
  border: 3px solid var(--brand-lamp-white);
  border-radius: 50%;
  box-shadow:
    0 0 0 1px rgba(0, 0, 0, 0.4),
    0 2px 6px rgba(0, 0, 0, 0.5);
  transform: translate(-50%, -50%);
  pointer-events: none;
}

.color-wheel.disabled {
  opacity: 0.5;
  pointer-events: none;
}
</style>
