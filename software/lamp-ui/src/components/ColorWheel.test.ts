// @vitest-environment jsdom
import { beforeEach, describe, expect, it, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import ColorWheel, { hueSatToPoint, pointToHueSat } from './ColorWheel.vue'

describe('pointToHueSat', () => {
  it('maps the four compass points to 0, 90, 180 and 270 degrees', () => {
    expect(pointToHueSat(0, -50, 100).hue).toBeCloseTo(0)
    expect(pointToHueSat(50, 0, 100).hue).toBeCloseTo(90)
    expect(pointToHueSat(0, 50, 100).hue).toBeCloseTo(180)
    expect(pointToHueSat(-50, 0, 100).hue).toBeCloseTo(270)
  })

  it('gives saturation 0 at the center and clamps beyond the edge', () => {
    expect(pointToHueSat(0, 0, 100).saturation).toBe(0)
    expect(pointToHueSat(30, 40, 100).saturation).toBeCloseTo(0.5)
    expect(pointToHueSat(300, 400, 100).saturation).toBe(1)
  })
})

describe('hueSatToPoint', () => {
  it('inverts pointToHueSat', () => {
    const { x, y } = hueSatToPoint(90, 0.5, 100)
    expect(x).toBeCloseTo(50)
    expect(y).toBeCloseTo(0)
    const back = pointToHueSat(x, y, 100)
    expect(back.hue).toBeCloseTo(90)
    expect(back.saturation).toBeCloseTo(0.5)
  })
})

describe('ColorWheel', () => {
  const rect = { left: 10, top: 20, width: 200, height: 200 } as DOMRect

  beforeEach(() => {
    HTMLElement.prototype.setPointerCapture = vi.fn()
    HTMLElement.prototype.releasePointerCapture = vi.fn()
    HTMLElement.prototype.getBoundingClientRect = () => rect
  })

  const pointer = (clientX: number, clientY: number) => ({
    clientX,
    clientY,
    pointerId: 1,
    button: 0,
  })

  it('emits one change per pointer update while captured', async () => {
    const wrapper = mount(ColorWheel, { props: { hue: 0, saturation: 0 } })
    const disc = wrapper.get('.color-wheel-disc')
    await disc.trigger('pointerdown', pointer(210, 120))
    await disc.trigger('pointermove', pointer(110, 220))
    expect(HTMLElement.prototype.setPointerCapture).toHaveBeenCalledWith(1)
    const events = wrapper.emitted('change') as { hue: number; saturation: number }[][]
    expect(events).toHaveLength(2)
    expect(events[0][0].hue).toBeCloseTo(90)
    expect(events[0][0].saturation).toBeCloseTo(1)
    expect(events[1][0].hue).toBeCloseTo(180)
  })

  it('ignores movement after the pointer is released', async () => {
    const wrapper = mount(ColorWheel, { props: { hue: 0, saturation: 0 } })
    const disc = wrapper.get('.color-wheel-disc')
    await disc.trigger('pointerdown', pointer(110, 20))
    await disc.trigger('pointerup', pointer(110, 20))
    await disc.trigger('pointermove', pointer(210, 120))
    expect(wrapper.emitted('change')).toHaveLength(1)
    expect(HTMLElement.prototype.releasePointerCapture).toHaveBeenCalledWith(1)
  })

  it('steps hue with the horizontal arrows and wraps', async () => {
    const wrapper = mount(ColorWheel, { props: { hue: 358, saturation: 0.5 } })
    const disc = wrapper.get('.color-wheel-disc')
    await disc.trigger('keydown', { key: 'ArrowRight' })
    await disc.trigger('keydown', { key: 'ArrowLeft' })
    expect(wrapper.emitted('change')).toEqual([
      [{ hue: 3, saturation: 0.5 }],
      [{ hue: 353, saturation: 0.5 }],
    ])
  })

  it('steps saturation with the vertical arrows and clamps', async () => {
    const wrapper = mount(ColorWheel, { props: { hue: 10, saturation: 0.98 } })
    const disc = wrapper.get('.color-wheel-disc')
    await disc.trigger('keydown', { key: 'ArrowUp' })
    await wrapper.setProps({ saturation: 0.02 })
    await disc.trigger('keydown', { key: 'ArrowDown' })
    expect(wrapper.emitted('change')).toEqual([
      [{ hue: 10, saturation: 1 }],
      [{ hue: 10, saturation: 0 }],
    ])
  })

  it('exposes its value to assistive tech', () => {
    const wrapper = mount(ColorWheel, { props: { hue: 120.4, saturation: 0.256 } })
    const disc = wrapper.get('.color-wheel-disc')
    expect(disc.attributes('aria-label')).toBe('Color wheel')
    expect(disc.attributes('aria-valuetext')).toBe('Hue 120°, saturation 26%')
  })

  it('does not emit when disabled', async () => {
    const wrapper = mount(ColorWheel, { props: { hue: 0, saturation: 0, disabled: true } })
    const disc = wrapper.get('.color-wheel-disc')
    await disc.trigger('pointerdown', pointer(210, 120))
    await disc.trigger('keydown', { key: 'ArrowRight' })
    expect(wrapper.emitted('change')).toBeUndefined()
    expect(wrapper.classes()).toContain('disabled')
  })
})
