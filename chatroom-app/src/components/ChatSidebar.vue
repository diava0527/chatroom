<template>
  <view class="side" :class="{ 'side--open': open }">
    <view class="side__top">
      <view class="brand">
        <AppLogo size="sm" />
        <view class="brand__text">
          <text class="brand__name">聊天室</text>
          <text class="brand__sub">{{ onlineCount }} 人在线</text>
        </view>
      </view>
      <button class="side__close" hover-class="side__close--pressed" @click="emit('close')">×</button>
    </view>

    <view class="me">
      <AppAvatar :initial="initial" tone="brand" size="md" />
      <view class="me__text">
        <text class="me__name">{{ nickname }}</text>
        <view class="me__status">
          <view class="me__dot" :class="{ 'is-online': connected }" />
          <text>{{ status }}</text>
        </view>
      </view>
    </view>

    <scroll-view class="side__list" scroll-y>
      <view
        class="entry"
        :class="{ 'entry--active': !currentPartner }"
        hover-class="entry--pressed"
        :hover-stay-time="80"
        @click="emit('lobby')"
      >
        <view class="entry__icon">#</view>
        <view class="entry__body">
          <text class="entry__name">公共大厅</text>
          <text class="entry__sub">所有在线成员</text>
        </view>
        <text v-if="lobbyUnread" class="badge">{{ lobbyBadge }}</text>
      </view>

      <view class="group">
        <text class="group__label">成员</text>
        <text class="group__count">{{ onlineCount }}</text>
      </view>

      <view
        v-for="user in users"
        :key="user.nickname"
        class="entry"
        :class="{ 'entry--active': currentPartner === user.nickname, 'entry--offline': !user.online }"
        hover-class="entry--pressed"
        :hover-stay-time="80"
        @click="emit('select', user.nickname)"
      >
        <AppAvatar :initial="user.initial" :online="user.online" size="sm" />
        <view class="entry__body">
          <text class="entry__name">{{ user.nickname }}</text>
          <text class="entry__sub">{{ user.online ? '在线' : '离线' }}</text>
        </view>
        <text v-if="user.unread" class="badge">{{ badge(user.unread) }}</text>
      </view>

      <view v-if="!users.length" class="side__empty">暂时没有其他成员，邀请朋友来聊聊吧</view>
    </scroll-view>

    <view class="side__foot">
      <button
        class="logout"
        :class="{ 'is-busy': loggingOut }"
        hover-class="logout--pressed"
        :hover-stay-time="80"
        @click="emit('logout')"
      >
        {{ loggingOut ? '正在退出…' : '退出登录' }}
      </button>
    </view>
  </view>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import AppAvatar from './AppAvatar.vue'
import AppLogo from './AppLogo.vue'

interface SidebarUser {
  nickname: string
  initial: string
  online: boolean
  unread: number
}

const props = defineProps<{
  users: SidebarUser[]
  nickname: string
  status: string
  connected: boolean
  onlineCount: number
  lobbyUnread: number
  currentPartner: string
  open: boolean
  loggingOut: boolean
}>()

const emit = defineEmits<{
  select: [nickname: string]
  lobby: []
  logout: []
  close: []
}>()

const initial = computed(() => props.nickname.slice(0, 1))
const lobbyBadge = computed(() => badge(props.lobbyUnread))

function badge(value: number): string {
  return value > 99 ? '99+' : String(value)
}
</script>

<style lang="scss" scoped>
.side {
  position: fixed;
  top: 0;
  bottom: 0;
  left: 0;
  z-index: 20;
  display: flex;
  flex-direction: column;
  width: 76vw;
  max-width: 300px;
  height: 100%;
  background: $color-surface;
  padding-bottom: env(safe-area-inset-bottom);
  transform: translate3d(-104%, 0, 0);
  transition: transform $dur-slow $ease-out;
  box-shadow: 12px 0 32px rgba(19, 26, 44, 0.14);
}

.side--open {
  transform: translate3d(0, 0, 0);
}

@include respond-from($bp-desktop) {
  .side {
    position: relative;
    top: auto;
    bottom: auto;
    left: auto;
    z-index: 1;
    width: $layout-sidebar;
    max-width: none;
    flex-shrink: 0;
    padding-bottom: 0;
    transform: none;
    transition: none;
    box-shadow: none;
    border-right: 1px solid $color-line;
  }
}

.side__top {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: $space-lg $space-md $space-sm;
}

.brand {
  display: flex;
  align-items: center;
  gap: $space-xs;
}

.brand__text {
  display: flex;
  flex-direction: column;
}

.brand__name {
  font-size: $font-body-lg;
  font-weight: $weight-bold;
  color: $color-ink;
  letter-spacing: 0.3px;
}

.brand__sub {
  margin-top: 2px;
  font-size: $font-micro;
  color: $color-faint;
}

.side__close {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  border-radius: 50%;
  background: $color-surface-muted;
  color: $color-muted;
  font-size: 17px;
  line-height: 1;
}

.side__close--pressed {
  background: $color-surface-sunken;
}

.me {
  display: flex;
  align-items: center;
  gap: $space-sm;
  margin: $space-xs $space-md $space-md;
  padding: $space-sm $space-md;
  border-radius: $radius-lg;
  background: linear-gradient(140deg, #f7f9ff 0%, #eef2ff 100%);
  border: 1px solid rgba(74, 92, 245, 0.08);
}

.me__text {
  display: flex;
  flex-direction: column;
  min-width: 0;
}

.me__name {
  font-size: $font-body;
  font-weight: $weight-semibold;
  color: $color-text-strong;
  @include truncate;
}

.me__status {
  display: flex;
  align-items: center;
  gap: 5px;
  margin-top: 3px;
  font-size: $font-micro;
  color: $color-faint;
}

.me__dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: $color-warning;
  animation: soft-pulse 1.6s $ease-in-out infinite;

  &.is-online {
    background: $color-accent;
    animation: none;
  }
}

.side__list {
  flex: 1;
  height: 0;
  min-height: 0;
  padding: 0 $space-md;
}

.entry {
  display: flex;
  align-items: center;
  gap: $space-sm;
  padding: $space-sm;
  margin-bottom: 4px;
  border-radius: $radius-md;
  transition:
    background-color $dur-fast $ease-out,
    transform $dur-fast $ease-out;
}

.entry--active {
  background: $color-primary-ghost;
  box-shadow: inset 0 0 0 1px rgba(74, 92, 245, 0.16);
}

.entry--offline {
  opacity: 0.55;
}

.entry--pressed {
  transform: scale(0.985);
  background: $color-surface-muted;
}

.entry__icon {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  flex-shrink: 0;
  border-radius: 10px;
  background: linear-gradient(150deg, #222c46 0%, $color-ink 100%);
  color: #fff;
  font-size: 14px;
  font-weight: $weight-semibold;
}

.entry__body {
  display: flex;
  flex-direction: column;
  flex: 1;
  min-width: 0;
}

.entry__name {
  font-size: $font-body;
  font-weight: $weight-medium;
  color: $color-text-strong;
  @include truncate;
}

.entry__sub {
  margin-top: 2px;
  font-size: $font-micro;
  color: $color-faint;
}

.group {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: $space-md 4px 6px;
}

.group__label {
  font-size: $font-micro;
  font-weight: $weight-semibold;
  letter-spacing: 1px;
  color: $color-faint;
  text-transform: uppercase;
}

.group__count {
  font-size: $font-micro;
  color: $color-faint;
  background: $color-surface-muted;
  border-radius: $radius-pill;
  padding: 1px 7px;
}

.badge {
  flex-shrink: 0;
  min-width: 18px;
  padding: 2px 5px;
  border-radius: $radius-pill;
  background: $color-danger;
  color: #fff;
  font-size: 9px;
  font-weight: $weight-semibold;
  text-align: center;
  line-height: 1.3;
}

.side__empty {
  padding: $space-lg $space-xs;
  font-size: $font-caption;
  color: $color-faint;
  line-height: $leading-relaxed;
  text-align: center;
}

.side__foot {
  padding: $space-sm $space-md $space-md;
  border-top: 1px solid $color-line;
}

.logout {
  width: 100%;
  height: 36px;
  border-radius: $radius-md;
  background: $color-surface-muted;
  color: $color-muted;
  font-size: $font-caption;
  font-weight: $weight-medium;
  transition: background-color $dur-fast $ease-out;
}

.logout--pressed {
  background: $color-danger-soft;
  color: $color-danger;
}

.logout.is-busy {
  opacity: 0.6;
}

@include respond-from($bp-desktop) {
  .side__close {
    display: none;
  }
}
</style>