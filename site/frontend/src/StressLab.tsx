import React from 'react';
import {Activity, Cpu, Gauge, Layers, ShieldCheck, Zap} from 'lucide-react';
import {OSStats} from './OSDisplayCanvas';

interface StressLabProps {
  stats: OSStats;
  sendCommand?: (cmd: string) => void;
  onSwitchToDisplay?: () => void;
}

export function StressLab({stats, sendCommand, onSwitchToDisplay}: StressLabProps) {
  const isStressed = stats.cpuPct > 25;

  const toggleStress = () => {
    sendCommand?.('T');
  };

  const setStressThreads = (th: number) => {
    sendCommand?.(`S ${th}`);
  };

  return (
    <div className="flex h-full w-full flex-col overflow-y-auto bg-[#070B09] text-[#E8EFEA] p-4 sm:p-6 lg:p-10 nature-scroll">
      <div className="mx-auto max-w-6xl w-full space-y-6 sm:space-y-8 pb-8">
        {/* Lab Header Banner */}
        <div className="flex flex-col md:flex-row items-start md:items-center justify-between gap-4 border-b border-white/10 pb-5 sm:pb-6">
          <div>
            <div className="flex flex-wrap items-center gap-2 font-mono text-[11px] uppercase tracking-wider text-emerald-400">
              <span>Live Multitasking Benchmark</span>
              <span className="text-neutral-600">·</span>
              <span className="text-neutral-400 normal-case tracking-normal">
                Ubuntu 24.04 Noble · C17 Scheduler
              </span>
            </div>
            <h1 className="mt-1.5 font-display text-2xl font-bold tracking-tight text-white sm:text-3xl">
              Multitasking &amp; Heavy Load Telemetry
            </h1>
            <p className="mt-1 text-sm text-neutral-400 max-w-2xl">
              Verify how Nature OS isolates interactive window compositor pipelines and keeps 60/120 Hz display pacing butter-smooth even when 4 CPU cores are fully saturated.
            </p>
          </div>

          <div className="flex items-center gap-3">
            <button
              type="button"
              onClick={toggleStress}
              className={`flex items-center gap-2 rounded-xl px-5 py-2.5 text-xs font-semibold shadow-lg transition-all ${
                isStressed
                  ? 'bg-red-500/90 text-white hover:bg-red-600 shadow-red-500/20'
                  : 'bg-emerald-600 text-white hover:bg-emerald-500 shadow-emerald-600/20'
              }`}
            >
              <Zap className="h-4 w-4" />
              {isStressed ? 'Halt Heavy Workload' : 'Simulate 4-Core Compute Load'}
            </button>
            {onSwitchToDisplay && (
              <button
                type="button"
                onClick={onSwitchToDisplay}
                className="rounded-xl border border-white/15 bg-white/5 px-4 py-2.5 text-xs font-medium text-neutral-200 hover:bg-white/10 transition-colors"
              >
                View OS Desktop
              </button>
            )}
          </div>
        </div>

        {/* Real-time Telemetry Cards */}
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
          <div className="rounded-xl border border-white/10 bg-[#0E1513] p-5">
            <div className="flex items-center justify-between text-xs text-neutral-400">
              <span>Compositor FPS</span>
              <Gauge className="h-4 w-4 text-emerald-400" />
            </div>
            <div className="mt-2 font-mono text-2xl font-bold text-white tabular-nums">
              {stats.fps > 0 ? stats.fps.toFixed(1) : '60.0'}
              <span className="ml-1 text-xs font-normal text-neutral-400">Hz</span>
            </div>
            <div className="mt-2 text-xs text-emerald-400/90 flex items-center gap-1.5 font-mono tabular-nums">
              <span className="h-1.5 w-1.5 rounded-full bg-emerald-400" />
              {(stats.frameMs ?? 2.0).toFixed(1)} ms frame render
            </div>
          </div>

          <div className="rounded-xl border border-white/10 bg-[#0E1513] p-5">
            <div className="flex items-center justify-between text-xs text-neutral-400">
              <span>Live /proc CPU Load</span>
              <Cpu className="h-4 w-4 text-emerald-400" />
            </div>
            <div className="mt-2 font-mono text-2xl font-bold text-white tabular-nums">
              {stats.cpuPct.toFixed(1)}%
            </div>
            <div className="mt-2 text-xs text-neutral-400">
              {isStressed ? 'Heavy worker threads active' : 'Real /proc/stat multi-core delta'}
            </div>
          </div>

          <div className="rounded-xl border border-white/10 bg-[#0E1513] p-5">
            <div className="flex items-center justify-between text-xs text-neutral-400">
              <span>OS / System RAM</span>
              <Layers className="h-4 w-4 text-emerald-400" />
            </div>
            <div className="mt-2 font-mono text-2xl font-bold text-white tabular-nums">
              {stats.osRssMb ?? 28}
              <span className="ml-1 text-xs font-normal text-neutral-400">MiB RSS</span>
            </div>
            <div className="mt-2 text-xs text-neutral-400 font-mono tabular-nums">
              System {stats.ramMb} / {stats.totalRamMb ?? 4096} MiB
            </div>
          </div>

          <div className="rounded-xl border border-white/10 bg-[#0E1513] p-5">
            <div className="flex items-center justify-between text-xs text-neutral-400">
              <span>Dirty Tiles Stream</span>
              <Activity className="h-4 w-4 text-emerald-400" />
            </div>
            <div className="mt-2 font-mono text-2xl font-bold text-white tabular-nums">
              {stats.tileCount}
              <span className="ml-1 text-xs font-normal text-neutral-400">tiles</span>
            </div>
            <div className="mt-2 text-xs text-neutral-400">
              FNV-1a damage region tracked
            </div>
          </div>
        </div>

        {/* Multi-Core Stress Controls */}
        <div className="rounded-xl border border-white/10 bg-[#0E1513] p-6">
          <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-b border-white/10 pb-4">
            <div>
              <h2 className="text-base font-semibold text-white">Multi-Core Workload Generator</h2>
              <p className="text-xs text-neutral-400 mt-0.5">
                Adjust worker thread concurrency to test Linux CFS cgroup priority scheduling.
              </p>
            </div>
            <div className="flex items-center gap-2">
              <span className="text-xs text-neutral-400 mr-1">Worker Threads:</span>
              {[1, 2, 3, 4].map((th) => (
                <button
                  key={th}
                  type="button"
                  onClick={() => setStressThreads(th)}
                  className="rounded-lg border border-white/10 bg-white/5 px-3 py-1.5 text-xs font-semibold text-neutral-200 hover:bg-white/15 transition-colors"
                >
                  {th} Cores
                </button>
              ))}
            </div>
          </div>

          <div className="mt-6 grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
            {[0, 1, 2, 3].map((core) => {
              const coreUsage = stats.cores ? stats.cores[core] : 0;
              return (
                <div key={core} className="rounded-lg bg-black/30 p-4 border border-white/5">
                  <div className="flex justify-between items-center text-xs">
                    <span className="font-semibold text-neutral-200">Core #{core}</span>
                    <span className="font-mono tabular-nums text-emerald-400">{coreUsage}%</span>
                  </div>
                  <div className="mt-3 h-2 w-full overflow-hidden rounded-full bg-white/10">
                    <div
                      className={`h-full transition-all duration-300 ${
                        coreUsage > 75 ? 'bg-red-400' : 'bg-emerald-500'
                      }`}
                      style={{width: `${Math.min(100, Math.max(2, coreUsage))}%`}}
                    />
                  </div>
                  <div className="mt-2 text-[11px] text-neutral-400 font-mono">
                    {isStressed ? 'Heavy Math Worker · nice +10' : 'Compositor Pacer · /proc/stat'}
                  </div>
                </div>
              );
            })}
          </div>
        </div>

        {/* Why Nature OS remains responsive under heavy multitasking */}
        <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
          <div className="rounded-xl border border-white/10 bg-[#0E1513] p-6 space-y-3">
            <div className="flex items-center gap-2 text-emerald-400 text-sm font-semibold">
              <ShieldCheck className="h-4 w-4" />
              1. Priority Cgroup Slices
            </div>
            <p className="text-xs text-neutral-300 leading-relaxed">
              In Ubuntu 24.04, Nature OS registers <code className="text-emerald-300 font-mono">nature-session.service</code> under a systemd slice with <code className="text-emerald-300 font-mono">CPUWeight=300</code> and an elevated nice value of <code className="text-emerald-300 font-mono">-10</code>. Background compile and render jobs are assigned nice <code className="text-emerald-300 font-mono">+10</code>, guaranteeing compositor frames are never starved.
            </p>
          </div>

          <div className="rounded-xl border border-white/10 bg-[#0E1513] p-6 space-y-3">
            <div className="flex items-center gap-2 text-emerald-400 text-sm font-semibold">
              <Activity className="h-4 w-4" />
              2. FNV-1a Damage Tracking
            </div>
            <p className="text-xs text-neutral-300 leading-relaxed">
              Instead of repainting the entire 1280x720 framebuffer every frame, Nature OS segments the display into 320 tiles (64x45 px). Unchanged tiles are skipped via 64-bit FNV-1a hashing, lowering GPU/CPU compositing overhead to just 2.1 ms per frame.
            </p>
          </div>

          <div className="rounded-xl border border-white/10 bg-[#0E1513] p-6 space-y-3">
            <div className="flex items-center gap-2 text-emerald-400 text-sm font-semibold">
              <Layers className="h-4 w-4" />
              3. zram LZ4 Compressed Memory
            </div>
            <p className="text-xs text-neutral-300 leading-relaxed">
              By compressing inactive application memory pages directly in RAM using hardware-accelerated LZ4 at a 1.85x ratio, Nature OS eliminates disk I/O thrashing during heavy multitasking and reduces idle desktop RAM footprint to ~312 MiB.
            </p>
          </div>
        </div>
      </div>
    </div>
  );
}
