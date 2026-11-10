// G-Engine — web/engine.js
// Loads the engine (WebAssembly) and gives it what a browser has and C++ does not:
// a WebGL2 context, a console, and strings. Everything else happens inside the engine.
//
// The engine's renderer is written against a tiny graphics API ("gx", see engine/gfx/gl.h).
// Here each gx function is forwarded to WebGL2; on Android the same functions call OpenGL ES 3.

export const Button = { left: 0, right: 1, up: 2, down: 3, jump: 4, action: 5 };
export const Option = { lighting: 0, bloom: 1, lightScale: 2, exposure: 3 };

export async function loadEngine(canvas, wasmUrl = 'g-engine.wasm') {
  const gl = canvas.getContext('webgl2', {
    alpha: false, antialias: false, depth: false, stencil: false,
    powerPreference: 'high-performance', preserveDrawingBuffer: false,
  });
  if (!gl) throw new Error('webgl2-unavailable');

  // Rendering to 16-bit float textures (HDR) is an extension in WebGL2.
  const floatTargets = !!gl.getExtension('EXT_color_buffer_float') || !!gl.getExtension('EXT_color_buffer_half_float');

  let memory = null;
  const decoder = new TextDecoder();
  const encoder = new TextEncoder();
  const bytes = (ptr, len) => new Uint8Array(memory.buffer, ptr, len);
  const text = (ptr, len) => decoder.decode(bytes(ptr, len));
  const cstring = (ptr) => {
    if (!ptr) return '';
    const all = new Uint8Array(memory.buffer);
    let end = ptr;
    while (all[end] !== 0) ++end;
    return decoder.decode(all.subarray(ptr, end));
  };
  const writeText = (ptr, capacity, value) => {
    if (capacity <= 0) return;
    const data = encoder.encode(value || '').subarray(0, capacity - 1);
    bytes(ptr, data.length).set(data);
    bytes(ptr, capacity)[data.length] = 0;
  };

  // WebGL objects are JavaScript objects; the engine refers to them by number.
  const T = { shader: [null], program: [null], buffer: [null], vao: [null], texture: [null], fbo: [null], uniform: [null] };
  const add = (table, object) => { table.push(object); return table.length - 1; };
  const drop = (table, id, destroy) => { if (table[id]) { destroy(table[id]); table[id] = null; } };

  const gx = {
    gxViewport: (x, y, w, h) => gl.viewport(x, y, w, h),
    gxEnable: (cap) => gl.enable(cap),
    gxDisable: (cap) => gl.disable(cap),
    gxBlendFunc: (src, dst) => gl.blendFunc(src, dst),
    gxBlendEquation: (mode) => gl.blendEquation(mode),
    gxDepthFunc: (func) => gl.depthFunc(func),
    gxDepthMask: (on) => gl.depthMask(!!on),
    gxColorMask: (r, g, b, a) => gl.colorMask(!!r, !!g, !!b, !!a),
    gxClearColor: (r, g, b, a) => gl.clearColor(r, g, b, a),
    gxClearDepth: (depth) => gl.clearDepth(depth),
    gxClear: (mask) => gl.clear(mask),
    gxClearColorBuffer: (index, r, g, b, a) => gl.clearBufferfv(gl.COLOR, index, [r, g, b, a]),

    gxCreateShader: (type) => add(T.shader, gl.createShader(type)),
    gxShaderSource: (shader, ptr, len) => gl.shaderSource(T.shader[shader], text(ptr, len)),
    gxCompileShader: (shader, logPtr, capacity) => {
      const s = T.shader[shader];
      gl.compileShader(s);
      if (gl.getShaderParameter(s, gl.COMPILE_STATUS)) return 1;
      writeText(logPtr, capacity, gl.getShaderInfoLog(s));
      return 0;
    },
    gxDeleteShader: (shader) => drop(T.shader, shader, (s) => gl.deleteShader(s)),
    gxCreateProgram: () => add(T.program, gl.createProgram()),
    gxAttachShader: (program, shader) => gl.attachShader(T.program[program], T.shader[shader]),
    gxLinkProgram: (program, logPtr, capacity) => {
      const p = T.program[program];
      gl.linkProgram(p);
      if (gl.getProgramParameter(p, gl.LINK_STATUS)) return 1;
      writeText(logPtr, capacity, gl.getProgramInfoLog(p));
      return 0;
    },
    gxUseProgram: (program) => gl.useProgram(T.program[program]),
    gxDeleteProgram: (program) => drop(T.program, program, (p) => gl.deleteProgram(p)),
    gxGetUniformLocation: (program, ptr, len) => {
      const location = gl.getUniformLocation(T.program[program], text(ptr, len));
      return location ? add(T.uniform, location) : -1;
    },
    gxUniform1i: (l, v) => { if (l >= 0) gl.uniform1i(T.uniform[l], v); },
    gxUniform1f: (l, x) => { if (l >= 0) gl.uniform1f(T.uniform[l], x); },
    gxUniform2f: (l, x, y) => { if (l >= 0) gl.uniform2f(T.uniform[l], x, y); },
    gxUniform3f: (l, x, y, z) => { if (l >= 0) gl.uniform3f(T.uniform[l], x, y, z); },
    gxUniform4f: (l, x, y, z, w) => { if (l >= 0) gl.uniform4f(T.uniform[l], x, y, z, w); },

    gxCreateBuffer: () => add(T.buffer, gl.createBuffer()),
    gxBindBuffer: (target, buffer) => gl.bindBuffer(target, T.buffer[buffer]),
    gxBufferData: (target, ptr, len, usage) => gl.bufferData(target, bytes(ptr, len), usage),
    gxDeleteBuffer: (buffer) => drop(T.buffer, buffer, (b) => gl.deleteBuffer(b)),
    gxCreateVertexArray: () => add(T.vao, gl.createVertexArray()),
    gxBindVertexArray: (vao) => gl.bindVertexArray(T.vao[vao]),
    gxDeleteVertexArray: (vao) => drop(T.vao, vao, (v) => gl.deleteVertexArray(v)),
    gxEnableVertexAttribArray: (index) => gl.enableVertexAttribArray(index),
    gxVertexAttribPointer: (index, size, stride, offset) => gl.vertexAttribPointer(index, size, gl.FLOAT, false, stride, offset),
    gxVertexAttribDivisor: (index, divisor) => gl.vertexAttribDivisor(index, divisor),

    gxCreateTexture: () => add(T.texture, gl.createTexture()),
    gxBindTexture: (target, texture) => gl.bindTexture(target, T.texture[texture]),
    gxActiveTexture: (unit) => gl.activeTexture(unit),
    gxTexImage2D: (target, level, internalFormat, w, h, format, type, ptr, len) =>
      gl.texImage2D(target, level, internalFormat, w, h, 0, format, type, ptr ? bytes(ptr, len) : null),
    gxTexParameteri: (target, name, value) => gl.texParameteri(target, name, value),
    gxGenerateMipmap: (target) => gl.generateMipmap(target),
    gxDeleteTexture: (texture) => drop(T.texture, texture, (t) => gl.deleteTexture(t)),

    gxCreateFramebuffer: () => add(T.fbo, gl.createFramebuffer()),
    gxBindFramebuffer: (target, fbo) => gl.bindFramebuffer(target, T.fbo[fbo]),
    gxFramebufferTexture2D: (target, attachment, texTarget, texture, level) =>
      gl.framebufferTexture2D(target, attachment, texTarget, T.texture[texture], level),
    gxFramebufferComplete: (target) => (gl.checkFramebufferStatus(target) === gl.FRAMEBUFFER_COMPLETE ? 1 : 0),
    gxDrawBuffers: (count) => {
      if (count === 0) {
        gl.drawBuffers([gl.NONE]);
        gl.readBuffer(gl.NONE);
      } else {
        gl.drawBuffers(Array.from({ length: count }, (_, i) => gl.COLOR_ATTACHMENT0 + i));
      }
    },
    gxDeleteFramebuffer: (fbo) => drop(T.fbo, fbo, (f) => gl.deleteFramebuffer(f)),

    gxDrawArrays: (mode, first, count) => gl.drawArrays(mode, first, count),
    gxDrawArraysInstanced: (mode, first, count, instances) => gl.drawArraysInstanced(mode, first, count, instances),

    gxSupportsFloatTargets: () => (floatTargets ? 1 : 0),
    gxGetError: () => gl.getError(),
  };

  // The C library inside the engine only ever prints (stdout / stderr).
  let lineBuffer = '';
  const wasi = {
    fd_write: (fd, iov, iovCount, writtenPtr) => {
      const view = new DataView(memory.buffer);
      let written = 0;
      for (let i = 0; i < iovCount; ++i) {
        const ptr = view.getUint32(iov + i * 8, true);
        const len = view.getUint32(iov + i * 8 + 4, true);
        lineBuffer += text(ptr, len);
        written += len;
      }
      let newline;
      while ((newline = lineBuffer.indexOf('\n')) >= 0) {
        (fd === 2 ? console.warn : console.log)(lineBuffer.slice(0, newline));
        lineBuffer = lineBuffer.slice(newline + 1);
      }
      view.setUint32(writtenPtr, written, true);
      return 0;
    },
    fd_close: () => 0,
    fd_seek: () => 70,
    fd_fdstat_get: () => 0,
  };

  const levels = [console.debug, console.info, console.warn, console.error];
  const imports = {
    gx,
    wasi_snapshot_preview1: wasi,
    ge: { geLog: (level, ptr, len) => (levels[level] || console.log)('[G-Engine] ' + text(ptr, len)) },
  };

  const response = await fetch(wasmUrl);
  if (!response.ok) throw new Error('wasm-missing');
  const { instance } = await WebAssembly.instantiate(await response.arrayBuffer(), imports);
  const x = instance.exports;
  memory = x.memory;
  if (x._initialize) x._initialize();

  /// Calls an engine function that takes (pointer, length) for a string.
  const withText = (value, call) => {
    const data = encoder.encode(value);
    const ptr = x.ge_alloc(data.length + 1);
    bytes(ptr, data.length).set(data);
    bytes(ptr, data.length + 1)[data.length] = 0;
    try {
      return call(ptr, data.length);
    } finally {
      x.ge_free(ptr);
    }
  };

  return {
    gl,
    hdr: floatTargets,
    init: (width, height) => x.ge_init(width, height) === 1,
    resize: (width, height) => x.ge_resize(width, height),
    frame: (dt) => x.ge_frame(dt),
    button: (button, down) => x.ge_button(button, down ? 1 : 0),
    scripts: () => Array.from({ length: x.ge_script_count() }, (_, i) => ({ name: cstring(x.ge_script_name(i)), source: cstring(x.ge_script_source(i)) })),
    /// How the engine understands a script, without running it: {ok, notes:[{line,bn,en}], errors:[{line,col,bn,en}]}
    check: (source) => JSON.parse(withText(source, (ptr, len) => cstring(x.ge_check(ptr, len)))),
    /// Installs the script if it has no errors (and restarts the level). Same result shape as check().
    setScript: (index, source) => JSON.parse(withText(source, (ptr, len) => cstring(x.ge_set_script(index, ptr, len)))),
    resetScripts: () => x.ge_reset_scripts(),
    restart: () => x.ge_restart(),
    /// The level is a picture made of characters:  # wall  P player  G guard  c coin  ^ spikes  B crate  D door  L lamp
    level: () => cstring(x.ge_level()),
    /// Replaces the level picture and starts it; the scripts stay as they are.
    setLevel: (picture) => withText(picture, (ptr, len) => x.ge_set_level(ptr, len)),
    hud: () => JSON.parse(cstring(x.ge_hud())),
    option: (option, value) => x.ge_option(option, value),
  };
}
