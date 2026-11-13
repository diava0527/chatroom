<template>
  <view class="field" :class="{ 'is-focused': focused, 'is-disabled': disabled, 'is-invalid': !!error }">
    <text v-if="label" class="field__label">{{ label }}</text>
    <view class="field__box">
      <input
        class="field__input"
        :value="modelValue"
        :password="password"
        :disabled="disabled"
        :maxlength="maxlength"
        :placeholder="placeholder"
        placeholder-class="field-ph"
        :confirm-type="confirmType"
        :cursor-spacing="24"
        @input="handleInput"
        @focus="focused = true"
        @blur="focused = false"
        @confirm="emit('confirm')"
      />
      <text v-if="suffix" class="field__suffix" @click="emit('suffix-click')">{{ suffix }}</text>
    </view>
    <text v-if="error" class="field__error">{{ error }}</text>
  </view>
</template>

<script setup lang="ts">
import { ref } from 'vue'

withDefaults(
  defineProps<{
    modelValue: string
    label?: string
    placeholder?: string
    password?: boolean
    disabled?: boolean
    maxlength?: number
    confirmType?: 'done' | 'send' | 'search' | 'next' | 'go'
    error?: string
    suffix?: string
  }>(),
  { label: '', placeholder: '', password: false, disabled: false, maxlength: 140, confirmType: 'done', error: '', suffix: '' },
)

const emit = defineEmits<{
  'update:modelValue': [value: string]
  confirm: []
  'suffix-click': []
}>()

const focused = ref(false)

function handleInput(event: { detail: { value: string } }) {
  emit('update:modelValue', event.detail.value)
}
</script>

<style lang="scss" scoped>
.field {
  display: flex;
  flex-direction: column;
}

.field__label {
  font-size: $font-caption;
  font-weight: $weight-medium;
  color: $color-muted;
  margin-bottom: 6px;
  letter-spacing: 0.2px;
}

.field__box {
  display: flex;
  align-items: center;
  height: 44px;
  padding: 0 12px;
  border-radius: $radius-md;
  background: $color-surface-muted;
  border: 1px solid transparent;
  transition:
    border-color $dur-fast $ease-out,
    background-color $dur-fast $ease-out,
    box-shadow $dur-fast $ease-out;

  .is-focused & {
    background: $color-surface;
    border-color: rgba(74, 92, 245, 0.45);
    box-shadow: 0 0 0 3px rgba(74, 92, 245, 0.08);
  }

  .is-invalid & {
    border-color: rgba(226, 96, 107, 0.5);
    background: $color-danger-soft;
  }

  .is-disabled & {
    opacity: 0.6;
  }
}

.field__input {
  flex: 1;
  min-width: 0;
  height: 100%;
  font-size: $font-body;
  color: $color-text-strong;
}

.field__suffix {
  margin-left: 8px;
  font-size: $font-caption;
  color: $color-primary;
}

.field__error {
  margin-top: 5px;
  font-size: $font-micro;
  color: $color-danger;
}

.field + .field {
  margin-top: $space-md;
}
</style>