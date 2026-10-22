// خادم WebSocket بسيط (RFC 6455) بدون أي اعتماديات خارجية
import crypto from 'node:crypto';
import { EventEmitter } from 'node:events';

const GUID = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11';
const MAX_PAYLOAD = 256 * 1024;

export class WSConn extends EventEmitter {
  constructor(socket) {
    super();
    this.socket = socket;
    this.buf = Buffer.alloc(0);
    this.frag = null;
    this.closed = false;
    this.alive = true;
    socket.on('data', (d) => this._onData(d));
    socket.on('close', () => this._fin());
    socket.on('error', () => this._fin());
    socket.on('timeout', () => this.terminate());
  }
  _fin() {
    if (this.closed) return;
    this.closed = true;
    this.emit('close');
  }
  _frame(opcode, payload) {
    const len = payload.length;
    let head;
    if (len < 126) head = Buffer.from([0x80 | opcode, len]);
    else if (len < 65536) { head = Buffer.alloc(4); head[0] = 0x80 | opcode; head[1] = 126; head.writeUInt16BE(len, 2); }
    else { head = Buffer.alloc(10); head[0] = 0x80 | opcode; head[1] = 127; head.writeBigUInt64BE(BigInt(len), 2); }
    return Buffer.concat([head, payload]);
  }
  _write(buf) {
    if (this.closed || this.socket.destroyed) return;
    try { this.socket.write(buf); } catch { /* تجاهل */ }
  }
  // droppable: يُتخطى إن كان الاتصال مزدحمًا (لقطات الحالة)
  send(str, droppable = false) {
    if (this.closed) return;
    if (droppable && this.socket.writableLength > 128 * 1024) return;
    this._write(this._frame(1, Buffer.from(str, 'utf8')));
  }
  ping() { this._write(this._frame(9, Buffer.alloc(0))); }
  close(code = 1000) {
    if (this.closed) return;
    const b = Buffer.alloc(2); b.writeUInt16BE(code);
    this._write(this._frame(8, b));
    setTimeout(() => this.terminate(), 200).unref();
  }
  terminate() { try { this.socket.destroy(); } catch { /* */ } this._fin(); }
  _onData(d) {
    this.buf = this.buf.length ? Buffer.concat([this.buf, d]) : d;
    if (this.buf.length > MAX_PAYLOAD * 2) { this.close(1009); this.buf = Buffer.alloc(0); return; }
    for (;;) {
      const b = this.buf;
      if (b.length < 2) return;
      const fin = (b[0] & 0x80) !== 0, op = b[0] & 0x0f, masked = (b[1] & 0x80) !== 0;
      let len = b[1] & 0x7f, off = 2;
      if (len === 126) { if (b.length < 4) return; len = b.readUInt16BE(2); off = 4; }
      else if (len === 127) { if (b.length < 10) return; const big = b.readBigUInt64BE(2); if (big > BigInt(MAX_PAYLOAD)) { this.close(1009); this.buf = Buffer.alloc(0); return; } len = Number(big); off = 10; }
      if (len > MAX_PAYLOAD) { this.close(1009); this.buf = Buffer.alloc(0); return; }
      if (!masked) { this.close(1002); this.buf = Buffer.alloc(0); return; }
      if (b.length < off + 4 + len) return;
      const mask = b.subarray(off, off + 4);
      const payload = Buffer.from(b.subarray(off + 4, off + 4 + len));
      for (let i = 0; i < payload.length; i++) payload[i] ^= mask[i & 3];
      this.buf = b.subarray(off + 4 + len);
      this._handle(fin, op, payload);
    }
  }
  _handle(fin, op, payload) {
    if (op === 8) { this._write(this._frame(8, payload.subarray(0, 2))); setTimeout(() => this.terminate(), 100).unref(); return; }
    if (op === 9) { this._write(this._frame(10, payload)); return; }
    if (op === 10) { this.alive = true; return; }
    if (op === 1 || op === 2) { this.frag = { op, parts: [payload] }; }
    else if (op === 0 && this.frag) { this.frag.parts.push(payload); }
    else return;
    if (!fin) return;
    const f = this.frag; this.frag = null;
    if (f.op === 1) this.emit('message', Buffer.concat(f.parts).toString('utf8'));
  }
}

export function attachWS(httpServer, onConnection) {
  httpServer.on('upgrade', (req, socket) => {
    const key = req.headers['sec-websocket-key'];
    if (String(req.headers.upgrade || '').toLowerCase() !== 'websocket' || !key) { socket.destroy(); return; }
    const accept = crypto.createHash('sha1').update(key + GUID).digest('base64');
    socket.write('HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ' + accept + '\r\n\r\n');
    socket.setNoDelay(true);
    socket.setTimeout(0);
    onConnection(new WSConn(socket), req);
  });
}
