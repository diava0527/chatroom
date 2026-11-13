<template>
  <view class="logo" :class="`logo--${size}`">
    <view class="logo__bubble">
      <view class="logo__dot" />
      <view class="logo__dot" />
      <view class="logo__dot" />
    </view>
  </view>
</template>

<script setup lang="ts">
withDefaults(defineProps<{ size?: 'sm' | 'md' | 'lg' }>(), { size: 'md' })
</script>

<style lang="scss" scoped>
.logo {
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  background: linear-gradient(140deg, #5b6bff 0%, $color-primary 46%, #7a4fe0 100%);
  box-shadow: $shadow-brand;

  &--sm {
    width: 32px;
    height: 32px;
    border-radius: 10px;
  }

  &--md {
    width: 42px;
    height: 42px;
    border-radius: 13px;
  }

  &--lg {
    width: 56px;
    height: 56px;
    border-radius: 17px;
  }
}

.logo__bubble {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
  background: rgba(255, 255, 255, 0.94);
  border-radius: 499.5px 499.5px 499.5px 4px;

  .logo--sm & {
    width: 18px;
    height: 14px;
    padding-bottom: 2px;
  }

  .logo--md & {
    width: 24px;
    height: 18px;
    padding-bottom: 2px;
  }

  .logo--lg & {
    width: 31px;
    height: 23px;
    padding-bottom: 3px;
  }
}

.logo__dot {
  border-radius: 50%;
  background: $color-primary;

  .logo--sm & {
    width: 2.5px;
    height: 2.5px;
  }

  .logo--md & {
    width: 3.5px;
    height: 3.5px;
  }

  .logo--lg & {
    width: 4.5px;
    height: 4.5px;
  }
}
</style>