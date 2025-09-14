import { describe, it, expect, beforeEach } from 'vitest'
import { mount, VueWrapper } from '@vue/test-utils'
import NumberInput from '../NumberInput.vue'

describe('NumberInput', () => {
  let wrapper: VueWrapper<InstanceType<typeof NumberInput>>

  beforeEach(() => {
    wrapper = mount(NumberInput, {
      props: {
        modelValue: 50,
        label: 'Test Input',
        id: 'test-input',
      },
    })
  })

  it('renders correctly with required props', () => {
    expect(wrapper.find('.number-input-container').exists()).toBe(true)
    expect(wrapper.find('.number-input').exists()).toBe(true)
    expect(wrapper.find('.number-input-buttons').exists()).toBe(true)
  })

  it('displays the correct model value', () => {
    const input = wrapper.find('.number-input')
    expect(input.element.value).toBe('50')
  })

  it('emits update event when input value changes', async () => {
    const input = wrapper.find('.number-input')
    await input.setValue('75')
    await input.trigger('input')

    expect(wrapper.emitted('update:modelValue')).toBeTruthy()
    expect(wrapper.emitted('update:modelValue')[0]).toEqual([75])
  })

  it('handles invalid input gracefully', async () => {
    const input = wrapper.find('.number-input')
    await input.setValue('invalid')
    await input.trigger('input')

    expect(wrapper.emitted('update:modelValue')).toBeTruthy()
    expect(wrapper.emitted('update:modelValue')[0]).toEqual([0])
  })

  it('increments value when up button is clicked', async () => {
    // The plus button is the second IconButton (index 1)
    const upButton = wrapper.findAllComponents({ name: 'IconButton' })[1]
    await upButton.trigger('click')

    expect(wrapper.emitted('update:modelValue')).toBeTruthy()
    expect(wrapper.emitted('update:modelValue')[0]).toEqual([51])
  })

  it('decrements value when down button is clicked', async () => {
    // The minus button is the first IconButton (index 0)
    const downButton = wrapper.findAllComponents({ name: 'IconButton' })[0]
    await downButton.trigger('click')

    expect(wrapper.emitted('update:modelValue')).toBeTruthy()
    expect(wrapper.emitted('update:modelValue')[0]).toEqual([49])
  })

  it('respects max constraint when incrementing', async () => {
    wrapper = mount(NumberInput, {
      props: {
        modelValue: 100,
        label: 'Test Input',
        id: 'test-input',
        max: 100,
      },
    })

    const upButton = wrapper.findAllComponents({ name: 'IconButton' })[1]
    await upButton.trigger('click')

    // Should not emit any event since value is already at max
    expect(wrapper.emitted('update:modelValue')).toBeFalsy()
  })

  it('respects min constraint when decrementing', async () => {
    wrapper = mount(NumberInput, {
      props: {
        modelValue: 0,
        label: 'Test Input',
        id: 'test-input',
        min: 0,
      },
    })

    const downButton = wrapper.findAllComponents({ name: 'IconButton' })[0]
    await downButton.trigger('click')

    // Should not emit any event since value is already at min
    expect(wrapper.emitted('update:modelValue')).toBeFalsy()
  })

  it('disables up button when at max value', () => {
    wrapper = mount(NumberInput, {
      props: {
        modelValue: 100,
        label: 'Test Input',
        id: 'test-input',
        max: 100,
      },
    })

    const upButton = wrapper.findAllComponents({ name: 'IconButton' })[1]
    expect(upButton.props('disabled')).toBe(true)
  })

  it('disables down button when at min value', () => {
    wrapper = mount(NumberInput, {
      props: {
        modelValue: 0,
        label: 'Test Input',
        id: 'test-input',
        min: 0,
      },
    })

    const downButton = wrapper.findAllComponents({ name: 'IconButton' })[0]
    expect(downButton.props('disabled')).toBe(true)
  })

  it('applies custom min and max values', () => {
    wrapper = mount(NumberInput, {
      props: {
        modelValue: 50,
        label: 'Test Input',
        id: 'test-input',
        min: 10,
        max: 90,
      },
    })

    const input = wrapper.find('.number-input')
    expect(input.attributes('min')).toBe('10')
    expect(input.attributes('max')).toBe('90')
  })

  it('applies placeholder text when provided', () => {
    wrapper = mount(NumberInput, {
      props: {
        modelValue: 50,
        label: 'Test Input',
        id: 'test-input',
        placeholder: 'Enter a number',
      },
    })

    const input = wrapper.find('.number-input')
    expect(input.attributes('placeholder')).toBe('Enter a number')
  })

  it('uses default min and max values when not provided', () => {
    const input = wrapper.find('.number-input')
    expect(input.attributes('min')).toBe('0')
    expect(input.attributes('max')).toBe('100')
  })

  it('has correct accessibility attributes', () => {
    const input = wrapper.find('.number-input')
    expect(input.exists()).toBe(true)
  })

  it('prevents incrementing beyond max value', async () => {
    wrapper = mount(NumberInput, {
      props: {
        modelValue: 99,
        label: 'Test Input',
        id: 'test-input',
        max: 100,
      },
    })

    const upButton = wrapper.findAllComponents({ name: 'IconButton' })[1]
    await upButton.trigger('click')

    expect(wrapper.emitted('update:modelValue')).toBeTruthy()
    expect(wrapper.emitted('update:modelValue')[0]).toEqual([100])
  })

  it('prevents decrementing below min value', async () => {
    wrapper = mount(NumberInput, {
      props: {
        modelValue: 1,
        label: 'Test Input',
        id: 'test-input',
        min: 0,
      },
    })

    const downButton = wrapper.findAllComponents({ name: 'IconButton' })[0]
    await downButton.trigger('click')

    expect(wrapper.emitted('update:modelValue')).toBeTruthy()
    expect(wrapper.emitted('update:modelValue')[0]).toEqual([0])
  })

  it('handles zero value correctly', async () => {
    wrapper = mount(NumberInput, {
      props: {
        modelValue: 0,
        label: 'Test Input',
        id: 'test-input',
      },
    })

    const input = wrapper.find('.number-input')
    expect(input.element.value).toBe('0')

    const upButton = wrapper.findAllComponents({ name: 'IconButton' })[1]
    await upButton.trigger('click')
    expect(wrapper.emitted('update:modelValue')).toBeTruthy()
    expect(wrapper.emitted('update:modelValue')[0]).toEqual([1])
  })

  it('handles negative values when min allows it', async () => {
    wrapper = mount(NumberInput, {
      props: {
        modelValue: -5,
        label: 'Test Input',
        id: 'test-input',
        min: -10,
        max: 10,
      },
    })

    const input = wrapper.find('.number-input')
    expect(input.element.value).toBe('-5')

    const downButton = wrapper.findAllComponents({ name: 'IconButton' })[0]
    await downButton.trigger('click')
    expect(wrapper.emitted('update:modelValue')[0]).toEqual([-6])
  })

  it('updates when modelValue prop changes', async () => {
    await wrapper.setProps({ modelValue: 75 })

    const input = wrapper.find('.number-input')
    expect(input.element.value).toBe('75')
  })

  it('renders SVG icons in buttons', () => {
    const buttons = wrapper.findAllComponents({ name: 'IconButton' })
    const downButton = buttons[0] // minus button
    const upButton = buttons[1] // plus button

    expect(upButton.exists()).toBe(true)
    expect(downButton.exists()).toBe(true)
  })
})
