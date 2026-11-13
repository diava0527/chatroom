/**
 * 服务器地址配置
 * ------------------------------------------------------------------
 * Web 端：默认同源访问（开发期由 Vite 代理转发 /api/v1 与 /ws/chat），
 *         正式发布可用 RELEASE_API_BASE 指向独立域名。
 * 小程序端：开发期默认连本机后端，正式版必须配置 HTTPS/WSS 合法域名。
 */

// 体验版和正式版必须填写已配置为合法域名的公网 HTTPS 地址（Web 端可留空表示同源）。
export const RELEASE_API_BASE = ''
export const LOCAL_API_BASE = 'http://127.0.0.1:8080'

const SERVER_KEY = 'chatroom.devServer'

// 构建期由 uni-app 注入，Web 端为 h5，微信小程序为 mp-weixin。
const PLATFORM = process.env.UNI_PLATFORM || 'h5'

export const isH5 = PLATFORM === 'h5'

interface WxLike {
  getAccountInfoSync: () => { miniProgram: { envVersion: string } }
}

function wxApi(): WxLike | null {
  return (globalThis as unknown as { wx?: WxLike }).wx || null
}

// Web 端开发默认走同源代理；小程序端默认连本机后端。
const DEFAULT_DEV_BASE = isH5 ? '' : LOCAL_API_BASE

export function isDevelop(): boolean {
  const wx = wxApi()
  if (!isH5 && wx) return wx.getAccountInfoSync().miniProgram.envVersion === 'develop'
  return process.env.NODE_ENV !== 'production'
}

export function normalizeServer(value: string): string {
  const base = value.trim().replace(/\/+$/, '')
  // 留空表示与当前站点同源，仅 Web 端可用。
  if (!base && isH5) return ''
  if (!/^https?:\/\/[a-zA-Z0-9.-]+(?::\d{1,5})?$/.test(base)) {
    throw new Error('请输入服务器根地址，例如 https://chat.example.com')
  }
  return base
}

export function getApiBase(): string {
  if (isDevelop()) {
    return (uni.getStorageSync(SERVER_KEY) as string) || DEFAULT_DEV_BASE
  }
  if (isH5) {
    return RELEASE_API_BASE ? normalizeServer(RELEASE_API_BASE) : ''
  }
  if (!RELEASE_API_BASE || !/^https:\/\//.test(RELEASE_API_BASE)) {
    throw new Error('尚未配置正式服务器，请先设置 RELEASE_API_BASE')
  }
  return normalizeServer(RELEASE_API_BASE)
}

export function getSocketBase(): string {
  const base = getApiBase()
  if (base) return base.replace(/^http/, 'ws')
  if (isH5) {
    const location = (globalThis as unknown as { location?: Location }).location
    const protocol = location && location.protocol === 'https:' ? 'wss://' : 'ws://'
    return protocol + (location ? location.host : 'localhost')
  }
  throw new Error('尚未配置服务器地址，无法建立连接')
}

export function setDevServer(value: string): void {
  if (!isDevelop()) throw new Error('正式环境不能修改服务器')
  const base = normalizeServer(value)
  if (!base) {
    uni.removeStorageSync(SERVER_KEY)
    return
  }
  uni.setStorageSync(SERVER_KEY, base)
}

export function getDevServer(): string {
  return (uni.getStorageSync(SERVER_KEY) as string) || DEFAULT_DEV_BASE
}