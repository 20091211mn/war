// محرك عرض WebGL صغير (بدون مكتبات): مجسّمات ملوّنة مضاءة + ضباب + سماء
const VS = `
attribute vec3 aPos; attribute vec3 aNor; attribute vec3 aCol;
uniform mat4 uVP; uniform mat4 uM; uniform vec3 uTint;
varying vec3 vN; varying vec3 vC; varying vec3 vP;
void main(){ vec4 w = uM*vec4(aPos,1.0); vP=w.xyz; vN=mat3(uM)*aNor; vC=aCol*uTint; gl_Position=uVP*w; }`;
const FS = `
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
varying vec3 vN; varying vec3 vC; varying vec3 vP;
uniform vec3 uSunDir; uniform float uSunI; uniform float uAmb; uniform vec3 uFogCol; uniform vec2 uFog; uniform vec3 uCam;
uniform float uDetail; uniform float uAlpha; uniform float uUnlit;
float hash(vec2 p){ return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
void main(){
  if (uUnlit > 0.5) { gl_FragColor = vec4(vC, uAlpha); return; }
  vec3 n = normalize(vN);
  float d = max(dot(n,uSunDir),0.0);
  float hemi = 0.5+0.5*n.y;
  vec3 col = vC;
  if (uDetail > 0.5) {
    vec3 an = abs(n);
    vec2 uv = an.y>0.6 ? vP.xz : (an.x>0.6 ? vP.zy : vP.xy);
    vec2 g = abs(fract(uv)-0.5);
    float ln = smoothstep(0.43,0.5,max(g.x,g.y));
    col *= 1.0-0.2*ln;
    col *= 0.9+0.2*hash(floor(uv*2.0));
  }
  float light = uAmb*(0.55+0.45*hemi) + uSunI*d;
  vec3 c = col*light;
  float dist = length(vP-uCam);
  float f = clamp((dist-uFog.x)/(uFog.y-uFog.x),0.0,1.0);
  c = mix(c,uFogCol,f);
  gl_FragColor = vec4(c,uAlpha);
}`;
const SKY_VS = `attribute vec2 aP; varying vec2 vU; void main(){ vU=aP; gl_Position=vec4(aP,0.9999,1.0); }`;
const SKY_FS = `
precision mediump float;
varying vec2 vU; uniform vec3 uF; uniform vec3 uR; uniform vec3 uU; uniform vec2 uT;
uniform vec3 uTop; uniform vec3 uHor; uniform vec3 uSun;
void main(){
  vec3 d = normalize(uF + vU.x*uT.x*uR + vU.y*uT.y*uU);
  float t = clamp(d.y,0.0,1.0);
  vec3 c = mix(uHor, uTop, pow(t,0.55));
  if (d.y < 0.0) c = mix(uHor, uHor*0.8, clamp(-d.y*4.0,0.0,1.0));
  float s = max(dot(d,uSun),0.0);
  c += vec3(1.0,0.92,0.75)*(pow(s,600.0)*1.2 + pow(s,12.0)*0.12);
  gl_FragColor = vec4(c,1.0);
}`;

const ring = []; for (let i = 0; i < 1024; i++) ring.push(new Float32Array(16));
let ri = 0;
export const tmp = () => ring[ri++ & 1023];
export const mat = () => new Float32Array(16);

export function ident(o) { o.fill(0); o[0] = o[5] = o[10] = o[15] = 1; return o; }
export function mul(o, a, b) {
  const a00 = a[0], a01 = a[1], a02 = a[2], a03 = a[3], a10 = a[4], a11 = a[5], a12 = a[6], a13 = a[7],
    a20 = a[8], a21 = a[9], a22 = a[10], a23 = a[11], a30 = a[12], a31 = a[13], a32 = a[14], a33 = a[15];
  for (let c = 0; c < 4; c++) {
    const b0 = b[c * 4], b1 = b[c * 4 + 1], b2 = b[c * 4 + 2], b3 = b[c * 4 + 3];
    o[c * 4] = a00 * b0 + a10 * b1 + a20 * b2 + a30 * b3;
    o[c * 4 + 1] = a01 * b0 + a11 * b1 + a21 * b2 + a31 * b3;
    o[c * 4 + 2] = a02 * b0 + a12 * b1 + a22 * b2 + a32 * b3;
    o[c * 4 + 3] = a03 * b0 + a13 * b1 + a23 * b2 + a33 * b3;
  }
  return o;
}
export function persp(o, fovy, asp, n, f) {
  const t = 1 / Math.tan(fovy / 2); o.fill(0);
  o[0] = t / asp; o[5] = t; o[10] = (f + n) / (n - f); o[11] = -1; o[14] = 2 * f * n / (n - f); return o;
}
// T(x,y,z) * Ry(ry) * Rx(rx) * S
export function trs(o, x, y, z, ry, rx, sx, sy, sz) {
  const cy = Math.cos(ry), sY = Math.sin(ry), cx = Math.cos(rx), sX = Math.sin(rx);
  o[0] = cy * sx; o[1] = 0; o[2] = -sY * sx; o[3] = 0;
  o[4] = sY * sX * sy; o[5] = cx * sy; o[6] = cy * sX * sy; o[7] = 0;
  o[8] = sY * cx * sz; o[9] = -sX * sz; o[10] = cy * cx * sz; o[11] = 0;
  o[12] = x; o[13] = y; o[14] = z; o[15] = 1; return o;
}
export function view(o, px, py, pz, yaw, pitch) {
  const cy = Math.cos(yaw), sy = Math.sin(yaw), cx = Math.cos(pitch), sx = Math.sin(pitch);
  const R = [[cy, sy * sx, sy * cx], [0, cx, -sx], [-sy, cy * sx, cy * cx]];
  for (let r = 0; r < 3; r++) for (let c = 0; c < 3; c++) o[c * 4 + r] = R[c][r];
  for (let r = 0; r < 3; r++) o[12 + r] = -(R[0][r] * px + R[1][r] * py + R[2][r] * pz);
  o[3] = o[7] = o[11] = 0; o[15] = 1; return o;
}
// نموذج من اتجاه: محور z على dir (للتراسر)
export function basis(o, px, py, pz, dx, dy, dz, sx, sy, sz) {
  let ux = 0, uy = 1, uz = 0;
  if (Math.abs(dy) > 0.99) { ux = 1; uy = 0; }
  let xx = uy * dz - uz * dy, xy = uz * dx - ux * dz, xz = ux * dy - uy * dx;
  const xl = Math.hypot(xx, xy, xz) || 1; xx /= xl; xy /= xl; xz /= xl;
  const yx = dy * xz - dz * xy, yy = dz * xx - dx * xz, yz = dx * xy - dy * xx;
  o[0] = xx * sx; o[1] = xy * sx; o[2] = xz * sx; o[3] = 0;
  o[4] = yx * sy; o[5] = yy * sy; o[6] = yz * sy; o[7] = 0;
  o[8] = dx * sz; o[9] = dy * sz; o[10] = dz * sz; o[11] = 0;
  o[12] = px; o[13] = py; o[14] = pz; o[15] = 1; return o;
}

function cubeData() {
  const faces = [[1, 0, 0], [-1, 0, 0], [0, 1, 0], [0, -1, 0], [0, 0, 1], [0, 0, -1]];
  const pos = [], nor = [], idx = [];
  faces.forEach((n, f) => {
    const a = n[0] ? [0, 1, 0] : [1, 0, 0], u = n[1] ? [0, 0, 1] : (n[0] ? [0, 0, 1] : [0, 1, 0]);
    const t1 = n[0] ? [0, 1, 0] : (n[1] ? [1, 0, 0] : [1, 0, 0]);
    const t2 = n[0] ? [0, 0, 1] : (n[1] ? [0, 0, 1] : [0, 1, 0]);
    const base = pos.length / 3;
    for (const [s1, s2] of [[-1, -1], [1, -1], [1, 1], [-1, 1]]) {
      pos.push(n[0] * 0.5 + t1[0] * 0.5 * s1 + t2[0] * 0.5 * s2, n[1] * 0.5 + t1[1] * 0.5 * s1 + t2[1] * 0.5 * s2, n[2] * 0.5 + t1[2] * 0.5 * s1 + t2[2] * 0.5 * s2);
      nor.push(n[0], n[1], n[2]);
    }
    idx.push(base, base + 1, base + 2, base, base + 2, base + 3);
  });
  return { pos, nor, idx };
}
function sphereData(seg = 10, rings = 7) {
  const pos = [], nor = [], idx = [];
  for (let j = 0; j <= rings; j++) for (let i = 0; i <= seg; i++) {
    const th = j / rings * Math.PI, ph = i / seg * Math.PI * 2;
    const x = Math.sin(th) * Math.cos(ph), y = Math.cos(th), z = Math.sin(th) * Math.sin(ph);
    pos.push(x * 0.5, y * 0.5, z * 0.5); nor.push(x, y, z);
  }
  for (let j = 0; j < rings; j++) for (let i = 0; i < seg; i++) {
    const a = j * (seg + 1) + i, b = a + seg + 1;
    idx.push(a, b, a + 1, b, b + 1, a + 1);
  }
  return { pos, nor, idx };
}

export class Engine {
  constructor(canvas) {
    const opts = { antialias: true, alpha: false, powerPreference: 'high-performance', preserveDrawingBuffer: false };
    const gl = canvas.getContext('webgl', opts) || canvas.getContext('experimental-webgl', opts);
    if (!gl) throw new Error('WebGL غير مدعوم على هذا الجهاز/المتصفح');
    this.gl = gl; this.canvas = canvas;
    this.prog = this._program(VS, FS);
    this.sky = this._program(SKY_VS, SKY_FS);
    const P = this.prog; const U = {};
    for (const n of ['uVP', 'uM', 'uTint', 'uSunDir', 'uSunI', 'uAmb', 'uFogCol', 'uFog', 'uCam', 'uDetail', 'uAlpha', 'uUnlit']) U[n] = gl.getUniformLocation(P, n);
    this.U = U;
    this.A = { pos: gl.getAttribLocation(P, 'aPos'), nor: gl.getAttribLocation(P, 'aNor'), col: gl.getAttribLocation(P, 'aCol') };
    const SU = {}; for (const n of ['uF', 'uR', 'uU', 'uT', 'uTop', 'uHor', 'uSun']) SU[n] = gl.getUniformLocation(this.sky, n);
    this.SU = SU; this.SA = gl.getAttribLocation(this.sky, 'aP');
    this.skyBuf = gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER, this.skyBuf);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 3, -1, -1, 3]), gl.STATIC_DRAW);
    this.cube = this._mesh(cubeData()); this.sphere = this._mesh(sphereData());
    this.map = null;
    this.VP = mat(); this.VPvm = mat(); this.proj = mat(); this.projVm = mat(); this.V = mat();
    this.cam = { x: 0, y: 0, z: 0, yaw: 0, pitch: 0, fov: 75 };
    this.theme = null; this.fovVm = 58 * Math.PI / 180;
    this.scale = 1; this.w = 1; this.h = 1;
    gl.enable(gl.DEPTH_TEST); gl.disable(gl.CULL_FACE);
    this.lost = false;
    canvas.addEventListener('webglcontextlost', (e) => { e.preventDefault(); this.lost = true; });
    canvas.addEventListener('webglcontextrestored', () => { location.reload(); });
  }
  _program(vs, fs) {
    const gl = this.gl;
    const mk = (type, src) => { const s = gl.createShader(type); gl.shaderSource(s, src); gl.compileShader(s); if (!gl.getShaderParameter(s, gl.COMPILE_STATUS)) throw new Error('shader: ' + gl.getShaderInfoLog(s)); return s; };
    const p = gl.createProgram(); gl.attachShader(p, mk(gl.VERTEX_SHADER, vs)); gl.attachShader(p, mk(gl.FRAGMENT_SHADER, fs)); gl.linkProgram(p);
    if (!gl.getProgramParameter(p, gl.LINK_STATUS)) throw new Error('link: ' + gl.getProgramInfoLog(p));
    return p;
  }
  _mesh(d) {
    const gl = this.gl, n = d.pos.length / 3, v = new Float32Array(n * 9);
    for (let i = 0; i < n; i++) { v.set([d.pos[i * 3], d.pos[i * 3 + 1], d.pos[i * 3 + 2], d.nor[i * 3], d.nor[i * 3 + 1], d.nor[i * 3 + 2], 1, 1, 1], i * 9); }
    const vb = gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER, vb); gl.bufferData(gl.ARRAY_BUFFER, v, gl.STATIC_DRAW);
    const ib = gl.createBuffer(); gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, ib); gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(d.idx), gl.STATIC_DRAW);
    return { vb, ib, count: d.idx.length };
  }
  resize(scale) {
    if (scale) this.scale = scale;
    const dpr = Math.min(window.devicePixelRatio || 1, 2) * this.scale;
    const w = Math.max(2, Math.floor(this.canvas.clientWidth * dpr)), h = Math.max(2, Math.floor(this.canvas.clientHeight * dpr));
    if (w !== this.canvas.width || h !== this.canvas.height) { this.canvas.width = w; this.canvas.height = h; }
    this.w = w; this.h = h;
  }
  // بناء مجسّم الخريطة الثابت (استدعاء واحد للرسم)
  setMap(map) {
    const gl = this.gl, th = map.theme;
    this.theme = th;
    const verts = [], idx = [];
    const addBox = (x0, y0, z0, x1, y1, z1, col) => {
      const base = verts.length / 9;
      const F = [
        [[1, 0, 0], [x1, y0, z1], [x1, y0, z0], [x1, y1, z0], [x1, y1, z1], 0.86],
        [[-1, 0, 0], [x0, y0, z0], [x0, y0, z1], [x0, y1, z1], [x0, y1, z0], 0.86],
        [[0, 1, 0], [x0, y1, z1], [x1, y1, z1], [x1, y1, z0], [x0, y1, z0], 1.08],
        [[0, -1, 0], [x0, y0, z0], [x1, y0, z0], [x1, y0, z1], [x0, y0, z1], 0.5],
        [[0, 0, 1], [x0, y0, z1], [x1, y0, z1], [x1, y1, z1], [x0, y1, z1], 0.95],
        [[0, 0, -1], [x1, y0, z0], [x0, y0, z0], [x0, y1, z0], [x1, y1, z0], 0.95],
      ];
      for (let f = 0; f < 6; f++) {
        const [n, a, b, c, d, br] = F[f], o = verts.length / 9;
        for (const p of [a, b, c, d]) verts.push(p[0], p[1], p[2], n[0], n[1], n[2], Math.min(1, col[0] * br), Math.min(1, col[1] * br), Math.min(1, col[2] * br));
        idx.push(o, o + 1, o + 2, o, o + 2, o + 3);
      }
      return base;
    };
    const M = 60;
    addBox(-map.W / 2 - M, -1, -map.D / 2 - M, map.W / 2 + M, 0, map.D / 2 + M, th.ground);
    map.boxes.forEach((b, i) => {
      const base = th.cols[b.k] || th.cols[0];
      const j = 0.92 + 0.16 * (((i * 7919) % 97) / 97);
      addBox(b.x0, b.y0, b.z0, b.x1, b.y1, b.z1, [base[0] * j, base[1] * j, base[2] * j]);
    });
    if (verts.length / 9 > 65000) throw new Error('الخريطة كبيرة جدًا');
    if (this.map) { gl.deleteBuffer(this.map.vb); gl.deleteBuffer(this.map.ib); }
    const vb = gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER, vb); gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(verts), gl.STATIC_DRAW);
    const ib = gl.createBuffer(); gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, ib); gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(idx), gl.STATIC_DRAW);
    this.map = { vb, ib, count: idx.length };
  }
  begin(cam) {
    const gl = this.gl, th = this.theme || { sky: [0.3, 0.4, 0.6], hor: [0.7, 0.7, 0.7], fog: [0.7, 0.7, 0.7], fogN: 40, fogF: 120, sun: [0.3, 0.8, 0.4], amb: 0.6, sunI: 0.5 };
    this.cur = null;
    this.resize();
    gl.viewport(0, 0, this.w, this.h);
    gl.clearColor(th.hor[0], th.hor[1], th.hor[2], 1); gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    this.cam = cam; const asp = this.w / this.h;
    persp(this.proj, cam.fov * Math.PI / 180, asp, 0.1, 400);
    view(this.V, cam.x, cam.y, cam.z, cam.yaw, cam.pitch);
    mul(this.VP, this.proj, this.V);
    persp(this.projVm, this.fovVm, asp, 0.02, 20);
    // سماء
    const cy = Math.cos(cam.yaw), sy = Math.sin(cam.yaw), cx = Math.cos(cam.pitch), sx = Math.sin(cam.pitch);
    const tn = Math.tan(cam.fov * Math.PI / 360);
    gl.disable(gl.DEPTH_TEST);
    gl.useProgram(this.sky);
    gl.uniform3f(this.SU.uF, -sy * cx, sx, -cy * cx); gl.uniform3f(this.SU.uR, cy, 0, -sy); gl.uniform3f(this.SU.uU, sy * sx, cx, cy * sx);
    gl.uniform2f(this.SU.uT, tn * asp, tn);
    gl.uniform3fv(this.SU.uTop, th.sky); gl.uniform3fv(this.SU.uHor, th.hor);
    const sl = Math.hypot(...th.sun); gl.uniform3f(this.SU.uSun, th.sun[0] / sl, th.sun[1] / sl, th.sun[2] / sl);
    gl.bindBuffer(gl.ARRAY_BUFFER, this.skyBuf); gl.enableVertexAttribArray(this.SA); gl.vertexAttribPointer(this.SA, 2, gl.FLOAT, false, 0, 0);
    for (const a of [this.A.nor, this.A.col]) gl.disableVertexAttribArray(a);
    gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.enable(gl.DEPTH_TEST);
    // برنامج الأجسام
    gl.useProgram(this.prog);
    const U = this.U;
    gl.uniform3f(U.uSunDir, th.sun[0] / sl, th.sun[1] / sl, th.sun[2] / sl);
    gl.uniform1f(U.uSunI, th.sunI); gl.uniform1f(U.uAmb, th.amb);
    gl.uniform3fv(U.uFogCol, th.fog); gl.uniform2f(U.uFog, th.fogN, th.fogF);
    gl.uniform3f(U.uCam, cam.x, cam.y, cam.z);
    gl.uniform1f(U.uAlpha, 1); gl.uniform1f(U.uUnlit, 0);
    gl.uniformMatrix4fv(U.uVP, false, this.VP);
    gl.disable(gl.BLEND); gl.depthMask(true);
    for (const a of [this.A.pos, this.A.nor, this.A.col]) gl.enableVertexAttribArray(a);
  }
  drawMap() {
    const gl = this.gl, U = this.U; if (!this.map) return;
    gl.bindBuffer(gl.ARRAY_BUFFER, this.map.vb); gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.map.ib);
    this._attrs();
    gl.uniformMatrix4fv(U.uM, false, IDENT); gl.uniform3f(U.uTint, 1, 1, 1); gl.uniform1f(U.uDetail, 1);
    gl.drawElements(gl.TRIANGLES, this.map.count, gl.UNSIGNED_SHORT, 0);
    gl.uniform1f(U.uDetail, 0);
    this.cur = null;
  }
  _attrs() {
    const gl = this.gl, A = this.A;
    gl.vertexAttribPointer(A.pos, 3, gl.FLOAT, false, 36, 0); gl.vertexAttribPointer(A.nor, 3, gl.FLOAT, false, 36, 12); gl.vertexAttribPointer(A.col, 3, gl.FLOAT, false, 36, 24);
  }
  _bind(mesh) { const gl = this.gl; gl.bindBuffer(gl.ARRAY_BUFFER, mesh.vb); gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, mesh.ib); this._attrs(); this.cur = mesh; }
  // opts: unlit, alpha, add
  draw(mesh, m, r, g, b, unlit = false, alpha = 1, add = false) {
    const gl = this.gl, U = this.U;
    if (this.cur !== mesh) this._bind(mesh);
    gl.uniformMatrix4fv(U.uM, false, m); gl.uniform3f(U.uTint, r, g, b);
    gl.uniform1f(U.uUnlit, unlit ? 1 : 0); gl.uniform1f(U.uAlpha, alpha);
    if (add) { gl.enable(gl.BLEND); gl.blendFunc(gl.SRC_ALPHA, gl.ONE); gl.depthMask(false); }
    else if (alpha < 1) { gl.enable(gl.BLEND); gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA); gl.depthMask(false); }
    gl.drawElements(gl.TRIANGLES, mesh.count, gl.UNSIGNED_SHORT, 0);
    if (add || alpha < 1) { gl.disable(gl.BLEND); gl.depthMask(true); }
  }
  cubeAt(m, r, g, b, unlit, alpha, add) { this.draw(this.cube, m, r, g, b, unlit, alpha, add); }
  sphereAt(m, r, g, b, unlit, alpha, add) { this.draw(this.sphere, m, r, g, b, unlit, alpha, add); }
  // مجسم السلاح: عمق مستقل + إسقاط خاص
  beginViewmodel() {
    const gl = this.gl;
    gl.clear(gl.DEPTH_BUFFER_BIT);
    gl.uniformMatrix4fv(this.U.uVP, false, this.projVm);
    gl.uniform3f(this.U.uCam, 0, 0, 0); gl.uniform2f(this.U.uFog, 1e5, 2e5);
  }
  endViewmodel() {
    this.gl.uniformMatrix4fv(this.U.uVP, false, this.VP);
    this.gl.uniform3f(this.U.uCam, this.cam.x, this.cam.y, this.cam.z);
    this.gl.uniform2f(this.U.uFog, this.theme.fogN, this.theme.fogF);
  }
  // إسقاط نقطة عالمية إلى الشاشة (CSS px نسبة 0..1)
  project(x, y, z, out) {
    const m = this.VP, cw = m[3] * x + m[7] * y + m[11] * z + m[15];
    if (cw <= 0.05) return false;
    out.x = ((m[0] * x + m[4] * y + m[8] * z + m[12]) / cw * 0.5 + 0.5);
    out.y = (1 - ((m[1] * x + m[5] * y + m[9] * z + m[13]) / cw * 0.5 + 0.5));
    out.w = cw; return true;
  }
}
const IDENT = ident(new Float32Array(16));
