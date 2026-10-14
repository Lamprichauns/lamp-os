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
})
