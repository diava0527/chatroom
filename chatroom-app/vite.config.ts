import { defineConfig } from 'vite'
import uni from '@dcloudio/vite-plugin-uni'

// Web 端开发时由 Vite 代理转发到本地后端，避免后端 CORS 限制；
// 生产环境建议由反向代理把 /api/v1 与 /ws/chat 暴露在同一域名下。
export default defineConfig({
  plugins: [uni()],
  server: {
    host: true,
    port: 5173,
    proxy: {
      '/api/v1': { target: 'http://127.0.0.1:8080', changeOrigin: true },
      '/ws/chat': { target: 'ws://127.0.0.1:8080', ws: true },
    },
  },
})