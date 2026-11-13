<template>
  <view class="avatar" :class="[`avatar--${tone}`, `avatar--${size}`]">
    <text class="avatar__initial">{{ initial }}</text>
    <view v-if="typeof online === 'boolean'" class="avatar__dot" :class="{ 'is-online': online }" />
  </view>
</template>

<script setup lang="ts">
withDefaults(
  defineProps<{
    initial: string
    tone?: 'other' | 'me' | 'brand'
    size?: 'sm' | 'md' | 'lg'
    online?: boolean
  }>(),
  { tone: 'other', size: 'md', online: undefined },
)
</script>

<style lang="scss" scoped>
.avatar {
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  font-weight: $weight-semibold;
  letter-spacing: 1px;
  transition:
    transform $dur-fast $ease-out,
    box-shadow $dur-fast $ease-out;

  &--sm {
    width: 32px;
    height: 32px;
    border-radius: 10px;
    font-size: 12px;
  }

  &--md {
    width: 36px;
    height: 36px;
    border-radius: 12px;
    font-size: 14px;
  }

  &--lg {
    width: 42px;
    height: 42px;
    border-radius: 14px;
    font-size: 16px;
  }

  &--other {
    background: linear-gradient(160deg, #eef1ff 0%, #e3e9fb 100%);
    color: #5566a8;
  }

  &--me {
    background: linear-gradient(150deg, #6d7bff 0%, $color-primary 100%);
    color: #fff;
    box-shadow: 0 4px 10px rgba(74, 92, 245, 0.24);
  }

  &--brand {
    background: linear-gradient(150deg, #222c46 0%, #131a2c 100%);
    color: #fff;
    box-shadow: 0 4px 10px rgba(19, 26, 44, 0.22);
  }
}

.avatar__initial {
  line-height: 1;
}

.avatar__dot {
  position: absolute;
  right: -1px;
  bottom: -1px;
  width: 9px;
  height: 9px;
  border-radius: 50%;
  background: #c3cad8;
  border: 2px solid $color-surface;

  &.is-online {
    background: $color-accent;
  }
}
</style>