import { reactive } from 'vue'
import { ChatClient, type ChatState } from '../services/chat'

function initialState(): ChatState {
  return {
    nickname: '',
    status: '连接中',
    connected: false,
    title: '公共大厅',
    currentPartner: '',
    onlineCount: 0,
    lobbyUnread: 0,
    users: [],
    messages: [],
    scrollTarget: '',
  }
}

/** 把 ChatClient 的命令式推送桥接为页面可直接绑定响应式状态。 */
export function useChat(onExpired: () => void) {
  const state = reactive<ChatState>(initialState())
  const client = new ChatClient((next) => Object.assign(state, next), onExpired)
  return { state, client }
}