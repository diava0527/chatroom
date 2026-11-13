<template>
  <view class="chat">
    <view class="chat__shell">
      <ChatSidebar
        :users="state.users"
        :nickname="state.nickname"
        :status="state.status"
        :connected="state.connected"
        :online-count="state.onlineCount"
        :lobby-unread="state.lobbyUnread"
        :current-partner="state.currentPartner"
        :open="sidebarOpen"
        :logging-out="loggingOut"
        @lobby="showLobby"
        @select="openPrivate"
        @logout="logout"
        @close="sidebarOpen = false"
      />

      <view v-if="sidebarOpen && !isWide" class="chat__mask" @click="sidebarOpen = false" />

      <view class="main">
        <view class="head" :style="{ paddingTop: statusBarHeight + 'px' }">
          <view class="head__row">
            <button
              v-if="!isWide"
              class="head__menu"
              hover-class="head__menu--pressed"
              :hover-stay-time="80"
              @click="sidebarOpen = true"
            >
              <view class="menu-icon">
                <view class="menu-icon__line" />
                <view class="menu-icon__line" />
                <view class="menu-icon__line" />
              </view>
            </button>

            <view class="head__text">
              <view class="head__title-row">
                <text class="head__title">{{ state.title }}</text>
                <text v-if="state.currentPartner" class="tag">私聊</text>
              </view>
              <view class="head__meta" @click="retry">
                <view class="dot" :class="{ 'dot--online': state.connected }" />
                <text class="head__status">{{ state.status }}</text>
                <text v-if="state.currentPartner && state.lobbyUnread" class="head__unread">
                  大厅 {{ state.lobbyUnread }} 条未读
                </text>
              </view>
            </view>

            <view v-if="!state.currentPartner" class="head__count">{{ state.onlineCount }} 人在线</view>
            <button
              v-else
              class="head__back"
              hover-class="head__back--pressed"
              :hover-stay-time="80"
              @click="showLobby"
            >
              返回大厅
            </button>
          </view>
        </view>

        <scroll-view
          class="messages"
          scroll-y
          :scroll-into-view="state.scrollTarget"
          scroll-with-animation
        >
          <EmptyState
            v-if="!state.messages.length"
            :variant="state.currentPartner ? 'private' : 'lobby'"
          />
          <view
            v-for="item in state.messages"
            :id="item.anchor"
            :key="item.messageId"
            class="messages__item"
          >
            <MessageBubble
              :sender-nickname="item.senderNickname"
              :content="item.content"
              :time="item.time"
              :initial="item.initial"
              :mine="item.mine"
            />
          </view>
          <view class="messages__bottom" />
        </scroll-view>

        <ChatComposer
          v-model:draft="draft"
          :connected="state.connected"
          :sending="sending"
          :note="composerNote"
          @send="send"
        />
      </view>
    </view>
  </view>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { onHide, onLoad, onShow, onUnload } from '@dcloudio/uni-app'
import ChatComposer from '../../components/ChatComposer.vue'
import ChatSidebar from '../../components/ChatSidebar.vue'
import EmptyState from '../../components/EmptyState.vue'
import MessageBubble from '../../components/MessageBubble.vue'
import { useChat } from '../../composables/useChat'
import { useViewportWidth } from '../../composables/useViewportWidth'
import { ApiError, clearSession, getSession, request, toast } from '../../services/api'

const statusBarHeight = ref(0)
const sidebarOpen = ref(false)
const draft = ref('')
const sending = ref(false)
const loggingOut = ref(false)

const viewportWidth = useViewportWidth()
const isWide = computed(() => viewportWidth.value >= 900)

const { state, client } = useChat(() => {
  toast(new Error('登录已失效，请重新登录'))
  toLogin()
})

const composerNote = computed(() =>
  state.currentPartner
    ? `${state.nickname} · 私聊窗口在退出登录后结束`
    : `${state.nickname} · 正在公共大厅聊天`,
)

watch(isWide, (wide) => {
  if (wide) sidebarOpen.value = false
})

onLoad(() => {
  statusBarHeight.value = uni.getSystemInfoSync().statusBarHeight || 0
  if (!getSession()) toLogin()
})

onShow(() => {
  if (!getSession()) {
    toLogin()
    return
  }
  client.start()
})

onHide(() => client.stop())

onUnload(() => client.stop())

function toLogin() {
  uni.reLaunch({ url: '/pages/login/index' })
}

function retry() {
  client.retry()
}

function showLobby() {
  client.showLobby()
  sidebarOpen.value = false
}

async function openPrivate(name: string) {
  if (await client.openPrivate(name)) sidebarOpen.value = false
}

async function send() {
  if (sending.value || !draft.value.trim()) return
  const text = draft.value
  sending.value = true
  try {
    await client.send(text)
    if (draft.value === text) draft.value = ''
  } catch (error) {
    toast(error)
  } finally {
    sending.value = false
  }
}

function logout() {
  if (loggingOut.value) return
  uni.showModal({
    title: '退出聊天室',
    content: '退出后，与你有关的私聊窗口将被清空。',
    success: (result) => {
      if (result.confirm) void performLogout()
    },
  })
}

async function performLogout() {
  loggingOut.value = true
  try {
    await request('/api/v1/auth/logout', 'POST', undefined, true)
  } catch (error) {
    if (!(error instanceof ApiError && error.code === 1003)) {
      // 网络失败不能只删本地 session，否则后端的单端登录限制会锁住账号。
      toast(error)
      loggingOut.value = false
      return
    }
  }
  client.stop()
  clearSession()
  toLogin()
}
</script>

<style lang="scss" scoped>
.chat {
  display: flex;
  height: 100vh;
  overflow: hidden;
  background: $color-canvas;
}

.chat__shell {
  position: relative;
  display: flex;
  flex: 1;
  min-width: 0;
  height: 100%;
  background: $color-surface;
}

.chat__mask {
  position: fixed;
  inset: 0;
  z-index: 10;
  background: $color-overlay;
  animation: fade-in $dur-normal $ease-out both;
}

@keyframes fade-in {
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
}

.main {
  display: flex;
  flex: 1;
  min-width: 0;
  flex-direction: column;
  height: 100%;
  background: $color-canvas;
}

.head {
  flex-shrink: 0;
  padding-bottom: $space-sm;
  background: $color-surface;
  border-bottom: 1px solid $color-line;
}

.head__row {
  display: flex;
  align-items: center;
  gap: $space-sm;
  padding: $space-sm $space-md;
}

.head__menu {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 40px;
  flex-shrink: 0;
  border-radius: $radius-sm;
  background: $color-surface-muted;
}

.head__menu--pressed {
  background: $color-surface-sunken;
}

.menu-icon {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.menu-icon__line {
  width: 13px;
  height: 1.5px;
  border-radius: 1px;
  background: $color-text-strong;
}

.head__text {
  flex: 1;
  min-width: 0;
}

.head__title-row {
  display: flex;
  align-items: center;
  gap: 6px;
}

.head__title {
  font-size: $font-subtitle;
  font-weight: $weight-bold;
  color: $color-ink;
  @include truncate;
}

.tag {
  flex-shrink: 0;
  padding: 2px 6px;
  border-radius: $radius-xs;
  background: $color-primary-soft;
  color: $color-primary-strong;
  font-size: 9px;
  font-weight: $weight-semibold;
}

.head__meta {
  display: flex;
  align-items: center;
  gap: 5px;
  margin-top: 4px;
}

.dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: $color-warning;
  animation: soft-pulse 1.6s $ease-in-out infinite;
}

.dot--online {
  background: $color-accent;
  animation: none;
}

.head__status {
  font-size: $font-micro;
  color: $color-faint;
}

.head__unread {
  font-size: $font-micro;
  color: $color-danger;
}

.head__count {
  flex-shrink: 0;
  font-size: $font-micro;
  color: $color-faint;
  padding: 4px 8px;
  border-radius: $radius-pill;
  background: $color-surface-muted;
}

.head__back {
  flex-shrink: 0;
  padding: 0 12px;
  height: 30px;
  line-height: 30px;
  border-radius: $radius-sm;
  background: $color-primary-soft;
  color: $color-primary-strong;
  font-size: $font-caption;
  font-weight: $weight-semibold;
}

.head__back--pressed {
  background: rgba(74, 92, 245, 0.16);
}

.messages {
  flex: 1;
  height: 0;
  min-height: 0;
  width: 100%;
}

.messages__bottom {
  height: $space-md;
}

@include respond-from($bp-desktop) {
  .chat {
    background:
      radial-gradient(circle at 12% 0%, rgba(74, 92, 245, 0.1) 0%, rgba(74, 92, 245, 0) 42%),
      radial-gradient(circle at 88% 100%, rgba(122, 79, 224, 0.1) 0%, rgba(122, 79, 224, 0) 40%),
      $color-canvas;
  }

  .chat__shell {
    flex: 1;
    max-width: $layout-shell-max;
    margin: 0 auto;
    border-left: 1px solid $color-line;
    border-right: 1px solid $color-line;
    box-shadow: $shadow-raised;
  }

  .head__row {
    padding: $space-md $space-lg;
  }

  .head__title {
    font-size: $font-title;
  }

  .messages__bottom {
    height: $space-xl;
  }
}
</style>