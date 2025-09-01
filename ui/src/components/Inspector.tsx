import { useEffect, useState } from 'react'
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
              <ul className="list-disc ml-5 max-h-40 overflow-auto">
                {scene.objects?.map((o, i) => <li key={i}>{o.name}</li>)}
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
        </div>
      </div>
    </div>
  )
}
