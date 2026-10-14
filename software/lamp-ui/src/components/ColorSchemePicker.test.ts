// @vitest-environment jsdom
import { describe, expect, it } from 'vitest'
import { mount } from '@vue/test-utils'
import ColorSchemePicker, { schemeSelection } from './ColorSchemePicker.vue'
import SegmentedControl from './SegmentedControl.vue'

const SHADE = '#3A7BD5C8'
const BASE = '#F4A6B800'

const mountPicker = (props: Partial<InstanceType<typeof ColorSchemePicker>['$props']> = {}) =>
  mount(ColorSchemePicker, {
    props: {
      shadeScheme: 0,
      baseScheme: 0,
      shadePrimary: SHADE,
      basePrimary: BASE,
      shadeEditable: true,
      baseEditable: true,
      ...props,
    },
  })

const options = (wrapper: ReturnType<typeof mountPicker>) => wrapper.findAll('[role="radio"]')

const checked = (wrapper: ReturnType<typeof mountPicker>) =>
  options(wrapper).map((o) => o.attributes('aria-checked'))

describe('schemeSelection', () => {
  it('returns the targeted surface scheme', () => {
    expect(schemeSelection(2, 4, 1)).toEqual({ selectedId: 2 })
    expect(schemeSelection(2, 4, 2)).toEqual({ selectedId: 4 })
  })

  it('returns the shared scheme for Both when the schemes match', () => {
    expect(schemeSelection(3, 3, 3)).toEqual({ selectedId: 3 })
  })

  it('returns no selection for Both when the schemes differ', () => {
    expect(schemeSelection(1, 5, 3)).toEqual({ selectedId: null })
  })

  it('treats an unknown id as Analogous', () => {
    expect(schemeSelection(99, 0, 1)).toEqual({ selectedId: 0 })
    expect(schemeSelection(99, 0, 3)).toEqual({ selectedId: 0 })
    expect(schemeSelection(99, 2, 3)).toEqual({ selectedId: null })
  })
})

describe('ColorSchemePicker', () => {
  it('renders a radio group with one option per scheme', () => {
    const wrapper = mountPicker()
    expect(wrapper.find('[role="radiogroup"]').exists()).toBe(true)
    expect(options(wrapper)).toHaveLength(6)
    expect(options(wrapper)[0].text()).toContain('Analogous')
    expect(options(wrapper)[0].text()).toContain('Neighboring hues. Smooth, calm blends.')
  })

  it('starts on Both when the schemes match', async () => {
    const wrapper = mountPicker({ shadeScheme: 2, baseScheme: 2 })
    expect(wrapper.getComponent(SegmentedControl).props('modelValue')).toBe(3)
    expect(checked(wrapper)).toEqual(['false', 'false', 'true', 'false', 'false', 'false'])
    await options(wrapper)[4].trigger('click')
    expect(wrapper.emitted('pick')).toEqual([[3, 4]])
  })

  it('starts on Shade when the schemes differ', async () => {
    const wrapper = mountPicker({ shadeScheme: 1, baseScheme: 4 })
    expect(wrapper.getComponent(SegmentedControl).props('modelValue')).toBe(1)
    expect(checked(wrapper)).toEqual(['false', 'true', 'false', 'false', 'false', 'false'])
    expect(wrapper.find('.scheme-note').exists()).toBe(false)
    await options(wrapper)[5].trigger('click')
    expect(wrapper.emitted('pick')).toEqual([[1, 5]])
  })

  it('emits the Base target after switching to Base', async () => {
    const wrapper = mountPicker({ shadeScheme: 1, baseScheme: 4 })
    await wrapper.findAll('.target')[1].trigger('click')
    expect(checked(wrapper)).toEqual(['false', 'false', 'false', 'false', 'true', 'false'])
    await options(wrapper)[0].trigger('click')
    expect(wrapper.emitted('pick')).toEqual([[2, 0]])
  })

  it('shows no selection and a note for Both with mixed schemes', async () => {
    const wrapper = mountPicker({ shadeScheme: 1, baseScheme: 4 })
    await wrapper.findAll('.target')[2].trigger('click')
    expect(checked(wrapper).every((value) => value === 'false')).toBe(true)
    expect(wrapper.get('.scheme-note').text()).toBe(
      'Shade uses Monochromatic, Base uses Triadic. Picking a scheme sets both.',
    )
    await options(wrapper)[3].trigger('click')
    expect(wrapper.emitted('pick')).toEqual([[3, 3]])
  })

  it('shows an unknown scheme id as Analogous', () => {
    const wrapper = mountPicker({ shadeScheme: 99, baseScheme: 99 })
    expect(checked(wrapper)[0]).toBe('true')
  })

  it('hides the selector and edits Base only when the shade is fixed', async () => {
    const wrapper = mountPicker({ shadeEditable: false, shadeScheme: 1, baseScheme: 4 })
    expect(wrapper.findComponent(SegmentedControl).exists()).toBe(false)
    expect(checked(wrapper)).toEqual(['false', 'false', 'false', 'false', 'true', 'false'])
    expect(wrapper.find('.scheme-note').exists()).toBe(false)
    await options(wrapper)[2].trigger('click')
    expect(wrapper.emitted('pick')).toEqual([[2, 2]])
  })

  it('hides the selector and edits Shade only when the base is fixed', async () => {
    const wrapper = mountPicker({ baseEditable: false, shadeScheme: 1, baseScheme: 4 })
    expect(wrapper.findComponent(SegmentedControl).exists()).toBe(false)
    expect(checked(wrapper)).toEqual(['false', 'true', 'false', 'false', 'false', 'false'])
    await options(wrapper)[2].trigger('click')
    expect(wrapper.emitted('pick')).toEqual([[1, 2]])
  })

  it('previews the targeted surface primary in each swatch', async () => {
    const wrapper = mountPicker({ shadeScheme: 0, baseScheme: 1 })
    const shadeSwatch = wrapper.get('.scheme-swatch').attributes('style')
    await wrapper.findAll('.target')[1].trigger('click')
    expect(wrapper.get('.scheme-swatch').attributes('style')).not.toBe(shadeSwatch)
  })

  it('disables the options when disabled', async () => {
    const wrapper = mountPicker({ disabled: true })
    expect(options(wrapper).every((o) => o.attributes('disabled') !== undefined)).toBe(true)
    await options(wrapper)[1].trigger('click')
    expect(wrapper.emitted('pick')).toBeUndefined()
  })
})
