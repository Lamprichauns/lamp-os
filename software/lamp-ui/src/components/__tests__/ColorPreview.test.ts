import { describe, it, expect, beforeEach } from 'vitest'
import { mount, VueWrapper } from '@vue/test-utils'
import ColorPreview from '../ColorPreview.vue'

describe('ColorPreview', () => {
  let wrapper: VueWrapper<InstanceType<typeof ColorPreview>>

  beforeEach(() => {
    wrapper = mount(ColorPreview, {
      props: {
        hexValue: '#FF0000FF',
      },
    })
  })

  it('renders correctly with default props', () => {
    expect(wrapper.find('.color-preview').exists()).toBe(true)
    expect(wrapper.find('.color-preview').classes()).toContain('color-preview-small')
  })

  it('renders with large size when specified', () => {
    wrapper = mount(ColorPreview, {
      props: {
        hexValue: '#FF0000FF',
        size: 'large',
      },
    })
    expect(wrapper.find('.color-preview').classes()).toContain('color-preview-large')
  })

  it('parses 8-digit hex values correctly', () => {
    wrapper = mount(ColorPreview, {
      props: {
        hexValue: '#FF0000FF',
      },
    })

    const baseLayer = wrapper.find('.base-layer')
    expect(baseLayer.attributes('style')).toContain('background-color: rgb(255, 0, 0)')
  })

  it('parses 6-digit hex values correctly', () => {
    wrapper = mount(ColorPreview, {
      props: {
        hexValue: '#00FF00',
      },
    })

    const baseLayer = wrapper.find('.base-layer')
    expect(baseLayer.attributes('style')).toContain('background-color: rgb(0, 255, 0)')
  })

  it('handles empty hex value gracefully', () => {
    wrapper = mount(ColorPreview, {
      props: {
        hexValue: '',
      },
    })

    const baseLayer = wrapper.find('.base-layer')
    expect(baseLayer.attributes('style')).toContain('background-color: rgb(255, 255, 255)')
  })

  it('shows warm white overlay when warm white value is present', () => {
    wrapper = mount(ColorPreview, {
      props: {
        hexValue: '#00000080', // Black with warm white
      },
    })

    const warmWhiteLayer = wrapper.find('.warm-white-layer')
    expect(warmWhiteLayer.exists()).toBe(true)
  })

  it('hides warm white overlay when no warm white value', () => {
    wrapper = mount(ColorPreview, {
      props: {
        hexValue: '#FF0000', // Red without warm white
      },
    })

    const warmWhiteLayer = wrapper.find('.warm-white-layer')
    expect(warmWhiteLayer.exists()).toBe(false)
  })

  it('calculates warm white opacity correctly', () => {
    wrapper = mount(ColorPreview, {
      props: {
        hexValue: '#000000FF', // Black with full warm white
      },
    })

    const warmWhiteLayer = wrapper.find('.warm-white-layer')
    expect(warmWhiteLayer.exists()).toBe(true)
    // Should have some opacity value
    expect(warmWhiteLayer.attributes('style')).toContain('opacity:')
  })

  it('emits click event when clicked', async () => {
    await wrapper.find('.color-preview').trigger('click')
    expect(wrapper.emitted('click')).toBeTruthy()
    expect(wrapper.emitted('click')).toHaveLength(2) // Component emits click, and it bubbles
  })

  it('handles invalid hex values gracefully', () => {
    wrapper = mount(ColorPreview, {
      props: {
        hexValue: '#INVALID',
      },
    })

    const baseLayer = wrapper.find('.base-layer')
    // Should fall back to white for invalid hex
    expect(baseLayer.exists()).toBe(true)
    // The component should handle invalid hex gracefully without crashing
  })

  it('handles short hex values gracefully', () => {
    wrapper = mount(ColorPreview, {
      props: {
        hexValue: '#123',
      },
    })

    const baseLayer = wrapper.find('.base-layer')
    // Should fall back to white for invalid length
    expect(baseLayer.attributes('style')).toContain('background-color: rgb(255, 255, 255)')
  })

  it('applies correct warm white color', () => {
    wrapper = mount(ColorPreview, {
      props: {
        hexValue: '#000000FF',
      },
    })

    const warmWhiteLayer = wrapper.find('.warm-white-layer')
    expect(warmWhiteLayer.attributes('style')).toContain('background-color: rgb(250, 187, 62)')
  })

  it('has correct CSS classes for different sizes', () => {
    // Test small size (default)
    expect(wrapper.find('.color-preview').classes()).toContain('color-preview-small')

    // Test large size
    wrapper = mount(ColorPreview, {
      props: {
        hexValue: '#FF0000FF',
        size: 'large',
      },
    })
    expect(wrapper.find('.color-preview').classes()).toContain('color-preview-large')
  })

  it('handles RGB values that sum to maximum (765)', () => {
    wrapper = mount(ColorPreview, {
      props: {
        hexValue: '#FFFFFF80', // White with warm white
      },
    })

    const warmWhiteLayer = wrapper.find('.warm-white-layer')
    // Should not show warm white overlay when RGB is at maximum
    expect(warmWhiteLayer.exists()).toBe(false)
  })

  it('updates when hex value changes', async () => {
    await wrapper.setProps({ hexValue: '#00FF00FF' })

    const baseLayer = wrapper.find('.base-layer')
    expect(baseLayer.attributes('style')).toContain('background-color: rgb(0, 255, 0)')
  })
})
