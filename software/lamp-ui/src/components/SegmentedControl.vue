<script setup lang="ts" generic="T extends number | number[]">
import { computed } from 'vue'

const props = defineProps<{
  modelValue: T
  options: { value: number; label: string }[]
  disabled?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: T]
}>()

const multiple = computed(() => Array.isArray(props.modelValue))

const isActive = (value: number) =>
  Array.isArray(props.modelValue) ? props.modelValue.includes(value) : props.modelValue === value

const select = (value: number) => {
  if (!Array.isArray(props.modelValue)) {
    emit('update:modelValue', value as T)
    return
  }
  const next = props.modelValue.includes(value)
    ? props.modelValue.filter((selected) => selected !== value)
    : [...props.modelValue, value]
  emit('update:modelValue', next as T)
}
</script>

<template>
  <div class="targets">
    <button
      v-for="opt in options"
      :key="opt.value"
      type="button"
      class="target"
      :class="{ active: isActive(opt.value) }"
      :aria-pressed="multiple ? isActive(opt.value) : undefined"
      :disabled="disabled"
      @click="select(opt.value)"
    >
      {{ opt.label }}
    </button>
  </div>
</template>

<style scoped>
.targets {
  display: flex;
  gap: 8px;
}

.target {
  flex: 1;
  padding: 8px 12px;
  background: rgba(255, 255, 255, 0.05);
  color: var(--brand-light-grey);
  border: 1px solid rgba(255, 255, 255, 0.2);
  border-radius: 4px;
  cursor: pointer;
}

.target.active {
  background: rgba(64, 176, 0, 0.2);
  color: var(--brand-lumen-green);
  border-color: var(--brand-lumen-green);
}

.target:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}
</style>
