// Minimal embed SDK for PromptGL/OpenGLOBJViewer
// Usage:
//   import { createViewer } from './embed.js';
//   const v = createViewer('#app', { src: 'assets/models/cube.obj', viewerUrl: '../index.html' });
//   v.applyOps([{ op: 'add_light', type: 'key' }]);

function encodeState(ops) {
  const payload = JSON.stringify({ ops });
  // Standard Base64; JSON is ASCII-safe
  return btoa(payload);
}

function normalizeOps(ops) {
  // Map convenience types (e.g., type:'key') to valid JSON Ops v1
  return (ops || []).map(op => {
    if (op && op.op === 'add_light' && op.type === 'key') {
      return { op: 'add_light', type: 'point', position: [2, 2, 2], color: [1, 1, 1], intensity: 1.0 };
    }
    return op;
  });
}

export function createViewer(target, options = {}) {
  const el = (typeof target === 'string') ? document.querySelector(target) : target;
  if (!el) throw new Error('createViewer: target not found');

  const viewerUrl = options.viewerUrl || '../index.html';
  const opsHistory = [];

  // Initial load op from src
  if (options.src) {
    opsHistory.push({ op: 'load', path: options.src, name: options.name || options.src });
  }

  // Create iframe and attach
  const iframe = document.createElement('iframe');
  iframe.style.width = '100%';
  iframe.style.height = '100%';
  iframe.style.border = '0';
  el.appendChild(iframe);

  function refresh() {
    const state = encodeState(opsHistory);
    const url = viewerUrl + (viewerUrl.includes('?') ? '&' : '?') + 'state=' + encodeURIComponent(state);
    iframe.src = url;
  }

  refresh();

  return {
    applyOps(newOps) {
      const ops = normalizeOps(newOps);
      opsHistory.push(...ops);
      refresh();
    },
    element: iframe,
    getStateOps() { return opsHistory.slice(); }
  };
}

