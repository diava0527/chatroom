<template>
  <view class="row" :class="{ 'row--self': mine }">
    <AppAvatar :initial="initial" :tone="mine ? 'me' : 'other'" size="sm" />
    <view class="row__main">
      <view class="row__meta">
        <text class="row__name">{{ mine ? '我' : senderNickname }}</text>
        <text class="row__time">{{ shortTime }}</text>
      </view>
      <view class="bubble">
        <text class="bubble__text" selectable>{{ content }}</text>
      </view>
    </view>
  </view>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import AppAvatar from './AppAvatar.vue'

const props = defineProps<{
  senderNickname: string
  content: string
  time: string
  initial: string
  mine: boolean
}>()

// 展示层只保留到分钟，避免气泡信息过密。
const shortTime = computed(() => (props.time ? props.time.slice(0, 5) : ''))
</script>

<style lang="scss" scoped>
.row {
  display: flex;
  align-items: flex-start;
  padding: $space-md $space-md 0;
  animation: fade-up $dur-normal $ease-out both;
}

.row--self {
  flex-direction: row-reverse;
}

.row__main {
  display: flex;
  flex-direction: column;
  max-width: 74%;
  min-width: 0;
  margin-left: $space-sm;

  .row--self & {
    margin-left: 0;
    margin-right: $space-sm;
    align-items: flex-end;
  }
}

.row__meta {
  display: flex;
  align-items: baseline;
  gap: 6px;
  margin-bottom: 5px;
  padding: 0 2px;
}

.row__name {
  font-size: $font-micro;
  font-weight: $weight-medium;
  color: $color-faint;
}

.row__time {
  font-size: $font-micro;
  color: #b3bccb;
}

.bubble {
  padding: 10px 13px;
  background: $color-surface;
  border: 1px solid $color-line;
  border-radius: 3px $radius-lg $radius-lg $radius-lg;
  box-shadow: 0 2px 8px rgba(23, 32, 71, 0.04);
  transition:
    transform $dur-fast $ease-out,
    box-shadow $dur-fast $ease-out;

  .row--self & {
    background: linear-gradient(135deg, #5b6bff 0%, $color-primary 100%);
    border-color: transparent;
    border-radius: $radius-lg 3px $radius-lg $radius-lg;
    box-shadow: 0 4px 12px rgba(74, 92, 245, 0.22);
  }
}

.bubble__text {
  font-size: $font-body;
  line-height: $leading-normal;
  color: $color-text-strong;
  word-break: break-word;
  white-space: pre-wrap;

  .row--self & {
    color: #fff;
  }
}
</style>