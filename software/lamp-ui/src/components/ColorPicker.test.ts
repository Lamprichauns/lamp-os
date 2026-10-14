// @vitest-environment jsdom
import { beforeEach, describe, expect, it, vi } from 'vitest'
import { DOMWrapper, flushPromises, mount } from '@vue/test-utils'
import ColorPicker from './ColorPicker.vue'
import ColorWheel from './ColorWheel.vue'

const mountOpen = async (modelValue: string) => {
  const wrapper = mount(ColorPicker, {
    props: { modelValue },
    global: { stubs: { teleport: true } },
    attachTo: document.body,
  })
  await wrapper.get('.color-picker > *').trigger('click')
  return wrapper
}

const lastEmit = (wrapper: Awaited<ReturnType<typeof mountOpen>>) =>
  wrapper.emitted('update:modelValue')!.at(-1)

describe('ColorPicker wheel', () => {
  beforeEach(() => {
    document.body.innerHTML = ''
    HTMLElement.prototype.setPointerCapture = vi.fn()
    HTMLElement.prototype.releasePointerCapture = vi.fn()
  })

  it('renders a wheel in the dialog', async () => {
    const wrapper = await mountOpen('#FF000000')
    expect(wrapper.findComponent(ColorWheel).exists()).toBe(true)
    wrapper.unmount()
  })

  it('sets RGB from a wheel change and preserves warm white', async () => {
    const wrapper = await mountOpen('#FF0000C8')
    wrapper.findComponent(ColorWheel).vm.$emit('change', { hue: 120, saturation: 1 })
    expect(lastEmit(wrapper)).toEqual(['#00FF00C8'])
    expect(wrapper.emitted('preview')!.at(-1)).toEqual(['#00FF00C8'])
    wrapper.unmount()
  })

  it('scales RGB from the Brightness slider and preserves warm white', async () => {
    const wrapper = await mountOpen('#FF0000C8')
    await wrapper.get('#color-brightness').setValue('50')
    expect(lastEmit(wrapper)).toEqual(['#800000C8'])
    wrapper.unmount()
  })

  it('moves the wheel and Brightness when an RGB slider changes', async () => {
    const wrapper = await mountOpen('#FF000000')
    await wrapper.get('#green').setValue('255')
    await wrapper.get('#red').setValue('0')
    expect(wrapper.findComponent(ColorWheel).props()).toMatchObject({ hue: 120, saturation: 1 })
    expect((wrapper.get('#color-brightness').element as HTMLInputElement).value).toBe('100')
    wrapper.unmount()
  })

  it('keeps the hue when the color turns grey', async () => {
    const wrapper = await mountOpen('#0000FF00')
    await wrapper.get('#red').setValue('255')
    await wrapper.get('#green').setValue('255')
    expect(wrapper.findComponent(ColorWheel).props('saturation')).toBe(0)
    expect(wrapper.findComponent(ColorWheel).props('hue')).toBe(300)
    wrapper.unmount()
  })

  it('keeps hue and saturation when brightness reaches zero', async () => {
    const wrapper = await mountOpen('#FF800000')
    await wrapper.get('#color-brightness').setValue('0')
    await wrapper.get('#red').setValue('0')
    const wheel = wrapper.findComponent(ColorWheel)
    expect(wheel.props('hue')).toBeCloseTo(30, 0)
    expect(wheel.props('saturation')).toBeGreaterThan(0.9)
    wrapper.unmount()
  })

  it('does not close when a press began on the wheel and the click lands on the overlay', async () => {
    const wrapper = await mountOpen('#FF000000')
    const overlay = wrapper.get('.dialog-overlay')
    await wrapper.get('.color-wheel-disc').trigger('pointerdown', { button: 0, pointerId: 1, clientX: 0, clientY: 0 })
    await overlay.trigger('click')
    expect(wrapper.find('.dialog-overlay').exists()).toBe(true)
    expect(wrapper.emitted('close')).toBeUndefined()
    wrapper.unmount()
  })

  it('closes and reverts when the press and click are both on the overlay', async () => {
    const wrapper = await mountOpen('#FF000000')
    const overlay = wrapper.get('.dialog-overlay')
    wrapper.findComponent(ColorWheel).vm.$emit('change', { hue: 120, saturation: 1 })
    await overlay.trigger('pointerdown')
    await overlay.trigger('click')
    expect(wrapper.find('.dialog-overlay').exists()).toBe(false)
    expect(lastEmit(wrapper)).toEqual(['#FF000000'])
    wrapper.unmount()
  })
})

describe('ColorPicker with the real Teleport', () => {
  beforeEach(() => {
    document.body.innerHTML = ''
    HTMLElement.prototype.setPointerCapture = vi.fn()
    HTMLElement.prototype.releasePointerCapture = vi.fn()
  })

  const mountReal = async (modelValue: string) => {
    const wrapper = mount(ColorPicker, { props: { modelValue }, attachTo: document.body })
    await wrapper.get('.color-picker > *').trigger('click')
    return wrapper
  }

  it('does not feed the wheel back through RGB rounding', async () => {
    const wrapper = await mountReal('#FF000000')
    wrapper.findComponent(ColorWheel).vm.$emit('change', { hue: 200.3, saturation: 0.437 })
    await flushPromises()
    expect(wrapper.emitted('update:modelValue')).toHaveLength(1)
    expect(wrapper.findComponent(ColorWheel).props()).toMatchObject({
      hue: 200.3,
      saturation: 0.437,
    })
    wrapper.unmount()
  })

  it('returns to the same color after Brightness sweeps down and back up', async () => {
    const wrapper = await mountReal('#FF800000')
    const slider = new DOMWrapper(document.querySelector('#color-brightness')!)
    for (let v = 99; v >= 1; v--) await slider.setValue(String(v))
    for (let v = 2; v <= 100; v++) await slider.setValue(String(v))
    expect(wrapper.emitted('update:modelValue')!.at(-1)).toEqual(['#FF800000'])
    wrapper.unmount()
  })
})
