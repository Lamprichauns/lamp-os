// @vitest-environment jsdom
import { describe, expect, it } from 'vitest'
import { mount } from '@vue/test-utils'
import ColorSchemePicker, { schemeSelection } from './ColorSchemePicker.vue'
import { harmonyColors } from '../utils/colorHarmony'
import { createGradientFromHexww } from '../utils/colorUtils'

const SHADE = '#3A7BD5C8'
const BASE = '#F4A6B800'

const mountPicker = (props: Partial<InstanceType<typeof ColorSchemePicker>['$props']> = {}) =>
  mount(ColorSchemePicker, {
    props: {
      shadeAuto: false,
      baseAuto: false,
      shadeScheme: 0,
      baseScheme: 0,
      shadePrimary: SHADE,
      basePrimary: BASE,
      shadeEditable: true,
      baseEditable: true,
      ...props,
    },
  })

type Picker = ReturnType<typeof mountPicker>

const chips = (wrapper: Picker) => wrapper.findAll('.target')

const toggleButton = (wrapper: Picker) => wrapper.get('button.change')

const clickToggle = async (wrapper: Picker) => {
  await toggleButton(wrapper).trigger('click')
}

const options = (wrapper: Picker) => wrapper.findAll('[role="radio"]')

const checked = (wrapper: Picker) => options(wrapper).map((o) => o.attributes('aria-checked'))

describe('schemeSelection', () => {
  it('returns the shared scheme when both surfaces are editable and match', () => {
    expect(schemeSelection(3, 3, true, true)).toEqual({ selectedId: 3 })
  })

  it('returns no selection when both are editable and the schemes differ', () => {
    expect(schemeSelection(1, 5, true, true)).toEqual({ selectedId: null })
  })

  it('returns the only editable surface scheme', () => {
    expect(schemeSelection(2, 4, true, false)).toEqual({ selectedId: 2 })
    expect(schemeSelection(2, 4, false, true)).toEqual({ selectedId: 4 })
  })

  it('treats an unknown id as Analogous', () => {
    expect(schemeSelection(99, 0, true, true)).toEqual({ selectedId: 0 })
    expect(schemeSelection(99, 2, true, true)).toEqual({ selectedId: null })
    expect(schemeSelection(99, 2, true, false)).toEqual({ selectedId: 0 })
  })
})

describe('ColorSchemePicker auto-pick chips', () => {
  it('shows a pressed state per surface from its autoColors flag', () => {
    const wrapper = mountPicker({ shadeAuto: true })
    expect(chips(wrapper).map((c) => c.text())).toEqual(['Shade', 'Base'])
    expect(chips(wrapper).map((c) => c.attributes('aria-pressed'))).toEqual(['true', 'false'])
  })

  it('emits the surface and on when a chip is turned on', async () => {
    const wrapper = mountPicker()
    await chips(wrapper)[0].trigger('click')
    await chips(wrapper)[1].trigger('click')
    expect(wrapper.emitted('auto-pick')).toEqual([
      [1, true],
      [2, true],
    ])
  })

  it('emits only the surface that changed when a chip is turned off', async () => {
    const wrapper = mountPicker({ shadeAuto: true, baseAuto: true })
    await chips(wrapper)[1].trigger('click')
    expect(wrapper.emitted('auto-pick')).toEqual([[2, false]])
  })

  it('lets both chips be on together', () => {
    const wrapper = mountPicker({ shadeAuto: true, baseAuto: true })
    expect(chips(wrapper).map((c) => c.attributes('aria-pressed'))).toEqual(['true', 'true'])
  })

  it('shows only the Base chip when the shade is fixed', async () => {
    const wrapper = mountPicker({ shadeEditable: false, shadeAuto: true })
    expect(chips(wrapper).map((c) => c.text())).toEqual(['Base'])
    expect(chips(wrapper)[0].attributes('aria-pressed')).toBe('false')
    await chips(wrapper)[0].trigger('click')
    expect(wrapper.emitted('auto-pick')).toEqual([[2, true]])
  })

  it('shows only the Shade chip when the base is fixed', () => {
    const wrapper = mountPicker({ baseEditable: false })
    expect(chips(wrapper).map((c) => c.text())).toEqual(['Shade'])
  })

  it('disables the chips when disabled', async () => {
    const wrapper = mountPicker({ disabled: true })
    expect(chips(wrapper).every((c) => c.attributes('disabled') !== undefined)).toBe(true)
    await chips(wrapper)[0].trigger('click')
    expect(wrapper.emitted('auto-pick')).toBeUndefined()
  })
})

describe('ColorSchemePicker scheme list', () => {
  it('starts collapsed with a Change button', () => {
    const wrapper = mountPicker({ shadeScheme: 2, baseScheme: 2 })
    expect(wrapper.find('[role="radiogroup"]').exists()).toBe(false)
    expect(toggleButton(wrapper).text()).toBe('Change')
    expect(toggleButton(wrapper).attributes('aria-expanded')).toBe('false')
    expect(wrapper.get('.scheme-summary-name').text()).toBe('Complementary')
  })

  it('opens on Change and closes on Hide', async () => {
    const wrapper = mountPicker()
    await clickToggle(wrapper)
    expect(toggleButton(wrapper).text()).toBe('Hide')
    expect(toggleButton(wrapper).attributes('aria-expanded')).toBe('true')
    expect(options(wrapper)).toHaveLength(6)
    expect(options(wrapper)[0].text()).toContain('Analogous')
    expect(options(wrapper)[0].text()).toContain('Neighboring hues. Smooth, calm blends.')
    await clickToggle(wrapper)
    expect(wrapper.find('[role="radiogroup"]').exists()).toBe(false)
    expect(toggleButton(wrapper).text()).toBe('Change')
  })

  it('emits the scheme id and keeps the list open on a pick', async () => {
    const wrapper = mountPicker({ shadeScheme: 2, baseScheme: 2 })
    await clickToggle(wrapper)
    expect(checked(wrapper)).toEqual(['false', 'false', 'true', 'false', 'false', 'false'])
    await options(wrapper)[4].trigger('click')
    expect(wrapper.emitted('pick')).toEqual([[4]])
    expect(wrapper.find('[role="radiogroup"]').exists()).toBe(true)
  })

  it('shows Mixed, no selection, and a note when the schemes differ', async () => {
    const wrapper = mountPicker({ shadeScheme: 1, baseScheme: 4 })
    expect(wrapper.get('.scheme-summary-name').text()).toBe('Mixed')
    expect(wrapper.find('.scheme-summary-swatch').exists()).toBe(false)
    expect(wrapper.get('.scheme-note').text()).toBe(
      'Shade uses Monochromatic, Base uses Triadic. Picking a scheme sets both.',
    )
    await clickToggle(wrapper)
    expect(checked(wrapper).every((value) => value === 'false')).toBe(true)
    await options(wrapper)[3].trigger('click')
    expect(wrapper.emitted('pick')).toEqual([[3]])
  })

  it('shows an unknown scheme id as Analogous', async () => {
    const wrapper = mountPicker({ shadeScheme: 99, baseScheme: 99 })
    expect(wrapper.get('.scheme-summary-name').text()).toBe('Analogous')
    await clickToggle(wrapper)
    expect(checked(wrapper)[0]).toBe('true')
  })

  it('shows the Base scheme and no note when the shade is fixed', async () => {
    const wrapper = mountPicker({ shadeEditable: false, shadeScheme: 1, baseScheme: 4 })
    expect(wrapper.get('.scheme-summary-name').text()).toBe('Triadic')
    expect(wrapper.find('.scheme-note').exists()).toBe(false)
    await clickToggle(wrapper)
    expect(checked(wrapper)).toEqual(['false', 'false', 'false', 'false', 'true', 'false'])
    await options(wrapper)[2].trigger('click')
    expect(wrapper.emitted('pick')).toEqual([[2]])
  })

  it('shows the Shade scheme and no note when the base is fixed', () => {
    const wrapper = mountPicker({ baseEditable: false, shadeScheme: 1, baseScheme: 4 })
    expect(wrapper.get('.scheme-summary-name').text()).toBe('Monochromatic')
    expect(wrapper.find('.scheme-note').exists()).toBe(false)
  })

  it('disables Change and the options when disabled', async () => {
    const wrapper = mountPicker({ disabled: true })
    expect(toggleButton(wrapper).attributes('disabled')).toBeDefined()
    const opened = mountPicker()
    await clickToggle(opened)
    await opened.setProps({ disabled: true })
    expect(options(opened).every((o) => o.attributes('disabled') !== undefined)).toBe(true)
    await options(opened)[1].trigger('click')
    expect(opened.emitted('pick')).toBeUndefined()
  })

  it('does not render the Shade / Base / Both target selector', () => {
    const wrapper = mountPicker()
    expect(wrapper.findAll('.targets')).toHaveLength(1)
    expect(wrapper.text()).not.toContain('Both')
  })
})

describe('ColorSchemePicker swatch', () => {
  const expected = (primary: string) =>
    createGradientFromHexww(harmonyColors(primary, 0, 3).reverse(), 'to right')
  const summarySwatch = (wrapper: Picker) =>
    (wrapper.get('.scheme-summary-swatch').element as HTMLElement).style.background
  const listSwatch = (wrapper: Picker) =>
    (wrapper.get('.scheme-swatch').element as HTMLElement).style.background

  it('uses the Shade primary when no chip is on', () => {
    expect(summarySwatch(mountPicker())).toBe(expected(SHADE))
  })

  it('uses the Base primary when no chip is on and the shade is fixed', () => {
    expect(summarySwatch(mountPicker({ shadeEditable: false }))).toBe(expected(BASE))
  })

  it('uses the Base primary when only the Base chip is on', () => {
    expect(summarySwatch(mountPicker({ baseAuto: true }))).toBe(expected(BASE))
  })

  it('prefers the Shade primary when both chips are on', () => {
    expect(summarySwatch(mountPicker({ shadeAuto: true, baseAuto: true }))).toBe(expected(SHADE))
  })

  it('ignores a chip flag on a fixed surface', () => {
    const wrapper = mountPicker({ shadeEditable: false, shadeAuto: true })
    expect(summarySwatch(wrapper)).toBe(expected(BASE))
  })

  it('draws the list swatches from the same primary', async () => {
    const wrapper = mountPicker({ baseAuto: true })
    await clickToggle(wrapper)
    expect(listSwatch(wrapper)).toBe(expected(BASE))
  })
})
