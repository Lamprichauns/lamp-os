<template>
  <div class="scheme-picker">
    <SegmentedControl
      v-if="shadeEditable && baseEditable"
      v-model="target"
      :options="targetOptions"
      :disabled="disabled"
    />

    <div class="scheme-list" role="radiogroup" aria-label="Color scheme">
      <button
        v-for="scheme in COLOR_SCHEMES"
        :key="scheme.id"
        type="button"
        role="radio"
        class="scheme-option"
        :class="{ active: scheme.id === selectedId }"
        :aria-checked="scheme.id === selectedId"
        :disabled="disabled"
        @click="emit('pick', activeTarget, scheme.id)"
      >
        <span class="scheme-name">{{ scheme.name }}</span>
        <span class="scheme-description">{{ scheme.description }}</span>
        <span class="scheme-swatch" :style="{ background: swatchBackground(scheme) }"></span>
      </button>
    </div>

    <div v-if="mixedNote" class="scheme-note">{{ mixedNote }}</div>
  </div>
</template>

<script lang="ts">
import { schemeById } from '../utils/colorHarmony'

// 1 = Shade, 2 = Base, 3 = Both, as in Expression.target.
export type SchemeTarget = 1 | 2 | 3

export function schemeSelection(
  shadeScheme: number,
  baseScheme: number,
  target: SchemeTarget,
): { selectedId: number | null } {
  const shadeId = schemeById(shadeScheme).id
  const baseId = schemeById(baseScheme).id
  if (target === 1) return { selectedId: shadeId }
  if (target === 2) return { selectedId: baseId }
  return { selectedId: shadeId === baseId ? shadeId : null }
}
</script>

<script setup lang="ts">
import { computed, ref } from 'vue'
import SegmentedControl from './SegmentedControl.vue'
import { COLOR_SCHEMES, harmonyColors, type ColorScheme } from '../utils/colorHarmony'
import { createGradientFromHexww } from '../utils/colorUtils'

interface Props {
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
  pick: [target: SchemeTarget, schemeId: number]
}>()

const targetOptions = [
  { value: 1, label: 'Shade' },
  { value: 2, label: 'Base' },
  { value: 3, label: 'Both' },
]

const target = ref<number>(
  schemeSelection(props.shadeScheme, props.baseScheme, 3).selectedId === null ? 1 : 3,
)

const activeTarget = computed<SchemeTarget>(() => {
  if (props.shadeEditable && props.baseEditable) return target.value as SchemeTarget
  return props.baseEditable ? 2 : 1
})

const selectedId = computed(
  () => schemeSelection(props.shadeScheme, props.baseScheme, activeTarget.value).selectedId,
)

const previewPrimary = computed(() =>
  activeTarget.value === 2 ? props.basePrimary : props.shadePrimary,
)

const mixedNote = computed(() =>
  activeTarget.value === 3 && selectedId.value === null
    ? `Shade uses ${schemeById(props.shadeScheme).name}, Base uses ${schemeById(props.baseScheme).name}. Picking a scheme sets both.`
    : '',
)

const swatchBackground = (scheme: ColorScheme) =>
  createGradientFromHexww(
    harmonyColors(previewPrimary.value, scheme.id, scheme.naturalCount),
    'to right',
  )
</script>

<style scoped>
.scheme-picker {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.scheme-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
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
  color: var(--brand-light-grey);
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
  font-size: 0.75rem;
  line-height: 1.4;
  color: var(--brand-slate-grey);
}
</style>
