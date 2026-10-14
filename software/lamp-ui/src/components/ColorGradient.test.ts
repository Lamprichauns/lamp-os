// @vitest-environment jsdom
import { describe, expect, it } from 'vitest'
import { mount } from '@vue/test-utils'
import ColorGradient from './ColorGradient.vue'
import ColorPicker from './ColorPicker.vue'
import { harmonizePalette } from '../utils/colorHarmony'

const BLUE = '#3A7BD5C8'
const MANUAL = ['#111111FF', '#222222FF', '#333333FF', BLUE]

const mountGradient = (props: InstanceType<typeof ColorGradient>['$props']) =>
  mount(ColorGradient, { props, global: { stubs: { teleport: true } } })

const lastPalette = (wrapper: ReturnType<typeof mountGradient>) =>
  wrapper.emitted('update:modelValue')!.at(-1)![0] as string[]

const pickers = (wrapper: ReturnType<typeof mountGradient>) =>
  wrapper.findAllComponents(ColorPicker)

const button = (wrapper: ReturnType<typeof mountGradient>, title: string) =>
  wrapper.findAll(`button[title="${title}"]`)

describe('ColorGradient auto-pick', () => {
  it('renders the primary, the auto-pick row, then the remaining rows', () => {
    const wrapper = mountGradient({ modelValue: MANUAL })
    const rows = wrapper.get('.color-list').element.children
    expect([...rows].map((row) => row.className)).toEqual([
      'color-item',
      'auto-pick-row',
      'color-item',
      'color-item',
      'color-item',
    ])
    expect(pickers(wrapper)[0].props('modelValue')).toBe(BLUE)
  })

  it('keeps the auto-pick row visible with one color', () => {
    const wrapper = mountGradient({ modelValue: [BLUE] })
    expect(wrapper.find('.auto-pick-row').exists()).toBe(true)
  })

  it('shows the scheme name and a link that opens scheme settings', async () => {
    const wrapper = mountGradient({ modelValue: [BLUE], scheme: 2 })
    expect(wrapper.get('.auto-pick-help').text()).toBe('Complementary · change in Setup')
    await wrapper.get('.auto-pick-link').trigger('click')
    expect(wrapper.emitted('open-scheme-settings')).toHaveLength(1)
  })

  it('asks for a more colorful primary when auto is on and the primary is neutral', () => {
    const wrapper = mountGradient({ modelValue: ['#808080FF'], autoColors: true })
    expect(wrapper.get('.auto-pick-help').text()).toBe(
      'Pick a more colorful primary to vary the other colors.',
    )
  })

  it('toggling on with one color emits the natural count with the primary last', async () => {
    const wrapper = mountGradient({ modelValue: [BLUE] })
    await wrapper.get('.boolean-input').trigger('click')
    expect(wrapper.emitted('update:autoColors')).toEqual([[true]])
    const palette = lastPalette(wrapper)
    expect(palette).toHaveLength(3)
    expect(palette[2]).toBe(BLUE)
    expect(palette).toEqual(harmonizePalette([BLUE], 0, 3))
  })

  it('toggling on uses the scheme natural count', async () => {
    const wrapper = mountGradient({ modelValue: [BLUE], scheme: 5 })
    await wrapper.get('.boolean-input').trigger('click')
    const palette = lastPalette(wrapper)
    expect(palette).toHaveLength(4)
    expect(palette[3]).toBe(BLUE)
    expect(palette).toEqual(harmonizePalette([BLUE], 5, 4))
  })

  it('toggling on with four colors overwrites colors 2 to 4 and keeps the count', async () => {
    const wrapper = mountGradient({ modelValue: MANUAL, scheme: 2 })
    await wrapper.get('.boolean-input').trigger('click')
    const palette = lastPalette(wrapper)
    expect(palette).toHaveLength(4)
    expect(palette[3]).toBe(BLUE)
    expect(palette.slice(0, 3)).not.toEqual(MANUAL.slice(0, 3))
    expect(palette).toEqual(harmonizePalette(MANUAL, 2))
  })

  it('toggling off emits the flag and no palette change', async () => {
    const wrapper = mountGradient({ modelValue: MANUAL, autoColors: true })
    await wrapper.get('.boolean-input').trigger('click')
    expect(wrapper.emitted('update:autoColors')).toEqual([[false]])
    expect(wrapper.emitted('update:modelValue')).toBeUndefined()
  })

  it('regenerates the whole palette on a primary update while auto is on', () => {
    const wrapper = mountGradient({ modelValue: MANUAL, autoColors: true })
    pickers(wrapper)[0].vm.$emit('update:modelValue', '#00FF00C8')
    expect(lastPalette(wrapper)).toEqual(harmonizePalette([...MANUAL.slice(0, 3), '#00FF00C8'], 0))
  })

  it('changes only the edited index while auto is off', () => {
    const wrapper = mountGradient({ modelValue: MANUAL })
    pickers(wrapper)[1].vm.$emit('update:modelValue', '#00FF00C8')
    expect(lastPalette(wrapper)).toEqual([MANUAL[0], MANUAL[1], '#00FF00C8', MANUAL[3]])
  })

  it('changes only the primary on a primary update while auto is off', () => {
    const wrapper = mountGradient({ modelValue: MANUAL })
    pickers(wrapper)[0].vm.$emit('update:modelValue', '#00FF00C8')
    expect(lastPalette(wrapper)).toEqual([...MANUAL.slice(0, 3), '#00FF00C8'])
  })

  it('disables derived pickers only while auto is on', async () => {
    const wrapper = mountGradient({ modelValue: MANUAL, autoColors: true })
    expect(pickers(wrapper).map((picker) => picker.props('disabled'))).toEqual([
      false,
      true,
      true,
      true,
    ])
    await wrapper.setProps({ autoColors: false })
    expect(pickers(wrapper).map((picker) => picker.props('disabled'))).toEqual([
      false,
      false,
      false,
      false,
    ])
  })

  it('adds a regenerated color while auto is on', async () => {
    const wrapper = mountGradient({ modelValue: MANUAL.slice(1), autoColors: true })
    await wrapper.get('button[title="Add color"]').trigger('click')
    expect(lastPalette(wrapper)).toEqual(harmonizePalette(MANUAL.slice(1), 0, 4))
  })

  it('removes a derived color with regeneration and gives the primary no Remove', async () => {
    const wrapper = mountGradient({ modelValue: MANUAL.slice(1), autoColors: true })
    const removeButtons = wrapper
      .findAll('.color-item')
      .map((row) => row.find('button[title="Remove color"]'))
    expect(removeButtons.map((remove) => remove.exists())).toEqual([false, true, true])
    await removeButtons[1].trigger('click')
    expect(lastPalette(wrapper)).toEqual(harmonizePalette(MANUAL.slice(1), 0, 2))
  })

  it('keeps Remove on every row while auto is off', () => {
    const wrapper = mountGradient({ modelValue: MANUAL.slice(1) })
    expect(button(wrapper, 'Remove color')).toHaveLength(3)
  })

  it('disables Clone while auto is on', async () => {
    const wrapper = mountGradient({ modelValue: MANUAL.slice(1), autoColors: true })
    const disabledStates = () =>
      button(wrapper, 'Clone color').map((clone) => clone.attributes('disabled') !== undefined)
    expect(disabledStates()).toEqual([true, true, true])
    await wrapper.setProps({ autoColors: false })
    expect(disabledStates()).toEqual([false, false, false])
  })

  it('emits no palette when the parent changes modelValue or scheme', async () => {
    const wrapper = mountGradient({ modelValue: MANUAL, autoColors: true })
    await wrapper.setProps({ modelValue: ['#010203FF', '#040506FF', '#070809FF'] })
    await wrapper.setProps({ scheme: 3 })
    expect(wrapper.emitted('update:modelValue')).toBeUndefined()
    expect(pickers(wrapper)[0].props('modelValue')).toBe('#070809FF')
    expect(pickers(wrapper).map((p) => p.props('modelValue'))).toEqual([
      '#070809FF',
      '#040506FF',
      '#010203FF',
    ])
  })

  it('disables the Setup link when the component is disabled', () => {
    const wrapper = mountGradient({ modelValue: [BLUE], disabled: true })
    expect(wrapper.get('.auto-pick-link').attributes('disabled')).toBeDefined()
  })

  it('keeps the auto-pick switch mounted and focused when the count changes', async () => {
    const wrapper = mount(ColorGradient, {
      props: { modelValue: [BLUE] },
      attachTo: document.body,
      global: { stubs: { teleport: true } },
    })
    const toggle = wrapper.get('.boolean-input').element as HTMLElement
    toggle.focus()
    await wrapper.get('.boolean-input').trigger('click')
    expect(lastPalette(wrapper)).toHaveLength(3)
    await wrapper.setProps({ modelValue: lastPalette(wrapper) })
    expect(wrapper.get('.boolean-input').element).toBe(toggle)
    expect(document.activeElement).toBe(toggle)
    wrapper.unmount()
  })
})
