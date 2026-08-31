const assert = require('node:assert/strict');
const net = require('node:net');
const { spawn } = require('node:child_process');
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));

async function main() {
  const reservation = net.createServer();
  await new Promise(resolve => reservation.listen(0, '127.0.0.1', resolve));
  const port = reservation.address().port;
  await new Promise(resolve => reservation.close(resolve));
  const server = spawn(process.argv[2], [String(port)], { windowsHide: true, stdio: 'ignore' });
  let launchError;
  server.on('error', error => { launchError = error; });
  const base = `http://127.0.0.1:${port}`;
  const sockets = [];
  async function api(action, body, session) {
    const response = await fetch(base + '/api/v1/auth/' + action, {
      method: 'POST', headers: { 'Content-Type': 'application/json', 'X-Session-Id': session || '' },
      body: JSON.stringify(body || {}), signal: AbortSignal.timeout(2000),
    });
    return response.json();
  }
  function connect(session) {
    return new Promise((resolve, reject) => {
      const socket = new WebSocket(`ws://127.0.0.1:${port}/ws/chat?sessionId=${session}`);
      sockets.push(socket);
      socket.addEventListener('error', () => reject(new Error('Socket rejected')), { once: true });
      // online.users.changed is emitted after the controller has attached this connection.
      socket.addEventListener('message', () => resolve(socket), { once: true });
    });
  }
  async function close(socket) {
    const closed = new Promise(resolve => socket.addEventListener('close', resolve, { once: true }));
    socket.close(); await closed;
  }
  try {
    for (let i = 0; ; i++) {
      if (launchError) throw launchError;
      try { if ((await fetch(base + '/health')).ok) break; } catch {}
      if (i >= 80) throw new Error('Test server failed to start');
      await delay(50);
    }
    const credentials = { nickname: 'n', password: 'isolated-test-only' };
    assert.equal((await api('register', credentials)).code, 0);
    const first = (await api('login', credentials)).data.sessionId;
    const page = await connect(first);
    assert.equal((await api('login', credentials)).code, 1002, 'active duplicate rejected');
    await close(page); // 关闭网页：仅关闭 socket，不调用 logout。
    const relogin = await api('login', credentials);
    assert.equal(relogin.code, 0, 'closed page must not lock account');
    assert.notEqual(relogin.data.sessionId, first);
    await assert.rejects(connect(first), /rejected/, 'old token must be invalid');
    const second = relogin.data.sessionId;
    const mobile = await connect(second);
    await close(mobile);
    const resumed = await connect(second);
    assert.equal((await api('login', credentials)).code, 1002, 'resumed session stays active');
    const echo = new Promise(resolve => resumed.addEventListener('message', event => resolve(event.data), { once: true }));
    resumed.send('after-resume');
    assert.equal(await echo, 'after-resume');
    assert.equal((await api('logout', {}, second)).code, 0);
    const revoked = new Promise(resolve => resumed.addEventListener('close', resolve, { once: true }));
    resumed.send('must-not-dispatch');
    await revoked;
    console.log('PASS: real HTTP/WebSocket close-without-logout, immediate relogin, old-token rejection, background resume and revoked socket');
  } finally {
    for (const socket of sockets) if (socket.readyState === WebSocket.OPEN) socket.close();
    server.kill();
  }
}
main().catch(error => { console.error(error); process.exitCode = 1; });
