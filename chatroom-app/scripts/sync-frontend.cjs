// 把 uni-app 的 Web 构建产物同步到后端托管目录 frontend/，
// 使 http://localhost:8080 与开发服务器 http://localhost:5173 使用同一套界面与逻辑。
const fs = require('node:fs')
const path = require('node:path')

const source = path.resolve(__dirname, '../dist/build/h5')
const target = path.resolve(__dirname, '../../frontend')

if (!fs.existsSync(path.join(source, 'index.html'))) {
  console.error('未找到 H5 构建产物，请先执行 npm run build:h5')
  process.exit(1)
}

// 逐个文件镜像，避免整目录删除带来的句柄占用问题
function mirror(from, to) {
  fs.mkdirSync(to, { recursive: true })
  const entries = fs.readdirSync(from, { withFileTypes: true })
  const names = new Set(entries.map((entry) => entry.name))

  for (const entry of fs.readdirSync(to, { withFileTypes: true })) {
    if (!names.has(entry.name)) {
      fs.rmSync(path.join(to, entry.name), { recursive: true, force: true })
    }
  }

  for (const entry of entries) {
    const sourcePath = path.join(from, entry.name)
    const targetPath = path.join(to, entry.name)
    if (entry.isDirectory()) mirror(sourcePath, targetPath)
    else fs.copyFileSync(sourcePath, targetPath)
  }
}

mirror(source, target)

// 后端根路由 / 固定返回 login.html，用一个跳转页把入口交给单页应用
fs.writeFileSync(
  path.join(target, 'login.html'),
  `<!DOCTYPE html>
<html lang="zh-CN">
  <head>
    <meta charset="UTF-8" />
    <meta http-equiv="refresh" content="0; url=./index.html" />
    <title>聊天室</title>
  </head>
  <body>
    <script>
      location.replace('./index.html')
    </script>
  </body>
</html>
`,
)

console.log('已同步 Web 构建产物到 frontend/，http://localhost:8080 即为新界面')