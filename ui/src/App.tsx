import { useEffect, useRef, useState } from 'react'
import { CanvasHost } from './components/CanvasHost'
import { Console } from './components/Console'
import { Inspector } from './components/Inspector'

export default function App() {
  const [logs, setLogs] = useState<string[]>([
    'Welcome — ask me anything in the Console below.',
  ])
  const pushLog = (s: string) => setLogs(l => [...l, s])

  return (
    <div className="app-grid">
      <div className="area-canvas border-r border-neutral-800">
        <CanvasHost onLog={pushLog} />
      </div>
      <div className="area-console border-t border-neutral-800">
        <Console logs={logs} onLog={pushLog} />
      </div>
      <div className="area-inspector border-l border-neutral-800">
        <Inspector onLog={pushLog} />
      </div>
    </div>
  )
}

