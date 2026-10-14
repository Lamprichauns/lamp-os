<template>
  <div>
    <FormField label="Auto-pick colors for" id="autoPickSurfaces">
      <SegmentedControl
        :model-value="autoSurfaces"
        :options="surfaceOptions"
        :disabled="disabled"
        @update:model-value="setAutoSurfaces"
      />
    </FormField>

    <FormField label="Scheme" id="colorScheme">
      <div class="scheme-summary">
        <span class="scheme-summary-name">{{ selected ? selected.name : 'Mixed' }}</span>
        <span
          v-if="selected"
          class="scheme-summary-swatch"
          :style="{ background: swatchBackground(selected) }"
        ></span>
        <button
          type="button"
          class="change"
          :aria-expanded="open"
          :disabled="disabled"
          @click="open = !open"
        >
          {{ open ? 'Hide' : 'Change' }}
        </button>
      </div>

      <div v-if="mixedNote" class="scheme-note">{{ mixedNote }}</div>

      <div v-if="open" class="scheme-list" role="radiogroup" aria-label="Color scheme">
        <button
          v-for="scheme in COLOR_SCHEMES"
          :key="scheme.id"
          type="button"
          role="radio"
          class="scheme-option"
          :class="{ active: scheme.id === selectedId }"
          :aria-checked="scheme.id === selectedId"
          :disabled="disabled"
          @click="emit('pick', scheme.id)"
        >
          <span class="scheme-name">{{ scheme.name }}</span>
          <span class="scheme-description">{{ scheme.description }}</span>
          <span class="scheme-swatch" :style="{ background: swatchBackground(scheme) }"></span>
        </button>
      </div>
    </FormField>
  </div>
</template>

<script lang="ts">
import { schemeById } from '../utils/colorHarmony'

export function schemeSelection(
  shadeScheme: number,
  baseScheme: number,
  shadeEditable: boolean,
  baseEditable: boolean,
): { selectedId: number | null } {
  const shadeId = schemeById(shadeScheme).id
  const baseId = schemeById(baseScheme).id
  if (!baseEditable) return { selectedId: shadeId }
  if (!shadeEditable) return { selectedId: baseId }
  return { selectedId: shadeId === baseId ? shadeId : null }
}
</script>

<script setup lang="ts">
import { computed, ref } from 'vue'
import FormField from './FormField.vue'
import SegmentedControl from './SegmentedControl.vue'
import { COLOR_SCHEMES, harmonyColors, type ColorScheme } from '../utils/colorHarmony'
import { createGradientFromHexww } from '../utils/colorUtils'
import {
  SURFACE_TARGET_OPTIONS,
  TARGET_BASE,
  TARGET_SHADE,
  type Surface,
} from '../utils/surfaceTarget'

interface Props {
  shadeAuto: boolean
  baseAuto: boolean
  shadeScheme: number
  baseScheme: number
  shadePrimary: string
  basePrimary: string
  shadeEditable: boolean
  baseEditable: boolean
  disabled?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  disabled: false,
})

const emit = defineEmits<{
  'auto-pick': [surface: Surface, on: boolean]
  pick: [schemeId: number]
}>()

const open = ref(false)

const surfaceOptions = computed(() =>
  SURFACE_TARGET_OPTIONS.filter(
    ({ value }) =>
      (value === TARGET_SHADE && props.shadeEditable) ||
      (value === TARGET_BASE && props.baseEditable),
  ),
)

const autoSurfaces = computed(() => [
  ...(props.shadeEditable && props.shadeAuto ? [TARGET_SHADE] : []),
  ...(props.baseEditable && props.baseAuto ? [TARGET_BASE] : []),
])

const setAutoSurfaces = (next: number[]) => {
  for (const surface of [TARGET_SHADE, TARGET_BASE] as const) {
    const on = next.includes(surface)
    if (on !== autoSurfaces.value.includes(surface)) emit('auto-pick', surface, on)
  }
}

const selectedId = computed(
  () =>
    schemeSelection(props.shadeScheme, props.baseScheme, props.shadeEditable, props.baseEditable)
      .selectedId,
)

const selected = computed(() =>
  selectedId.value === null ? null : schemeById(selectedId.value),
)

const mixedNote = computed(() =>
  selectedId.value === null
    ? `Shade uses ${schemeById(props.shadeScheme).name}, Base uses ${schemeById(props.baseScheme).name}. Picking a scheme sets both.`
    : '',
)

const previewPrimary = computed(() => {
  if (props.shadeEditable && props.shadeAuto) return props.shadePrimary
  if (props.baseEditable && props.baseAuto) return props.basePrimary
  return props.shadeEditable ? props.shadePrimary : props.basePrimary
})

const swatchBackground = (scheme: ColorScheme) =>
  createGradientFromHexww(
    harmonyColors(previewPrimary.value, scheme.id, scheme.naturalCount).reverse(),
    'to right',
  )
</script>

<style scoped>
.scheme-summary {
  display: flex;
  align-items: center;
  gap: 12px;
}

.scheme-summary-name {
  font-size: 0.9rem;
  font-weight: 500;
  color: var(--color-text);
}

.scheme-summary-swatch {
  flex: 1;
  height: 20px;
  border-radius: 2px;
}

.change {
  margin-left: auto;
  padding: 6px 12px;
  background: rgba(64, 176, 0, 0.15);
  color: var(--brand-lumen-green);
  border: 1px solid rgba(64, 176, 0, 0.5);
  border-radius: 4px;
  cursor: pointer;
}

.change:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.scheme-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
  margin-top: 12px;
}

.scheme-option {
  display: flex;
  flex-direction: column;
  align-items: stretch;
  gap: 4px;
  padding: 10px 12px;
  text-align: left;
  font: inherit;
  background: rgba(255, 255, 255, 0.05);
  color: var(--color-text);
  border: 1px solid rgba(255, 255, 255, 0.2);
  border-radius: 4px;
  cursor: pointer;
}

.scheme-option.active {
  background: rgba(64, 176, 0, 0.2);
  border-color: var(--brand-lumen-green);
}

.scheme-option:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.scheme-name {
  font-size: 0.9rem;
  font-weight: 500;
}

.scheme-option.active .scheme-name {
  color: var(--brand-lumen-green);
}

.scheme-description {
  font-size: 0.75rem;
  line-height: 1.4;
  color: var(--brand-slate-grey);
}

.scheme-swatch {
  height: 20px;
  margin-top: 4px;
  border-radius: 2px;
}

.scheme-note {
  margin-top: 8px;
  font-size: 0.75rem;
  line-height: 1.4;
  color: var(--brand-slate-grey);
}
</style>
