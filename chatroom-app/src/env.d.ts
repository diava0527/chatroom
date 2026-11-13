/// <reference types="vite/client" />

// 由 uni-app / Vite 在构建期注入
declare const process: {
  env: {
    NODE_ENV?: string
    UNI_PLATFORM?: string
  }
}

declare module '*.vue' {
  import type { DefineComponent } from 'vue'
  const component: DefineComponent<Record<string, unknown>, Record<string, unknown>, unknown>
  export default component
}