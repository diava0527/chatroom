// 使用 Node 自带 fetch/WebSocket 驱动真实服务层，对真实后端做端到端联调。
// 运行前请先启动 C++ 后端（默认 http://127.0.0.1:8080），可用 TEST_API_BASE 覆盖地址。
const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const vm = require('node:vm')
const crypto = require('node:crypto')

const ts = require(path.join(__dirname, '../node_modules/typescript'))
const root = path.resolve(__dirname, '../src')
const API_BASE = process.env.TEST_API_BASE || 'http://127.0.0.1:8080'

function runtime() {
  const storage = new Map()
  const notices = []
  const sockets = []
  let version = 'develop'

  const wx = {
    getAccountInfoSync: () => ({ miniProgram: { envVersion: version } }),
  }

  const uni = {
    getStorageSync: (key) => storage.get(key),
    setStorageSync: (key, value) => storage.set(key, value),
    removeStorageSync: (key) => storage.delete(key),
    showToast: (options) => notices.push(options.title),
    request(options) {
      fetch(options.url, {
        method: options.method,
        headers: options.header,
        body: options.data ? JSON.stringify(options.data) : undefined,
        signal: AbortSignal.timeout(options.timeout),
      })
        .then(async (res) => options.success({ statusCode: res.status, data: await res.json() }))
        .catch(options.fail)
    },
    connectSocket(options) {
      const socket = new WebSocket(options.url)
      sockets.push(socket)
      let opened = false
      const task = {
        onOpen: (cb) =>
          socket.addEventListener('open', () => {
            opened = true
            cb()
          }),
        onMessage: (cb) => socket.addEventListener('message', (event) => cb({ data: event.data })),
        onClose: (cb) => socket.addEventListener('close', cb),
        onError: (cb) => socket.addEventListener('error', cb),
        send(options) {
          try {
            socket.send(options.data)
            options.success?.()
          } catch {
            options.fail?.()
          }
        },
        close() {
          if (opened || socket.readyState === WebSocket.OPEN) socket.close()
          else socket.addEventListener('open', () => socket.close(), { once: true })
        },
      }
      return task
    },
  }

  // 让一次加载的所有模块共享同一套平台环境
  const sandboxProcess = { env: { NODE_ENV: 'development', UNI_PLATFORM: 'mp-weixin' } }
  const cache = new Map()

  function load(file) {
    const full = path.resolve(root, file.endsWith('.ts') ? file : file + '.ts')
    if (cache.has(full)) return cache.get(full).exports
    const module = { exports: {} }
    cache.set(full, module)
    const code = ts.transpileModule(fs.readFileSync(full, 'utf8'), {
      compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 },
    }).outputText
    vm.runInNewContext(
      code,
      {
        module,
        exports: module.exports,
        uni,
        wx,
        process: sandboxProcess,
        console,
        setTimeout,
        clearTimeout,
        require: (relative) =>
          load(path.relative(root, path.resolve(path.dirname(full), relative))),
      },
      { filename: full },
    )
    return module.exports
  }

  return {
    load,
    uni,
    wx,
    sockets,
    notices,
    setVersion: (value) => {
      version = value
    },
  }
}

async function waitFor(check, description, timeout = 12000) {
  const end = Date.now() + timeout
  while (!check()) {
    if (Date.now() >= end) throw new Error('Timeout: ' + description)
    await new Promise((resolve) => setTimeout(resolve, 60))
  }
}

async function main() {
  const a = runtime()
  const b = runtime()
  const apiA = a.load('services/api')
  const apiB = b.load('services/api')
  const configA = a.load('services/config')

  a.setVersion('release')
  assert.throws(() => configA.getApiBase(), /正式服务器/)
  a.setVersion('trial')
  assert.throws(() => configA.getApiBase(), /正式服务器/)
  a.setVersion('develop')
  assert.throws(() => configA.setDevServer('http://user:password@host'), /服务器根地址/)
  assert.throws(() => configA.setDevServer('http://host/api'), /服务器根地址/)
  apiA.saveSession('test', 'not-real')
  configA.setDevServer('http://localhost:8080')
  assert.equal(apiA.getSession(), null, 'session must not leak across servers')
  configA.setDevServer(API_BASE)
  apiA.clearSession()
  console.log('PASS: 正式环境隔离、地址校验、会话与服务器绑定')

  const { ChatClient, mergeMessages } = a.load('services/chat')
  const message = {
    messageId: 'id1',
    content: 'hi',
    timestamp: '2026-08-31 10:00:00',
    senderNickname: 'a',
    receiverNickname: 'LOBBY',
  }
  assert.equal(mergeMessages([message], [message]).length, 1)
  assert.equal(
    mergeMessages(
      [],
      Array.from({ length: 230 }, (_, i) => ({ ...message, messageId: 'id' + i })),
    ).length,
    200,
  )
  console.log('PASS: 消息去重与长会话容量限制')

  const suffix = crypto.randomBytes(4).toString('hex')
  const nickA = 'wx_test_a_' + suffix
  const nickB = 'wx_test_b_' + suffix
  const password = crypto.randomBytes(18).toString('hex')
  let clientA
  let clientB
  let stateA
  let stateB
  let expiredA = false
  try {
    await apiA.request('/api/v1/health')
    for (const [api, nickname] of [
      [apiA, nickA],
      [apiB, nickB],
    ]) {
      await api.request('/api/v1/auth/register', 'POST', { nickname, password })
      const login = await api.request('/api/v1/auth/login', 'POST', { nickname, password })
      api.saveSession(login.nickname, login.sessionId)
    }
    await assert.rejects(
      apiA.request('/api/v1/auth/register', 'POST', { nickname: nickA, password }),
      (error) => error.code === 1001,
    )
    await assert.rejects(
      apiA.request('/api/v1/auth/login', 'POST', { nickname: nickA, password: 'wrong' }),
      (error) => error.code === 1002,
    )
    console.log('PASS: 真实后端注册、登录、重复昵称和错误密码处理')

    clientA = new ChatClient(
      (state) => {
        stateA = state
      },
      () => {
        expiredA = true
      },
    )
    clientB = new (b.load('services/chat').ChatClient)(
      (state) => {
        stateB = state
      },
      () => {},
    )
    clientA.start()
    clientB.start()
    await waitFor(() => stateA?.connected && stateB?.connected, 'both sockets ready')
    await waitFor(
      () => stateA.users.some((user) => user.nickname === nickB && user.online),
      'online user broadcast',
    )
    await clientA.send('multi platform lobby integration test')
    await waitFor(
      () => stateB.messages.some((item) => item.content === 'multi platform lobby integration test'),
      'lobby delivery',
    )
    assert.ok(stateA.messages.some((item) => item.mine))
    console.log('PASS: WebSocket 握手、在线成员广播及双端大厅收发')

    assert.equal(await clientA.openPrivate(nickB), true)
    await clientA.send('multi platform private integration test')
    await waitFor(
      () => stateB.users.some((user) => user.nickname === nickA && user.unread === 1),
      'private unread',
    )
    assert.equal(await clientB.openPrivate(nickA), true)
    await waitFor(
      () => stateB.messages.some((item) => item.content === 'multi platform private integration test'),
      'private history',
    )
    await clientB.send('multi platform private reply')
    await waitFor(
      () => stateA.messages.some((item) => item.content === 'multi platform private reply'),
      'private reply',
    )
    assert.equal(stateB.users.find((user) => user.nickname === nickA).unread, 0)
    console.log('PASS: 私聊创建、双向收发、历史记录及未读清零')

    const before = a.sockets.length
    a.sockets.at(-1).close()
    await waitFor(() => a.sockets.length > before && stateA.connected, 'automatic reconnect')
    clientA.stop()
    const stopped = a.sockets.length
    await new Promise((resolve) => setTimeout(resolve, 1100))
    assert.equal(a.sockets.length, stopped, 'hidden page must not reconnect')
    clientA.start()
    await waitFor(() => a.sockets.length > stopped && stateA.connected, 'foreground resume')
    console.log('PASS: 网络断开自动重连、后台暂停与前台恢复')

    await apiB.request('/api/v1/auth/logout', 'POST', undefined, true)
    clientB.stop()
    apiB.clearSession()
    await waitFor(
      () => !stateA.users.some((user) => user.nickname === nickB && user.online),
      'logout offline',
    )
    const relogin = await apiB.request('/api/v1/auth/login', 'POST', { nickname: nickB, password })
    apiB.saveSession(relogin.nickname, relogin.sessionId)
    clientB.start()
    await waitFor(
      () => stateB.connected && stateA.users.some((user) => user.nickname === nickB && user.online),
      'relogin online',
    )
    assert.equal(await clientA.openPrivate(nickB), true)
    await waitFor(
      () => stateA.currentPartner === nickB && stateA.messages.length === 0,
      'old private window cleared and reopened',
    )
    await clientA.send('new private window')
    await waitFor(
      () => stateB.messages.some((item) => item.content === 'new private window'),
      'new private delivery',
    )
    console.log('PASS: 退出后私聊清空，重登后创建新窗口')

    await apiA.request('/api/v1/auth/logout', 'POST', undefined, true)
    clientA.retry()
    await waitFor(() => expiredA, 'expired session redirect')
    assert.equal(apiA.getSession(), null)
    console.log('PASS: 会话失效清理并跳转登录')
  } finally {
    clientA?.stop()
    clientB?.stop()
    for (const api of [apiA, apiB]) {
      if (api.getSession()) {
        await api.request('/api/v1/auth/logout', 'POST', undefined, true).catch(() => {})
      }
      api.clearSession()
    }
  }
  console.log('全部检查通过。测试使用独立 wx_test_ 前缀账号，已退出登录；未修改现有账号。')
}

main().catch((error) => {
  console.error(error)
  process.exitCode = 1
})