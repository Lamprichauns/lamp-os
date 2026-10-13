// @vitest-environment jsdom
import { describe, expect, it } from 'vitest'
import { mount } from '@vue/test-utils'
import BooleanInput from './BooleanInput.vue'

describe('BooleanInput', () => {
  it('emits the toggled value when clicked', async () => {
    const wrapper = mount(BooleanInput, { props: { modelValue: false } })
    await wrapper.get('button').trigger('click')
    expect(wrapper.emitted('update:modelValue')).toEqual([[true]])
  })
})
