<template>
  <view class="empty">
    <view class="empty__orb">
      <view class="empty__ring" />
      <view class="empty__glyph">{{ isPrivate ? '❝' : '#' }}</view>
    </view>
    <text class="empty__title">{{ title }}</text>
    <text class="empty__caption">{{ caption }}</text>
  </view>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = withDefaults(
  defineProps<{ variant?: 'lobby' | 'private'; title?: string; caption?: string }>(),
  { variant: 'lobby', title: '', caption: '' },
)

const isPrivate = computed(() => props.variant === 'private')

const title = computed(() => {
  if (props.title) return props.title
  return isPrivate.value ? '你们的对话，从这里开始' : '你好，欢迎来到聊天室'
})

const caption = computed(() => {
  if (props.caption) return props.caption
  return isPrivate.value ? '发送一条消息，打个招呼吧' : '大厅仅显示进入之后的消息，来聊聊吧'
})
</script>

<style lang="scss" scoped>
.empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 70px 24px 40px;
  text-align: center;
  animation: fade-up $dur-slow $ease-out both;
}

.empty__orb {
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 64px;
  height: 64px;
  border-radius: 20px;
  background: linear-gradient(150deg, rgba(74, 92, 245, 0.14) 0%, rgba(122, 79, 224, 0.1) 100%);
  margin-bottom: $space-xl;
}

.empty__ring {
  position: absolute;
  inset: -6px;
  border-radius: 24px;
  border: 1px dashed rgba(74, 92, 245, 0.24);
}

.empty__glyph {
  font-size: 24px;
  font-weight: $weight-bold;
  color: $color-primary;
}

.empty__title {
  font-size: $font-subtitle;
  font-weight: $weight-semibold;
  color: $color-text-strong;
}

.empty__caption {
  margin-top: 7px;
  font-size: $font-caption;
  color: $color-faint;
  line-height: $leading-relaxed;
}
</style>