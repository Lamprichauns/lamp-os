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
  it('has no toggle row and renders one row per color', () => {
    const wrapper = mountGradient({ modelValue: MANUAL })
    const rows = wrapper.get('.color-list').element.children
    expect([...rows].map((row) => row.className)).toEqual([
      'color-item',
      'color-item',
      'color-item',
      'color-item',
    ])
    expect(wrapper.find('.boolean-input').exists()).toBe(false)
    expect(pickers(wrapper)[0].props('modelValue')).toBe(BLUE)
  })

  it('shows no note while auto is off', () => {
    const wrapper = mountGradient({ modelValue: [BLUE], scheme: 2 })
    expect(wrapper.find('.auto-pick-note').exists()).toBe(false)
  })

  it('shows one note under the primary while auto is on, linking to Setup', async () => {
    const wrapper = mountGradient({ modelValue: MANUAL, scheme: 2, autoColors: true })
    const rows = wrapper.get('.color-list').element.children
    expect([...rows].map((row) => row.className)).toEqual([
      'color-item',
      'auto-pick-note',
      'color-item',
      'color-item',
      'color-item',
    ])
    expect(wrapper.get('.auto-pick-note').text()).toBe('Auto-picked · Complementary')
    await wrapper.get('.auto-pick-link').trigger('click')
    expect(wrapper.emitted('open-scheme-settings')).toHaveLength(1)
  })

  it('names the scheme in the link and treats an unknown id as Analogous', () => {
    const wrapper = mountGradient({ modelValue: [BLUE], scheme: 99, autoColors: true })
    expect(wrapper.get('.auto-pick-link').text()).toBe('Analogous')
  })

  it('asks for a more colorful top color, with no link, when the primary is neutral', () => {
    const wrapper = mountGradient({ modelValue: ['#808080FF'], autoColors: true })
    expect(wrapper.get('.auto-pick-note').text()).toBe(
      'Auto-picked · pick a more colorful top color',
    )
    expect(wrapper.find('.auto-pick-link').exists()).toBe(false)
  })

  it('removes the note when auto turns off', async () => {
    const wrapper = mountGradient({ modelValue: MANUAL, autoColors: true })
    await wrapper.setProps({ autoColors: false })
    expect(wrapper.find('.auto-pick-note').exists()).toBe(false)
  })

  it('emits no palette when auto turns on or off', async () => {
    const wrapper = mountGradient({ modelValue: MANUAL })
    await wrapper.setProps({ autoColors: true })
    await wrapper.setProps({ autoColors: false })
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
    const wrapper = mountGradient({ modelValue: [BLUE], autoColors: true, disabled: true })
    expect(wrapper.get('.auto-pick-link').attributes('disabled')).toBeDefined()
  })

  it('keeps the primary picker mounted when the count changes', async () => {
    const wrapper = mountGradient({ modelValue: [BLUE] })
    const primary = pickers(wrapper)[0].element
    await wrapper.setProps({ autoColors: true, modelValue: harmonizePalette([BLUE], 0, 3) })
    expect(pickers(wrapper)).toHaveLength(3)
    expect(pickers(wrapper)[0].element).toBe(primary)
  })
})
