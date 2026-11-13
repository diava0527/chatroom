<template>
  <button
    class="app-btn"
    :class="[`app-btn--${variant}`, `app-btn--${size}`, { 'app-btn--block': block, 'app-btn--disabled': isLocked }]"
    :disabled="isLocked"
    hover-class="app-btn--pressed"
    :hover-stay-time="80"
    @click="onClick"
  >
    <view v-if="loading" class="app-btn__spinner" />
    <text class="app-btn__label"><slot /></text>
  </button>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = withDefaults(
  defineProps<{
    variant?: 'primary' | 'secondary' | 'ghost' | 'danger'
    size?: 'sm' | 'md' | 'lg'
    block?: boolean
    loading?: boolean
    disabled?: boolean
  }>(),
  { variant: 'primary', size: 'md', block: false, loading: false, disabled: false },
)

const emit = defineEmits<{ click: [] }>()

const isLocked = computed(() => props.disabled || props.loading)

function onClick() {
  if (isLocked.value) return
  emit('click')
}
</script>

<style lang="scss" scoped>
.app-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  border-radius: $radius-md;
  font-weight: $weight-semibold;
  letter-spacing: 0.3px;
  transition:
    transform $dur-fast $ease-out,
    box-shadow $dur-fast $ease-out,
    background-color $dur-fast $ease-out,
    opacity $dur-fast $ease-out;
}

.app-btn--sm {
  height: 32px;
  padding: 0 12px;
  font-size: $font-caption;
  border-radius: $radius-sm;
}

.app-btn--md {
  height: 42px;
  padding: 0 16px;
  font-size: $font-body;
}

.app-btn--lg {
  height: 48px;
  padding: 0 20px;
  font-size: $font-body-lg;
  border-radius: $radius-lg;
}

.app-btn--block {
  display: flex;
  width: 100%;
}

.app-btn--primary {
  color: $color-primary-contrast;
  background: linear-gradient(135deg, #5b6bff 0%, $color-primary 52%, #6d4ce0 100%);
  box-shadow: $shadow-brand;
}

.app-btn--secondary {
  color: $color-primary-strong;
  background: $color-primary-soft;
}

.app-btn--ghost {
  color: $color-muted;
  background: transparent;
  border: 1px solid $color-line-strong;
}

.app-btn--danger {
  color: $color-danger;
  background: $color-danger-soft;
}

.app-btn--disabled {
  opacity: 0.55;
  box-shadow: none;
}

.app-btn--pressed {
  transform: scale(0.97);
  opacity: 0.92;
}

.app-btn__label {
  line-height: 1;
}

.app-btn__spinner {
  width: 13px;
  height: 13px;
  border-radius: 50%;
  border: 1.5px solid rgba(255, 255, 255, 0.45);
  border-top-color: #fff;
  animation: spin 0.7s linear infinite;
}

.app-btn--secondary .app-btn__spinner,
.app-btn--ghost .app-btn__spinner,
.app-btn--danger .app-btn__spinner {
  border-color: rgba(74, 92, 245, 0.24);
  border-top-color: $color-primary;
}
</style>