import { useEffect, useMemo, useState } from 'react'
import { applyOps, getScene, SceneSnapshot } from '../sdk/viewer'

export function Inspector({ onLog }: { onLog: (s: string)=>void }) {
  const [scene, setScene] = useState<SceneSnapshot | null>(null)

  function refresh() {
    const s = getScene()
    setScene(s)
  }

  useEffect(() => {
    const id = setInterval(refresh, 1000)
    return () => clearInterval(id)
  }, [])

  function addLight() {
    const ok = applyOps({ op: 'add_light', type: 'point', intensity: 1.0 })
    onLog(ok ? 'Added light' : 'Add light failed')
  }

  function setRenderMode(mode: 'point'|'wire'|'solid'|'raytrace') {
    const ok = applyOps({ op: 'set_render_mode', mode })
    onLog(ok ? `Render mode: ${mode}` : 'Set render mode failed')
  }

  const selectedName = scene?.selected ?? null
  const selectedLight = (scene && scene.selectedLightIndex != null && scene.selectedLightIndex >= 0) ? scene.selectedLightIndex : -1

  function selectObject(name: string) {
    const ok = applyOps({ op: 'select', type: 'object', target: name })
    if (!ok) onLog('Select object failed')
  }
  function selectLight(i: number) {
    const ok = applyOps({ op: 'select', type: 'light', index: i })
    if (!ok) onLog('Select light failed')
  }
  function removeSelected() {
    let ok = false
    if (selectedName) ok = applyOps({ op: 'remove', type: 'object', target: selectedName })
    else if (selectedLight >= 0) ok = applyOps({ op: 'remove', type: 'light', index: selectedLight })
    onLog(ok ? 'Removed selected' : 'Remove failed')
  }
  function duplicateSelected() {
    let ok = false
    if (selectedName) ok = applyOps({ op: 'duplicate', source: selectedName })
    else if (selectedLight >= 0) ok = applyOps({ op: 'duplicate_light', index: selectedLight })
    onLog(ok ? 'Duplicated selected' : 'Duplicate failed')
  }
  function nudge(dx:number,dy:number,dz:number) {
    if (!selectedName) return onLog('Select an object to transform')
    const ok = applyOps({ op: 'transform', target: selectedName, mode: 'delta', transform_delta: { position: [dx,dy,dz] } })
    onLog(ok ? 'Moved object' : 'Move failed')
  }

  return (
    <div className="h-full p-3 text-sm flex flex-col gap-3">
      <div className="flex items-center justify-between">
        <div className="font-semibold">Inspector</div>
        <button className="bg-neutral-700 rounded px-2 py-1" onClick={refresh}>Refresh</button>
      </div>
      <div className="text-neutral-300">
        {scene ? (
          <>
            <div>Objects: {scene.objects?.length ?? 0}</div>
            <div>Lights: {scene.lights?.length ?? 0}</div>
            <div className="mt-2">
              <div className="font-medium">Object Names</div>
              <ul className="list-disc ml-5 max-h-28 overflow-auto">
                {scene.objects?.map((o, i) => (
                  <li key={i}>
                    <button className={`hover:underline ${selectedName===o.name? 'text-blue-400':''}`} onClick={()=>selectObject(o.name)}>{o.name}</button>
                  </li>
                ))}
              </ul>
            </div>
            <div className="mt-2">
              <div className="font-medium">Lights</div>
              <ul className="list-disc ml-5 max-h-28 overflow-auto">
                {scene.lights?.map((L, i) => (
                  <li key={i}>
                    <button className={`hover:underline ${selectedLight===i? 'text-blue-400':''}`} onClick={()=>selectLight(i)}>Light {i}</button>
                    <span className="text-neutral-500"> &nbsp;I={L.intensity.toFixed?.(2) ?? L.intensity}</span>
                  </li>
                ))}
              </ul>
            </div>
          </>
        ) : (
          <div className="text-neutral-500">No scene yet.</div>
        )}
      </div>
      <div className="mt-auto flex flex-col gap-2">
        <div className="font-medium">Actions</div>
        <div className="flex gap-2 flex-wrap">
          <button className="bg-neutral-700 rounded px-2 py-1" onClick={addLight}>Add Light</button>
          <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>setRenderMode('solid')}>Solid</button>
          <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>setRenderMode('wire')}>Wire</button>
          <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>setRenderMode('raytrace')}>Ray</button>
          <button className="bg-red-700 rounded px-2 py-1" onClick={removeSelected}>Delete Selected</button>
          <button className="bg-neutral-700 rounded px-2 py-1" onClick={duplicateSelected}>Duplicate Selected</button>
        </div>
        <div className="mt-2 flex gap-2 flex-wrap">
          <div className="text-neutral-400">Nudge:</div>
          <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>nudge( 0.1, 0, 0)}>+X</button>
          <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>nudge(-0.1, 0, 0)}>-X</button>
          <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>nudge(0, 0.1, 0)}>+Y</button>
          <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>nudge(0,-0.1, 0)}>-Y</button>
          <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>nudge(0, 0, 0.1)}>+Z</button>
          <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>nudge(0, 0,-0.1)}>-Z</button>
        </div>
      </div>
    </div>
  )
}
