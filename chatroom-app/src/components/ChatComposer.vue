<template>
  <view class="composer">
    <view class="composer__box">
      <input
        class="composer__input"
        :value="draft"
        :placeholder="connected ? '说点什么…' : '正在连接，消息可先写在这里…'"
        placeholder-class="field-ph"
        :maxlength="1000"
        confirm-type="send"
        :cursor-spacing="20"
        @input="handleInput"
        @confirm="emit('send')"
      />
      <button
        class="composer__send"
        :class="{ 'is-ready': canSend }"
        :disabled="!canSend"
        hover-class="composer__send--pressed"
        :hover-stay-time="80"
        @click="emit('send')"
      >
        <view v-if="sending" class="composer__spinner" />
        <text v-else class="composer__send-label">发送</text>
      </button>
    </view>
    <text class="composer__note">{{ note }}</text>
  </view>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = defineProps<{
  draft: string
  connected: boolean
  sending: boolean
  note: string
}>()

const emit = defineEmits<{
  'update:draft': [value: string]
  send: []
}>()

const canSend = computed(() => props.connected && !props.sending && props.draft.trim().length > 0)

function handleInput(event: { detail: { value: string } }) {
  emit('update:draft', event.detail.value)
}
</script>

<style lang="scss" scoped>
.composer {
  flex-shrink: 0;
  padding: $space-sm $space-md calc(#{$space-sm} + env(safe-area-inset-bottom));
  background: $color-surface;
  border-top: 1px solid $color-line;
}

.composer__box {
  display: flex;
  align-items: center;
  gap: $space-xs;
  padding: 5px 5px 5px 12px;
  border-radius: $radius-xl;
  background: $color-surface-muted;
  border: 1px solid transparent;
  transition:
    border-color $dur-fast $ease-out,
    background-color $dur-fast $ease-out,
    box-shadow $dur-fast $ease-out;

  &:focus-within {
    background: $color-surface;
    border-color: rgba(74, 92, 245, 0.35);
    box-shadow: 0 0 0 3px rgba(74, 92, 245, 0.07);
  }
}

.composer__input {
  flex: 1;
  min-width: 0;
  height: 34px;
  font-size: $font-body;
  color: $color-text-strong;
}

.composer__send {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 58px;
  height: 36px;
  flex-shrink: 0;
  border-radius: $radius-lg;
  background: $color-surface-sunken;
  color: $color-faint;
  font-size: $font-caption;
  font-weight: $weight-semibold;
  transition:
    background-color $dur-fast $ease-out,
    color $dur-fast $ease-out,
    transform $dur-fast $ease-out;

  &.is-ready {
    background: linear-gradient(135deg, #5b6bff 0%, $color-primary 100%);
    color: #fff;
    box-shadow: 0 3px 9px rgba(74, 92, 245, 0.28);
  }
}

.composer__send--pressed {
  transform: scale(0.95);
}

.composer__spinner {
  width: 12px;
  height: 12px;
  border-radius: 50%;
  border: 1.5px solid rgba(255, 255, 255, 0.5);
  border-top-color: #fff;
  animation: spin 0.7s linear infinite;
}

.composer__note {
  display: block;
  margin-top: 5px;
  text-align: center;
  font-size: 10px;
  color: #b0b8c7;
}
</style>