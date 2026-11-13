// 结构完整性、品牌残留与发布地址检查。无需安装第三方依赖。
const fs = require('node:fs')
const path = require('node:path')

const root = path.resolve(__dirname, '..')
const srcRoot = path.join(root, 'src')

function fail(message) {
  throw new Error(message)
}

function readJson(relative) {
  const file = path.join(root, relative)
  if (!fs.existsSync(file)) fail('缺少文件：' + relative)
  try {
    return JSON.parse(fs.readFileSync(file, 'utf8'))
  } catch (error) {
    fail(relative + ' 不是合法 JSON：' + error.message)
  }
}

function walk(directory) {
  const result = []
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    const full = path.join(directory, entry.name)
    if (entry.isDirectory()) {
      if (['node_modules', 'dist', 'unpackage', '.backups'].includes(entry.name)) continue
      result.push(...walk(full))
    } else {
      result.push(full)
    }
  }
  return result
}

// 1. 多端工程必需文件
for (const file of [
  'index.html',
  'vite.config.ts',
  'tsconfig.json',
  'src/main.ts',
  'src/App.vue',
  'src/uni.scss',
  'src/styles/base.scss',
  'src/pages.json',
  'src/manifest.json',
  'src/services/api.ts',
  'src/services/chat.ts',
  'src/services/config.ts',
]) {
  if (!fs.existsSync(path.join(root, file))) fail('缺少多端工程文件：' + file)
}

// 2. 页面注册与页面文件一一对应
const pages = readJson('src/pages.json')
if (!Array.isArray(pages.pages) || pages.pages.length === 0) fail('src/pages.json 未注册任何页面')
for (const page of pages.pages) {
  const base = path.join(srcRoot, page.path)
  if (!fs.existsSync(base + '.vue')) fail('页面缺少 vue 文件：' + page.path + '.vue')
}

// 3. 小程序配置
const manifest = readJson('src/manifest.json')
if (manifest.vueVersion !== '3') fail('manifest.json 必须声明 vueVersion 3')
if (!manifest['mp-weixin'] || !manifest['mp-weixin'].appid) fail('manifest.json 缺少 mp-weixin.appid')
if (!manifest.h5 || !manifest.h5.title) fail('manifest.json 缺少 h5.title')

// 4. 品牌残留扫描：确保没有旧品牌文案、标识与存储键
const banned = [/长庚/, /changgeng/i]
const textExtensions = new Set(['.ts', '.vue', '.scss', '.json', '.js', '.cjs', '.html', '.md'])
const offenders = []
for (const file of walk(root)) {
  if (!textExtensions.has(path.extname(file))) continue
  const relative = path.relative(root, file)
  if (relative === path.join('scripts', 'check.cjs')) continue
  if (relative.startsWith('package-lock.json')) continue
  const text = fs.readFileSync(file, 'utf8')
  for (const pattern of banned) {
    if (pattern.test(text)) offenders.push(relative + ' -> ' + pattern)
  }
}
if (offenders.length) fail('发现旧品牌残留：\n' + offenders.join('\n'))

// 5. 发布地址检查（小程序端发布必须配置公网 HTTPS 域名）
if (process.argv.includes('--release')) {
  const configText = fs.readFileSync(path.join(srcRoot, 'services/config.ts'), 'utf8')
  const match = configText.match(/RELEASE_API_BASE\s*=\s*'([^']*)'/)
  const value = match ? match[1] : ''
  if (!/^https:\/\/[a-z0-9.-]+(?::443)?$/i.test(value) || /localhost|127\.0\.0\.1|example\./i.test(value)) {
    fail('请先在 src/services/config.ts 设置公网 RELEASE_API_BASE，当前不可发布小程序正式版。')
  }
}

console.log('PASS: 多端工程结构、页面注册、品牌清理与发布配置检查')