import { getSocketBase } from './config'
import { ApiError, clearSession, getSession, request, toast } from './api'

export interface Message {
  messageId: string
  senderNickname: string
  receiverNickname: string
  content: string
  timestamp: string
}

interface Conversation {
  id: string
  messages: Message[]
  unread: number
}

interface Packet {
  event: string
  payload: {
    messages?: Message[]
    users?: string[]
    privateSessionId?: string
    message?: Message | string
  } & Partial<Message>
}

export interface ChatState {
  nickname: string
  status: string
  connected: boolean
  title: string
  currentPartner: string
  onlineCount: number
  lobbyUnread: number
  users: { nickname: string; initial: string; online: boolean; unread: number }[]
  messages: (Message & { mine: boolean; time: string; initial: string; anchor: string })[]
  scrollTarget: string
}

/** 统一的 SocketTask 结构，屏蔽各端差异，便于代码复用与测试。 */
export interface ChatSocket {
  onOpen: (callback: () => void) => void
  onMessage: (callback: (event: { data: unknown }) => void) => void
  onClose: (callback: () => void) => void
  onError: (callback: () => void) => void
  send: (options: { data: string; success?: () => void; fail?: () => void }) => void
  close: (options?: Record<string, unknown>) => void
}

// 保留最近 200 条消息，避免长会话导致 setData 包体过大。
export function mergeMessages(previous: Message[], incoming: Message[]): Message[] {
  const seen = new Set<string>()
  return previous
    .concat(incoming)
    .filter((message) => {
      if (!message || !message.messageId || seen.has(message.messageId)) return false
      seen.add(message.messageId)
      return true
    })
    .sort((a, b) => a.timestamp.localeCompare(b.timestamp))
    .slice(-200)
}

export class ChatClient {
  private task: ChatSocket | null = null
  private timer: number | null = null
  private connectionTimer: number | null = null
  private generation = 0
  private running = false
  private attempt = 0
  private ready = false
  private status = '连接中'
  private nickname = ''
  private online: string[] = []
  private lobby: Message[] = []
  private lobbyUnread = 0
  private partner = ''
  private conversations: Record<string, Conversation> = Object.create(null)
  private opening = false

  constructor(
    private update: (state: ChatState) => void,
    private expired: () => void,
  ) {}

  private emit(): void {
    const messages = this.partner ? this.conversations[this.partner]?.messages || [] : this.lobby
    const names = Array.from(new Set(this.online.concat(Object.keys(this.conversations)))).filter(
      (name) => name !== this.nickname,
    )
    this.update({
      nickname: this.nickname,
      status: this.status,
      connected: this.ready,
      title: this.partner || '公共大厅',
      currentPartner: this.partner,
      onlineCount: this.online.length,
      lobbyUnread: this.lobbyUnread,
      users: names.map((name) => ({
        nickname: name,
        initial: name.slice(0, 1),
        online: this.online.includes(name),
        unread: this.conversations[name]?.unread || 0,
      })),
      messages: messages.map((message, index) => ({
        ...message,
        mine: message.senderNickname === this.nickname,
        initial: message.senderNickname.slice(0, 1),
        time: message.timestamp.includes(' ') ? message.timestamp.split(' ')[1] : message.timestamp,
        anchor: 'message-' + index,
      })),
      scrollTarget: messages.length ? 'message-' + (messages.length - 1) : '',
    })
  }

  start(): void {
    if (this.running) return
    this.running = true
    this.attempt = 0
    void this.connect()
  }

  stop(): void {
    this.running = false
    this.generation++
    this.ready = false
    this.status = '已暂停'
    if (this.timer !== null) clearTimeout(this.timer)
    if (this.connectionTimer !== null) clearTimeout(this.connectionTimer)
    this.timer = null
    this.connectionTimer = null
    const old = this.task
    this.task = null
    if (old) old.close({})
  }

  retry(): void {
    this.stop()
    this.start()
  }

  private async connect(): Promise<void> {
    const generation = ++this.generation
    this.status = this.attempt ? '正在重新连接' : '连接中'
    this.emit()
    try {
      const session = getSession()
      if (!session) {
        this.invalidate()
        return
      }
      this.nickname = session.nickname
      const result = await request<{ onlineUsers: string[] }>(
        '/api/v1/users/online',
        'GET',
        undefined,
        true,
      )
      if (!this.running || generation !== this.generation) return
      this.online = result.onlineUsers || []
      const task = uni.connectSocket({
        url: getSocketBase() + '/ws/chat?sessionId=' + encodeURIComponent(session.sessionId),
        // 提供回调确保返回 SocketTask，而非 Promise。
        success() {},
        fail: () => this.disconnected(generation),
      }) as unknown as ChatSocket
      this.task = task
      this.connectionTimer = setTimeout(() => this.disconnected(generation), 12000)
      task.onOpen(() => {
        if (generation !== this.generation || !this.running) {
          task.close({})
          return
        }
        this.sendPacket('lobby.enter')
      })
      task.onMessage((event) => {
        if (generation !== this.generation || !this.running) return
        try {
          this.receive(JSON.parse(String(event.data)) as Packet)
        } catch {
          toast(new Error('收到无法识别的消息'))
        }
      })
      task.onClose(() => this.disconnected(generation))
      task.onError(() => this.disconnected(generation))
    } catch (error) {
      if (generation !== this.generation || !this.running) return
      if (error instanceof ApiError && error.code === 1003) {
        this.invalidate()
        return
      }
      this.disconnected(generation)
    }
  }

  private invalidate(): void {
    this.stop()
    clearSession()
    this.expired()
  }

  private disconnected(generation: number): void {
    if (!this.running || generation !== this.generation) return
    // 错误和关闭事件可能同时到达，只安排一次重连并忽略旧连接的迟到回调。
    this.generation++
    if (this.connectionTimer !== null) clearTimeout(this.connectionTimer)
    this.connectionTimer = null
    const old = this.task
    this.task = null
    if (old) old.close({})
    this.ready = false
    this.status = '连接断开，正在重试'
    this.emit()
    const delay = Math.min(1000 * Math.pow(2, this.attempt++), 15000)
    this.timer = setTimeout(() => {
      this.timer = null
      void this.connect()
    }, delay)
  }

  private sendPacket(event: string, payload: Record<string, string> = {}): void {
    if (!this.task) return
    const generation = this.generation
    this.task.send({
      data: JSON.stringify({ event, payload }),
      fail: () => this.disconnected(generation),
    })
  }

  private conversation(name: string, id?: string): Conversation {
    if (!this.conversations[name] || (id && this.conversations[name].id !== id)) {
      this.conversations[name] = { id: id || '', messages: [], unread: 0 }
    }
    return this.conversations[name]
  }

  private receive(packet: Packet): void {
    const payload = packet.payload || {}
    switch (packet.event) {
      case 'lobby.enter.ack':
        if (this.connectionTimer !== null) clearTimeout(this.connectionTimer)
        this.connectionTimer = null
        this.ready = true
        this.attempt = 0
        this.status = '已连接'
        this.sendPacket('lobby.history.pull')
        if (this.partner && this.conversations[this.partner]?.id) {
          this.sendPacket('private.history.pull', {
            privateSessionId: this.conversations[this.partner].id,
          })
        }
        break
      case 'online.users.changed':
        this.online = payload.users || []
        break
      case 'lobby.message.receive': {
        const message = payload as Message
        const isNew = !this.lobby.some((item) => item.messageId === message.messageId)
        this.lobby = mergeMessages(this.lobby, [message])
        if (isNew && this.partner && message.senderNickname !== this.nickname) this.lobbyUnread++
        break
      }
      case 'lobby.history.response':
        this.lobby = mergeMessages(this.lobby, payload.messages || [])
        break
      case 'private.message.receive': {
        if (!payload.privateSessionId || !payload.message || typeof payload.message === 'string')
          return
        const message = payload.message
        const name =
          message.senderNickname === this.nickname ? message.receiverNickname : message.senderNickname
        if (!name) return
        const chat = this.conversation(name, payload.privateSessionId)
        const isNew = !chat.messages.some((item) => item.messageId === message.messageId)
        chat.messages = mergeMessages(chat.messages, [message])
        if (isNew && name !== this.partner && message.senderNickname !== this.nickname)
          chat.unread++
        break
      }
      case 'private.history.response': {
        const name = Object.keys(this.conversations).find(
          (key) => this.conversations[key].id === payload.privateSessionId,
        )
        if (name)
          this.conversations[name].messages = mergeMessages(
            this.conversations[name].messages,
            payload.messages || [],
          )
        break
      }
      case 'chat.error': {
        const message = typeof payload.message === 'string' ? payload.message : '聊天请求失败'
        if (message === 'private session is invalid') {
          // 对方退出后后端会删除私聊窗口，清除旧窗口，下次重新创建。
          const previousPartner = this.partner
          if (previousPartner) delete this.conversations[previousPartner]
          this.partner = ''
          toast(new Error('原私聊窗口已结束，正在重新打开；未送达的消息请重发'))
          if (previousPartner && this.online.includes(previousPartner))
            void this.openPrivate(previousPartner)
        } else toast(new Error(message))
        break
      }
      default:
        return
    }
    this.emit()
  }

  showLobby(): void {
    this.partner = ''
    this.lobbyUnread = 0
    this.emit()
  }

  async openPrivate(name: string): Promise<boolean> {
    if (!name || name === this.nickname || this.opening) return false
    this.opening = true
    const generation = this.generation
    try {
      // 后端每次 POST 都创建新窗口，必须复用收到消息时登记的窗口 ID。
      // 失效窗口由 private.history.pull 的错误响应触发重新创建。
      const result = this.conversations[name]?.id
        ? { privateSessionId: this.conversations[name].id }
        : await request<{ privateSessionId: string }>(
            '/api/v1/private-chat/session',
            'POST',
            { targetNickname: name },
            true,
          )
      if (!this.running || generation !== this.generation) return false
      const chat = this.conversation(name, result.privateSessionId)
      chat.unread = 0
      this.partner = name
      if (this.ready) this.sendPacket('private.history.pull', { privateSessionId: chat.id })
      this.emit()
      return true
    } catch (error) {
      if (error instanceof ApiError && error.code === 1003) this.invalidate()
      else toast(error)
      return false
    } finally {
      this.opening = false
    }
  }

  send(content: string): Promise<void> {
    return new Promise((resolve, reject) => {
      if (!this.ready || !this.task) {
        reject(new Error('聊天尚未连接，请稍后重试'))
        return
      }
      const text = content.trim()
      if (!text || text.length > 1000) {
        reject(new Error('请输入 1–1000 字的消息'))
        return
      }
      const payload = this.partner
        ? { privateSessionId: this.conversations[this.partner].id, content: text }
        : { content: text }
      this.task.send({
        data: JSON.stringify({
          event: this.partner ? 'private.message.send' : 'lobby.message.send',
          payload,
        }),
        success() {
          resolve()
        },
        fail() {
          reject(new Error('发送失败，消息已保留，请重试'))
        },
      })
    })
  }
}