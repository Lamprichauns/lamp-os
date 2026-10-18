// @vitest-environment jsdom
import { describe, expect, it } from 'vitest'
import { mount } from '@vue/test-utils'
import SegmentedControl from './SegmentedControl.vue'

const OPTIONS = [
  { value: 1, label: 'Shade' },
  { value: 2, label: 'Base' },
  { value: 3, label: 'Both' },
]

describe('SegmentedControl', () => {
  it('marks only the selected option active', () => {
    const wrapper = mount(SegmentedControl, { props: { modelValue: 2, options: OPTIONS } })
    expect(wrapper.findAll('.target').map((b) => b.classes('active'))).toEqual([false, true, false])
  })

  it('emits the clicked option value', async () => {
    const wrapper = mount(SegmentedControl, { props: { modelValue: 1, options: OPTIONS } })
    await wrapper.findAll('.target')[2].trigger('click')
    expect(wrapper.emitted('update:modelValue')).toEqual([[3]])
  })

  it('disables every button and emits nothing when disabled', async () => {
    const wrapper = mount(SegmentedControl, {
      props: { modelValue: 1, options: OPTIONS, disabled: true },
    })
    const buttons = wrapper.findAll('.target')
    expect(buttons.every((b) => b.attributes('disabled') !== undefined)).toBe(true)
    await buttons[1].trigger('click')
    expect(wrapper.emitted('update:modelValue')).toBeUndefined()
  })

  it('renders no aria-pressed in single-select mode', () => {
    const wrapper = mount(SegmentedControl, { props: { modelValue: 1, options: OPTIONS } })
    expect(wrapper.findAll('.target').every((b) => b.attributes('aria-pressed') === undefined)).toBe(
      true,
    )
  })

  describe('multi-select', () => {
    const mountMulti = (modelValue: number[], disabled = false) =>
      mount(SegmentedControl, { props: { modelValue, options: OPTIONS, disabled } })

    it('marks every selected option active and pressed', () => {
      const wrapper = mountMulti([1, 3])
      const buttons = wrapper.findAll('.target')
      expect(buttons.map((b) => b.classes('active'))).toEqual([true, false, true])
      expect(buttons.map((b) => b.attributes('aria-pressed'))).toEqual(['true', 'false', 'true'])
    })

    it('adds an unselected value to the array', async () => {
      const wrapper = mountMulti([1])
      await wrapper.findAll('.target')[1].trigger('click')
      expect(wrapper.emitted('update:modelValue')).toEqual([[[1, 2]]])
    })

    it('removes a selected value from the array', async () => {
      const wrapper = mountMulti([1, 2])
      await wrapper.findAll('.target')[0].trigger('click')
      expect(wrapper.emitted('update:modelValue')).toEqual([[[2]]])
    })

    it('can select nothing', async () => {
      const wrapper = mountMulti([2])
      await wrapper.findAll('.target')[1].trigger('click')
      expect(wrapper.emitted('update:modelValue')).toEqual([[[]]])
    })

    it('emits nothing when disabled', async () => {
      const wrapper = mountMulti([1], true)
      await wrapper.findAll('.target')[1].trigger('click')
      expect(wrapper.emitted('update:modelValue')).toBeUndefined()
    })
  })
})
