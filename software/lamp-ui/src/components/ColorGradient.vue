<template>
  <div class="color-gradient">
    <div v-if="localColors.length > 1" class="gradient-preview-container">
      <div class="gradient-preview" :style="gradientStyle"></div>
      <span class="indicator-right">Top</span>
    </div>
    <div class="color-list">
      <template v-for="(storageIndex, row) in rowIndices" :key="storageIndex">
        <div class="color-item">
          <div class="color-swatch-group">
            <ColorPicker
              v-model="localColors[storageIndex]"
              @update:model-value="updateColor(storageIndex, $event)"
              @open="emit('edit-session', true)"
              @close="emit('edit-session', false)"
              :disabled="disabled || isDerived(storageIndex)"
            />
          </div>

          <div class="color-actions">
            <IconButton
              v-if="localColors.length < props.maxColors"
              icon="clone"
              variant="clone"
              title="Clone color"
              :disabled="disabled || autoColors"
              @click="cloneColor(storageIndex)"
            />

            <IconButton
              v-if="localColors.length > 1 && (!autoColors || isDerived(storageIndex))"
              icon="remove"
              variant="remove"
              title="Remove color"
              :disabled="disabled"
              @click="removeColor(storageIndex)"
            />
          </div>
        </div>

        <div v-if="row === 0" class="auto-pick-row">
          <div class="auto-pick-header">
            <span class="auto-pick-label">Auto-pick colors</span>
            <BooleanInput
              :model-value="autoColors"
              :disabled="disabled"
              @update:model-value="setAutoColors"
            />
          </div>
          <div class="auto-pick-help">
            <template v-if="autoColors && primaryIsNeutral">
              Pick a more colorful primary to vary the other colors.
            </template>
            <template v-else>
              {{ schemeName }} &middot;
              <button type="button" class="auto-pick-link" @click="emit('open-scheme-settings')">change in Setup</button>
            </template>
          </div>
        </div>
      </template>
    </div>

    <div v-if="props.showAddButton" class="add-button-container">
      <IconButton
        icon="plus"
        variant="plus"
        title="Add color"
        :disabled="localColors.length >= props.maxColors || disabled"
        @click="addColor"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, computed } from 'vue'
import ColorPicker from './ColorPicker.vue'
import IconButton from './IconButton.vue'
import BooleanInput from './BooleanInput.vue'
import { createGradientFromHexww, parseHexww, rgbToHsv } from '../utils/colorUtils'
import { harmonizePalette, schemeById } from '../utils/colorHarmony'

interface Props {
  modelValue: string[]
  showAddButton?: boolean
  maxColors?: number
  disabled?: boolean
  autoColors?: boolean
  scheme?: number
}

const props = withDefaults(defineProps<Props>(), {
  modelValue: () => ['#FF000000'],
  showAddButton: true,
  maxColors: 5,
  disabled: false,
  autoColors: false,
  scheme: 0,
})

const emit = defineEmits<{
  'update:modelValue': [value: string[]]
  'edit-session': [open: boolean]
  'update:autoColors': [value: boolean]
  'open-scheme-settings': []
}>()

const localColors = ref<string[]>([...props.modelValue])

// Ensure we always have at least one color
if (localColors.value.length === 0) {
  localColors.value = ['#FF000000']
}

const gradientStyle = computed(() => {
  if (localColors.value.length <= 1) return {}

  return {
    background: createGradientFromHexww(localColors.value, 'to right'),
  }
})

const rowIndices = computed(() =>
  localColors.value.map((_, row) => localColors.value.length - 1 - row),
)

const isDerived = (storageIndex: number) =>
  props.autoColors && storageIndex !== localColors.value.length - 1

const schemeName = computed(() => schemeById(props.scheme).name)

const primaryIsNeutral = computed(() => {
  const { red, green, blue } = parseHexww(localColors.value[localColors.value.length - 1])
  const { s, v } = rgbToHsv(red, green, blue)
  return s === 0 || v === 0
})

const regenerate = (count = localColors.value.length) => {
  localColors.value = harmonizePalette(localColors.value, props.scheme, count)
}

const setAutoColors = (value: boolean) => {
  emit('update:autoColors', value)
  if (!value) return
  regenerate(
    localColors.value.length === 1
      ? schemeById(props.scheme).naturalCount
      : localColors.value.length,
  )
  emit('update:modelValue', [...localColors.value])
}

const updateColor = (index: number, value: string) => {
  localColors.value[index] = value
  if (props.autoColors) regenerate()
  emit('update:modelValue', [...localColors.value])
}

const addColor = () => {
  if (props.autoColors) {
    regenerate(localColors.value.length + 1)
    emit('update:modelValue', [...localColors.value])
    return
  }
  const firstColor = localColors.value[0] || '#FF000000'
  localColors.value.unshift(firstColor)
  emit('update:modelValue', [...localColors.value])
}

const removeColor = (index: number) => {
  if (localColors.value.length > 1) {
    if (props.autoColors) regenerate(localColors.value.length - 1)
    else localColors.value.splice(index, 1)
    emit('update:modelValue', [...localColors.value])
  }
}

const cloneColor = (index: number) => {
  if (localColors.value.length < props.maxColors) {
    const colorToClone = localColors.value[index]
    localColors.value.splice(index, 0, colorToClone)
    emit('update:modelValue', [...localColors.value])
  }
}

// Watch for external changes
watch(
  () => props.modelValue,
  (newValue) => {
    if (newValue && newValue.length > 0) {
      localColors.value = [...newValue]
    }
  },
  { deep: true },
)
</script>

<style scoped>
.color-gradient {
  width: 100%;
  position: relative;
}

.gradient-preview-container {
  position: relative;
  margin-bottom: 12px;
  display: flex;
  align-items: center;
  gap: 8px;
}

.gradient-preview {
  flex: 1;
  height: 20px;
  border-radius: 2px;
  pointer-events: none;
}

.indicator-right {
  font-size: 10px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  writing-mode: vertical-rl;
  text-orientation: mixed;
  color: #efa3c8;
}

.color-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
  margin-bottom: 12px;
}

.color-item {
  display: flex;
  align-items: center;
  gap: 12px;
  justify-content: space-between;
}

.color-swatch-group {
  display: flex;
  align-items: center;
  gap: 4px;
  flex: 1;
}

.color-actions {
  display: flex;
  align-items: center;
  gap: 8px;
}

.auto-pick-row {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 10px 8px;
  background: rgba(253, 253, 253, 0.02);
  border-radius: 6px;
}

.auto-pick-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.auto-pick-header :deep(.boolean-input-container) {
  width: auto;
}

.auto-pick-label {
  font-size: 0.9rem;
  font-weight: 500;
  color: var(--brand-fog-grey);
}

.auto-pick-help {
  font-size: 0.75rem;
  line-height: 1.4;
  color: var(--brand-slate-grey);
}

.auto-pick-link {
  padding: 0;
  border: none;
  background: none;
  font: inherit;
  color: var(--brand-glow-pink);
  text-decoration: underline;
  cursor: pointer;
}

.add-button-container {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-top: 8px;
}

/* Mobile optimizations */
@media (max-width: 480px) {
  .color-list {
    gap: 10px;
  }
}
</style>
