import { getApiBase } from './config'

export interface Session {
  nickname: string
  sessionId: string
  server: string
}

interface ApiResult<T> {
  code: number
  message: string
  data: T
}

const SESSION_KEY = 'chatroom.session'

const errors: Record<number, string> = {
  1001: '昵称已被使用，请换一个昵称',
  1002: '昵称或密码错误，或该账号已在其他端登录',
  1003: '登录已失效，请重新登录',
  1004: '请检查填写的内容',
  1005: '对方暂时不在线',
}

export class ApiError extends Error {
  constructor(message: string, public code = -1) {
    super(message)
  }
}

export function getSession(): Session | null {
  const value = uni.getStorageSync(SESSION_KEY) as Session | undefined
  return value && value.sessionId && value.nickname && value.server === getApiBase() ? value : null
}

export function saveSession(nickname: string, sessionId: string): void {
  uni.setStorageSync(SESSION_KEY, { nickname, sessionId, server: getApiBase() })
}

export function clearSession(): void {
  uni.removeStorageSync(SESSION_KEY)
}

export function errorMessage(error: unknown): string {
  return error instanceof Error ? error.message : '操作失败，请稍后重试'
}

export function toast(error: unknown): void {
  uni.showToast({ title: errorMessage(error), icon: 'none', duration: 3000 })
}

// 复用后端的 code/message/data 协议，不记录密码或会话凭据。
export function request<T>(
  path: string,
  method: 'GET' | 'POST' = 'GET',
  data?: Record<string, string>,
  auth = false,
): Promise<T> {
  return new Promise((resolve, reject) => {
    const base = getApiBase()
    const header: Record<string, string> = { 'content-type': 'application/json' }
    if (auth) {
      const session = getSession()
      if (!session) {
        reject(new ApiError(errors[1003], 1003))
        return
      }
      header['X-Session-Id'] = session.sessionId
    }
    uni.request({
      url: base + path,
      method,
      data,
      header,
      timeout: 10000,
      success(res) {
        const result = res.data as ApiResult<T> | undefined
        if (!result || typeof result.code !== 'number') {
          reject(new ApiError('服务器返回格式异常'))
          return
        }
        if (res.statusCode < 200 || res.statusCode >= 300 || result.code !== 0) {
          if (auth && result.code === 1003) clearSession()
          reject(new ApiError(errors[result.code] || result.message || '请求失败', result.code))
          return
        }
        resolve(result.data)
      },
      fail() {
        reject(new ApiError('无法连接服务器，请检查后端地址、网络和合法域名配置'))
      },
    })
  })
}