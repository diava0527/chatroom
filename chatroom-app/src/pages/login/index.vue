<template>
  <view class="auth" :style="{ paddingTop: statusBarHeight + 'px' }">
    <view class="auth__glow auth__glow--primary" />
    <view class="auth__glow auth__glow--violet" />

    <view class="auth__grid">
      <view class="intro">
        <AppLogo size="lg" />
        <text class="intro__brand">聊天室</text>
        <view class="intro__chips">
          <view class="chip">
            <view class="chip__dot" />
            <text class="chip__text">公共大厅</text>
          </view>
          <view class="chip">
            <view class="chip__dot chip__dot--accent" />
            <text class="chip__text">一对一私聊</text>
          </view>
          <view class="chip">
            <view class="chip__dot chip__dot--warning" />
            <text class="chip__text">多端同步</text>
          </view>
        </view>
      </view>

      <view class="panel">
        <view class="tabs">
          <view class="tabs__item" :class="{ 'is-active': !registering }" @click="switchMode(false)">登录</view>
          <view class="tabs__item" :class="{ 'is-active': registering }" @click="switchMode(true)">注册</view>
          <view class="tabs__slider" :class="{ 'is-right': registering }" />
        </view>

        <text class="panel__title">{{ registering ? '创建你的昵称' : '欢迎回来' }}</text>
        <text class="panel__sub">{{
          registering ? '注册后即可进入大厅，和在线的人打招呼' : '使用聊天室昵称和密码登录'
        }}</text>

        <view class="panel__form">
          <AppField
            v-model="nickname"
            label="昵称"
            placeholder="你的昵称"
            :maxlength="32"
            :disabled="busy"
          />
          <AppField
            v-model="password"
            label="密码"
            placeholder="输入密码"
            password
            :maxlength="128"
            :disabled="busy"
            confirm-type="done"
            @confirm="submit"
          />
          <AppField
            v-if="registering"
            v-model="confirmPassword"
            label="确认密码"
            placeholder="再次输入密码"
            password
            :maxlength="128"
            :disabled="busy"
            @confirm="submit"
          />
        </view>

        <view v-if="error" class="alert">{{ error }}</view>

        <AppButton block size="lg" :loading="busy" :disabled="checking" @click="submit">
          {{ registering ? '注册并进入聊天室' : '进入聊天室' }}
        </AppButton>

        <view v-if="develop" class="dev">
          <view class="dev__toggle" @click="toggleSettings">
            <text class="dev__toggle-label">开发连接设置</text>
            <text class="dev__toggle-icon">{{ showSettings ? '−' : '+' }}</text>
          </view>
          <view v-if="showSettings" class="dev__body">
            <AppField
              v-model="server"
              label="后端服务器地址"
              :placeholder="serverPlaceholder"
              :maxlength="200"
              :disabled="busy || checking"
            />
            <text class="dev__help">{{ serverHelp }}</text>
            <AppButton
              variant="secondary"
              size="sm"
              block
              :loading="checking"
              :disabled="busy"
              @click="checkServer"
            >
              保存并检测连接
            </AppButton>
            <text v-if="health" class="dev__health">{{ health }}</text>
          </view>
        </view>
      </view>
    </view>
  </view>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { onLoad } from '@dcloudio/uni-app'
import AppButton from '../../components/AppButton.vue'
import AppField from '../../components/AppField.vue'
import AppLogo from '../../components/AppLogo.vue'
import { errorMessage, getSession, request, saveSession, toast } from '../../services/api'
import { getApiBase, getDevServer, isDevelop, isH5, setDevServer } from '../../services/config'

const registering = ref(false)
const nickname = ref('')
const password = ref('')
const confirmPassword = ref('')
const busy = ref(false)
const error = ref('')
const develop = ref(false)
const showSettings = ref(false)
const server = ref('')
const checking = ref(false)
const health = ref('')
const statusBarHeight = ref(0)

const serverPlaceholder = computed(() =>
  isH5 ? '留空表示与当前站点同源' : 'https://chat.example.com',
)

const serverHelp = computed(() =>
  isH5
    ? 'Web 端开发默认由 /api 代理转发到 127.0.0.1:8080；填写完整地址可直连其它后端（需允许跨域）。生产环境建议由反向代理提供同源 HTTPS/WSS。'
    : '模拟器用 127.0.0.1。手机调试需先让后端可被手机访问，再填写服务器地址；当前后端仅监听本机。正式版需配置 HTTPS/WSS 合法域名。',
)

onLoad(() => {
  statusBarHeight.value = uni.getSystemInfoSync().statusBarHeight || 0
  try {
    develop.value = isDevelop()
    // 正式版地址缺失时在这里直接给出提示。
    getApiBase()
    server.value = getDevServer()
    if (getSession()) uni.redirectTo({ url: '/pages/chat/index' })
  } catch (err) {
    error.value = errorMessage(err)
  }
})

function switchMode(target: boolean) {
  if (busy.value || registering.value === target) return
  registering.value = target
  password.value = ''
  confirmPassword.value = ''
  error.value = ''
}

function toggleSettings() {
  showSettings.value = !showSettings.value
}

async function checkServer() {
  if (checking.value || busy.value) return
  checking.value = true
  health.value = ''
  try {
    setDevServer(server.value)
    await request('/api/v1/health')
    health.value = '连接成功，后端服务正常'
    server.value = getDevServer()
  } catch (err) {
    health.value = errorMessage(err)
  } finally {
    checking.value = false
  }
}

async function submit() {
  if (busy.value || checking.value) return
  const name = nickname.value.trim()
  if (!name || !password.value) {
    error.value = '请输入昵称和密码'
    return
  }
  if (registering.value && password.value !== confirmPassword.value) {
    error.value = '两次输入的密码不一致'
    return
  }
  busy.value = true
  error.value = ''
  try {
    if (registering.value) {
      await request('/api/v1/auth/register', 'POST', { nickname: name, password: password.value })
      registering.value = false
      confirmPassword.value = ''
      uni.showToast({ title: '注册成功', icon: 'success' })
    }
    const result = await request<{ nickname: string; sessionId: string }>(
      '/api/v1/auth/login',
      'POST',
      { nickname: name, password: password.value },
    )
    saveSession(result.nickname, result.sessionId)
    password.value = ''
    confirmPassword.value = ''
    uni.redirectTo({ url: '/pages/chat/index', fail: toast })
  } catch (err) {
    error.value = errorMessage(err)
  } finally {
    busy.value = false
  }
}
</script>

<style lang="scss" scoped>
.auth {
  position: relative;
  min-height: 100vh;
  padding-left: 20px;
  padding-right: 20px;
  padding-bottom: calc(#{$space-3xl} + env(safe-area-inset-bottom));
  overflow: hidden;
  background: linear-gradient(180deg, #f7f9ff 0%, $color-canvas 42%, #eef2fb 100%);
}

.auth__glow {
  position: absolute;
  border-radius: 50%;
  pointer-events: none;
}

.auth__glow--primary {
  top: -120px;
  left: -90px;
  width: 310px;
  height: 310px;
  background: radial-gradient(circle, rgba(74, 92, 245, 0.18) 0%, rgba(74, 92, 245, 0) 68%);
}

.auth__glow--violet {
  right: -110px;
  bottom: -60px;
  width: 280px;
  height: 280px;
  background: radial-gradient(circle, rgba(122, 79, 224, 0.14) 0%, rgba(122, 79, 224, 0) 68%);
}

.auth__grid {
  position: relative;
  display: flex;
  flex-direction: column;
  max-width: 1080px;
  margin: 0 auto;
}

.intro {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  padding: $space-3xl 0 $space-xl;
  animation: rise-in $dur-slow $ease-out both;
}

.intro__brand {
  margin-top: $space-lg;
  font-size: $font-display;
  font-weight: $weight-bold;
  letter-spacing: 1px;
  color: $color-ink;
}

.intro__chips {
  display: flex;
  flex-wrap: wrap;
  gap: $space-xs;
  margin-top: $space-lg;
}

.chip {
  display: flex;
  align-items: center;
  gap: 5px;
  padding: 5px 10px;
  border-radius: $radius-pill;
  background: rgba(255, 255, 255, 0.72);
  border: 1px solid rgba(74, 92, 245, 0.1);
}

.chip__dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: $color-primary;
}

.chip__dot--accent {
  background: $color-accent;
}

.chip__dot--warning {
  background: $color-warning;
}

.chip__text {
  font-size: $font-micro;
  color: $color-muted;
  font-weight: $weight-medium;
}

.panel {
  padding: 24px 20px;
  border-radius: $radius-xl;
  background: $color-surface;
  border: 1px solid $color-line;
  box-shadow: $shadow-raised;
  animation: fade-up $dur-slow $ease-out both;
}

.tabs {
  position: relative;
  display: flex;
  padding: 3px;
  border-radius: $radius-md;
  background: $color-surface-muted;
}

.tabs__item {
  position: relative;
  z-index: 1;
  flex: 1;
  text-align: center;
  line-height: 32px;
  font-size: $font-caption;
  font-weight: $weight-semibold;
  color: $color-muted;
  transition: color $dur-fast $ease-out;
}

.tabs__item.is-active {
  color: $color-primary-strong;
}

.tabs__slider {
  position: absolute;
  top: 3px;
  left: 3px;
  width: calc(50% - 3px);
  height: 32px;
  border-radius: $radius-sm;
  background: $color-surface;
  box-shadow: 0 2px 6px rgba(23, 32, 71, 0.08);
  transition: transform $dur-normal $ease-out;
}

.tabs__slider.is-right {
  transform: translate3d(calc(100% - 0px), 0, 0);
}

.panel__title {
  display: block;
  margin-top: $space-lg;
  font-size: $font-title;
  font-weight: $weight-bold;
  color: $color-ink;
}

.panel__sub {
  display: block;
  margin-top: 5px;
  font-size: $font-caption;
  color: $color-faint;
  line-height: $leading-normal;
}

.panel__form {
  margin-top: $space-xl;
}

.alert {
  margin-top: $space-md;
  padding: $space-sm $space-md;
  border-radius: $radius-sm;
  background: $color-danger-soft;
  color: $color-danger;
  font-size: $font-caption;
  line-height: $leading-normal;
  animation: pop-in $dur-fast $ease-out both;
}

.panel :deep(.app-btn) {
  margin-top: $space-lg;
}

.dev {
  margin-top: $space-lg;
  border-top: 1px dashed $color-line-strong;
  padding-top: $space-md;
}

.dev__toggle {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 5px;
  font-size: $font-micro;
  color: $color-faint;
}

.dev__toggle-icon {
  font-size: $font-body;
  color: $color-muted;
}

.dev__body {
  margin-top: $space-md;
  padding: $space-md;
  border-radius: $radius-md;
  background: $color-surface-muted;
  animation: fade-up $dur-fast $ease-out both;
}

.dev__help {
  display: block;
  margin: $space-xs 0 $space-md;
  font-size: $font-micro;
  color: $color-faint;
  line-height: $leading-relaxed;
}

.dev__health {
  display: block;
  margin-top: $space-xs;
  font-size: $font-micro;
  color: $color-accent;
}

@include respond-from($bp-desktop) {
  .auth {
    display: flex;
    align-items: center;
    padding-left: $space-2xl;
    padding-right: $space-2xl;
  }

  .auth__grid {
    display: grid;
    grid-template-columns: minmax(0, 1fr) 460px;
    align-items: center;
    gap: 80px;
    width: 100%;
    min-height: calc(100vh - #{$space-4xl});
  }

  .intro {
    padding: 0;
  }

  .intro__brand {
    font-size: 34px;
  }

  .panel {
    padding: 40px 36px;
    border-radius: 24px;
  }

  .panel__title {
    font-size: 24px;
  }
}
</style>