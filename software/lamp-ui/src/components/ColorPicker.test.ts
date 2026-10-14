// @vitest-environment jsdom
import { beforeEach, describe, expect, it } from 'vitest'
import { DOMWrapper, flushPromises, mount } from '@vue/test-utils'
import ColorPicker from './ColorPicker.vue'

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

describe('ColorPicker', () => {
  beforeEach(() => {
    document.body.innerHTML = ''
  })

  it('shows the preview, hex input and four sliders in that order', async () => {
    const wrapper = await mountOpen('#FF000000')
    const body = wrapper.get('.dialog-body').element
    const order = ['.color-preview-large', '#hex-input', '#red', '#green', '#blue', '#ww']
    const positions = order.map((selector) => {
      const el = body.querySelector(selector)
      expect(el, selector).not.toBeNull()
      return Array.from(body.querySelectorAll('*')).indexOf(el!)
    })
    expect(positions).toEqual([...positions].sort((a, b) => a - b))
    expect(wrapper.find('.color-wheel-disc').exists()).toBe(false)
    wrapper.unmount()
  })

  it('emits the new color and a preview from a slider and keeps the other channels', async () => {
    const wrapper = await mountOpen('#FF0000C8')
    await wrapper.get('#green').setValue('128')
    expect(lastEmit(wrapper)).toEqual(['#FF8000C8'])
    expect(wrapper.emitted('preview')!.at(-1)).toEqual(['#FF8000C8'])
    wrapper.unmount()
  })

  it('preserves warm white through RGB edits and edits it on its own', async () => {
    const wrapper = await mountOpen('#10203040')
    await wrapper.get('#red').setValue('255')
    expect(lastEmit(wrapper)).toEqual(['#FF203040'])
    await wrapper.get('#ww').setValue('0')
    expect(lastEmit(wrapper)).toEqual(['#FF203000'])
    wrapper.unmount()
  })

  it('reverts to the original color on Cancel', async () => {
    const wrapper = await mountOpen('#FF000000')
    await wrapper.get('#blue').setValue('255')
    await wrapper.get('.btn-cancel').trigger('click')
    expect(lastEmit(wrapper)).toEqual(['#FF000000'])
    expect(wrapper.emitted('close')).toHaveLength(1)
    expect(wrapper.find('.dialog-overlay').exists()).toBe(false)
    wrapper.unmount()
  })

  it('keeps the edit on OK', async () => {
    const wrapper = await mountOpen('#FF000000')
    await wrapper.get('#blue').setValue('255')
    await wrapper.get('.btn-ok').trigger('click')
    expect(lastEmit(wrapper)).toEqual(['#FF00FF00'])
    expect(wrapper.find('.dialog-overlay').exists()).toBe(false)
    wrapper.unmount()
  })

  it('closes and reverts when the press and click are both on the overlay', async () => {
    const wrapper = await mountOpen('#FF000000')
    const overlay = wrapper.get('.dialog-overlay')
    await wrapper.get('#green').setValue('255')
    await overlay.trigger('pointerdown')
    await overlay.trigger('click')
    expect(wrapper.find('.dialog-overlay').exists()).toBe(false)
    expect(lastEmit(wrapper)).toEqual(['#FF000000'])
    wrapper.unmount()
  })

  it('does not close or revert when a drag began in the dialog and the click lands on the overlay', async () => {
    const wrapper = await mountOpen('#FF000000')
    const overlay = wrapper.get('.dialog-overlay')
    await wrapper.get('#green').setValue('255')
    await wrapper.get('#green').trigger('pointerdown')
    await overlay.trigger('click')
    expect(wrapper.find('.dialog-overlay').exists()).toBe(true)
    expect(wrapper.emitted('close')).toBeUndefined()
    expect(lastEmit(wrapper)).toEqual(['#FFFF0000'])
    wrapper.unmount()
  })

  it('does not close on a click with no press on the overlay', async () => {
    const wrapper = await mountOpen('#FF000000')
    await wrapper.get('.dialog-overlay').trigger('click')
    expect(wrapper.find('.dialog-overlay').exists()).toBe(true)
    wrapper.unmount()
  })
})

describe('ColorPicker with the real Teleport', () => {
  beforeEach(() => {
    document.body.innerHTML = ''
  })

  const mountReal = async (modelValue: string) => {
    const wrapper = mount(ColorPicker, { props: { modelValue }, attachTo: document.body })
    await wrapper.get('.color-picker > *').trigger('click')
    return wrapper
  }

  const teleported = (selector: string) => new DOMWrapper(document.querySelector(selector)!)

  it('emits nothing on open', async () => {
    const wrapper = await mountReal('#10203040')
    await flushPromises()
    expect(wrapper.emitted('update:modelValue')).toBeUndefined()
    expect(wrapper.emitted('preview')).toBeUndefined()
    wrapper.unmount()
  })

  it('emits once for a hex edit that changes every channel', async () => {
    const wrapper = await mountReal('#10203040')
    const input = teleported('#hex-input')
    await input.setValue('#A1B2C3D4')
    await input.trigger('blur')
    await flushPromises()
    expect(wrapper.emitted('update:modelValue')).toEqual([['#A1B2C3D4']])
    expect(wrapper.emitted('preview')).toBeUndefined()
    wrapper.unmount()
  })

  it('emits once for a slider change', async () => {
    const wrapper = await mountReal('#10203040')
    await teleported('#blue').setValue('200')
    await flushPromises()
    expect(wrapper.emitted('update:modelValue')).toEqual([['#1020C840']])
    expect(wrapper.emitted('preview')).toEqual([['#1020C840']])
    wrapper.unmount()
  })

  it('shows the hex edit on the sliders', async () => {
    const wrapper = await mountReal('#10203040')
    const input = teleported('#hex-input')
    await input.setValue('#A1B2C3D4')
    await input.trigger('blur')
    await flushPromises()
    const value = (selector: string) => (teleported(selector).element as HTMLInputElement).value
    expect([value('#red'), value('#green'), value('#blue'), value('#ww')]).toEqual([
      '161',
      '178',
      '195',
      '212',
    ])
    wrapper.unmount()
  })
})
