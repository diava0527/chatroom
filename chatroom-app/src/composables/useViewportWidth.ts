import { onMounted, onUnmounted, ref } from 'vue'

interface ResizeResult {
  size: { windowWidth: number }
}

interface ResizeApi {
  onWindowResize?: (callback: (result: ResizeResult) => void) => void
  offWindowResize?: (callback: (result: ResizeResult) => void) => void
}

/** 监听窗口宽度，用于在 Web / 平板 / 手机上切换布局策略。 */
export function useViewportWidth() {
  const width = ref(uni.getSystemInfoSync().windowWidth || 375)
  const api = uni as unknown as ResizeApi
  const handler = (result: ResizeResult) => {
    width.value = result.size.windowWidth
  }

  onMounted(() => {
    if (typeof api.onWindowResize === 'function') api.onWindowResize(handler)
  })

  onUnmounted(() => {
    if (typeof api.offWindowResize === 'function') api.offWindowResize(handler)
  })

  return width
}