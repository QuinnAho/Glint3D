// Minimal viewer SDK wrapping Emscripten Module calls

declare global {
  interface Window { Module: any }
}

function waitForModuleReady(timeoutMs = 10000): Promise<void> {
  const start = Date.now()
  return new Promise((resolve, reject) => {
    const tick = () => {
      const M = (window as any).Module
      if (M && (M.calledRun || M.onRuntimeInitialized == null)) return resolve()
      if (Date.now() - start > timeoutMs) return reject(new Error('Engine Module timeout'))
      setTimeout(tick, 50)
    }
    tick()
  })
}

export async function init(canvas: HTMLCanvasElement) {
  // Bind canvas for Emscripten GL
  ;(window as any).Module = (window as any).Module || {}
  window.Module.canvas = canvas
  await waitForModuleReady()
}

export function applyOps(ops: any | any[]): boolean {
  const M = window.Module
  if (!M || !M.ccall) { console.error('Module not ready'); return false }
  const payload = Array.isArray(ops) ? ops : [ops]
  const json = JSON.stringify(payload)
  const ok = M.ccall('app_apply_ops_json', 'number', ['string'], [json])
  return ok === 1
}

export function shareLink(): string {
  const M = window.Module
  if (!M || !M.cwrap) return ''
  const getLink = M.cwrap('app_share_link', 'string', [])
  return getLink() || ''
}

export async function mountFileAndLoad(file: File, onLog?: (s: string)=>void) {
  await waitForModuleReady()
  try {
    const arr = new Uint8Array(await file.arrayBuffer())
    const name = Date.now() + '_' + file.name
    const dst = '/uploads/' + name
    try { window.Module.FS_createPath('/', 'uploads', true, true) } catch {}
    window.Module.FS_createDataFile(dst, null, arr, true, true, true)
    const baseName = file.name.replace(/\.[^.]+$/, '')
    const ok = applyOps({ op: 'load', path: dst, name: baseName })
    onLog?.(ok ? `Loaded ${file.name}` : `Failed to load: ${file.name}`)
  } catch (e: any) {
    onLog?.(`Import error: ${e?.message || e}`)
  }
}

export type SceneSnapshot = {
  camera: { position: number[], front: number[] }
  selected: string | null
  objects: { name: string }[]
  lights: { position: number[], color: number[], intensity: number }[]
}

export function getScene(): SceneSnapshot | null {
  const M = window.Module
  if (!M || !M.cwrap) return null
  const getJson = M.cwrap('app_scene_to_json', 'string', [])
  const s = getJson()
  if (!s) return null
  try { return JSON.parse(s) } catch { return null }
}
