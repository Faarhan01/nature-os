import React, {useCallback, useRef, useState} from 'react';
import {
  Activity,
  Camera,
  FolderTree,
  Gauge,
  HardDrive,
  Maximize2,
  Menu,
  Minimize2,
  Monitor,
  SlidersHorizontal,
  X,
} from 'lucide-react';
import {ArchitectureDossier} from './ArchitectureDossier';
import {CanvasHandle, OSDisplayCanvas, OSStats} from './OSDisplayCanvas';
import {SiteSettings} from './SiteSettings';
import {loadSiteSettings, saveSiteSettings, SiteSettings as SiteSettingsState} from './siteSettings';
import {SourceExplorer} from './SourceExplorer';
import {StressLab} from './StressLab';

type ViewMode = 'display' | 'stress' | 'architecture' | 'source' | 'settings';

interface NavItem {
  id: ViewMode;
  label: string;
  hint: string;
  icon: React.ComponentType<{className?: string}>;
}

const NAV_ITEMS: NavItem[] = [
  {id: 'display', label: 'Display', hint: 'Live interactive desktop', icon: Monitor},
  {id: 'stress', label: 'Stress Lab', hint: 'Multi-core & frame telemetry', icon: Activity},
  {id: 'architecture', label: 'Architecture', hint: 'Ubuntu Noble & C17 stack', icon: HardDrive},
  {id: 'source', label: 'Source', hint: 'Browse /nature-os tree', icon: FolderTree},
  {id: 'settings', label: 'Settings', hint: 'Touch keyboard & display', icon: SlidersHorizontal},
];

export default function App() {
  const [view, setView] = useState<ViewMode>('display');
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);
  const [isOsFullscreen, setIsOsFullscreen] = useState(false);
  const [inspectPath, setInspectPath] = useState<string>('shell/src/shell.c');
  const [stats, setStats] = useState<OSStats>({
    fps: 60.0,
    ramMb: 420,
    cpuPct: 1.2,
    tileCount: 0,
    totalRamMb: 4096,
    osRssMb: 28,
    frameMs: 2.0,
    cores: [1, 1, 1, 1],
  });
  const [siteSettings, setSiteSettings] = useState<SiteSettingsState>(() => loadSiteSettings());
  const cmdSenderRef = useRef<((cmd: string) => void) | null>(null);
  const canvasRef = useRef<CanvasHandle>(null);

  const registerCommandSender = useCallback((fn: (cmd: string) => void) => {
    cmdSenderRef.current = fn;
  }, []);

  const handleStatsUpdate = useCallback((newStats: OSStats) => {
    setStats(newStats);
  }, []);

  const handleSettingsChange = useCallback((next: SiteSettingsState) => {
    setSiteSettings(next);
    saveSiteSettings(next);
  }, []);

  const handleFullscreenChange = useCallback((active: boolean) => {
    setIsOsFullscreen(active);
  }, []);

  const sendCommand = (cmd: string) => {
    cmdSenderRef.current?.(cmd);
  };

  const selectView = (nextView: ViewMode) => {
    setView(nextView);
    setMobileMenuOpen(false);
    if (nextView === 'display') {
      requestAnimationFrame(() => window.dispatchEvent(new Event('nature-focus-canvas')));
    }
  };

  const handleFullscreenClick = (e: React.MouseEvent<HTMLButtonElement>) => {
    e.currentTarget.blur();
    setMobileMenuOpen(false);
    if (view !== 'display') {
      setView('display');
      requestAnimationFrame(() => canvasRef.current?.enterFullscreen());
    } else {
      canvasRef.current?.enterFullscreen();
    }
  };

  const openSourceFile = (path: string) => {
    setInspectPath(path);
    setView('source');
    setMobileMenuOpen(false);
  };

  const isStressed = stats.cpuPct > 25;
  const activeNav = NAV_ITEMS.find((item) => item.id === view) ?? NAV_ITEMS[0];

  return (
    <div className="flex h-dvh w-screen flex-col md:flex-row bg-[#070B09] text-[#E8EFEA] overflow-hidden select-none">
      {/* Desktop Left Vertical Navigation Rail */}
      <aside className="hidden md:flex md:w-56 lg:w-60 shrink-0 flex-col justify-between border-r border-white/10 bg-[#0A0F0D]">
        <div className="flex flex-col">
          {/* Brand Block */}
          <div className="flex items-center gap-3 border-b border-white/10 px-4 py-3.5">
            <div className="flex h-8 w-8 shrink-0 items-center justify-center rounded-lg bg-emerald-500/15 border border-emerald-500/30 font-display text-sm font-bold text-emerald-400">
              N
            </div>
            <div className="min-w-0">
              <div className="flex items-center gap-2">
                <span className="font-display text-[15px] font-bold tracking-tight text-white truncate">
                  Nature OS
                </span>
              </div>
              <div className="font-mono text-[10px] text-emerald-400/90 truncate">
                24.04 Noble · C17/C++20
              </div>
            </div>
          </div>

          {/* Vertical Page Options */}
          <nav className="flex flex-col gap-1 p-3" aria-label="Primary Navigation">
            <div className="px-2.5 pb-1.5 font-mono text-[10px] uppercase tracking-wider text-neutral-500">
              Workspace
            </div>
            {NAV_ITEMS.map((item) => {
              const Icon = item.icon;
              const active = view === item.id;
              return (
                <button
                  key={item.id}
                  type="button"
                  onClick={(e) => {
                    e.currentTarget.blur();
                    selectView(item.id);
                  }}
                  className={`relative flex w-full items-center gap-3 rounded-xl px-3 py-2.5 text-left transition-colors min-h-[44px] ${
                    active
                      ? 'bg-emerald-500/12 text-white border border-emerald-500/25'
                      : 'text-neutral-400 border border-transparent hover:bg-white/[0.04] hover:text-neutral-100'
                  }`}
                >
                  {active && (
                    <span className="absolute left-0 top-2.5 bottom-2.5 w-[3px] rounded-r bg-emerald-400" />
                  )}
                  <Icon
                    className={`h-4 w-4 shrink-0 ${
                      active ? 'text-emerald-400' : 'text-neutral-400'
                    }`}
                  />
                  <div className="min-w-0 flex-1">
                    <div className="flex items-center justify-between gap-1.5">
                      <span className="text-[13px] font-semibold tracking-tight truncate">
                        {item.label}
                      </span>
                      {item.id === 'stress' && isStressed && (
                        <span className="font-mono text-[9px] font-semibold uppercase tracking-wider text-red-400 shrink-0">
                          LOAD
                        </span>
                      )}
                    </div>
                    <div className="text-[10.5px] text-neutral-500 truncate">{item.hint}</div>
                  </div>
                </button>
              );
            })}
          </nav>
        </div>

        {/* Bottom Live Runtime Telemetry Card */}
        <div className="p-3 border-t border-white/10">
          <div className="rounded-xl border border-white/10 bg-black/35 p-3 space-y-2.5">
            <div className="flex items-center justify-between">
              <span className="font-mono text-[10px] uppercase tracking-wider text-neutral-400">
                Compositor Stream
              </span>
              <span className="flex items-center gap-1 font-mono text-[11px] tabular-nums text-emerald-400">
                <Gauge className="h-3 w-3" />
                {stats.fps > 0 ? stats.fps.toFixed(0) : '60'} FPS
              </span>
            </div>

            <div className="grid grid-cols-2 gap-2 text-[11px]">
              <div className="rounded-lg bg-white/[0.03] px-2.5 py-1.5 border border-white/5">
                <div className="text-[10px] text-neutral-500">OS RSS</div>
                <div className="font-mono font-semibold tabular-nums text-neutral-200">
                  {stats.osRssMb ?? 28} MiB
                </div>
              </div>
              <div className="rounded-lg bg-white/[0.03] px-2.5 py-1.5 border border-white/5">
                <div className="text-[10px] text-neutral-500">CPU Load</div>
                <div
                  className={`font-mono font-semibold tabular-nums ${
                    isStressed ? 'text-red-400' : 'text-emerald-300'
                  }`}
                >
                  {stats.cpuPct.toFixed(1)}%
                </div>
              </div>
            </div>
            <div className="flex items-center justify-between text-[10px] font-mono text-neutral-500 tabular-nums">
              <span>RAM {stats.ramMb}/{stats.totalRamMb ?? 4096}M</span>
              <span>{(stats.frameMs ?? 2.0).toFixed(1)} ms</span>
            </div>

            <div className="h-1.5 w-full overflow-hidden rounded-full bg-white/10">
              <div
                className={`h-full transition-all duration-300 ${
                  isStressed ? 'bg-red-400' : 'bg-emerald-500'
                }`}
                style={{width: `${Math.min(100, Math.max(4, stats.cpuPct))}%`}}
              />
            </div>
          </div>
        </div>
      </aside>

      {/* Main Content Column (Mobile Header + Desktop Header + Active View) */}
      <div className="relative flex flex-1 flex-col min-w-0 min-h-0">
        {/* Mobile Header: Brand on Left, Expand + Mobile Menu Buttons together on Right */}
        <header className="relative z-30 flex md:hidden h-12 shrink-0 items-center justify-between gap-2 border-b border-white/10 bg-[#0B100E] px-3">
          <div className="flex items-center gap-2 min-w-0">
            <div className="flex h-7 w-7 shrink-0 items-center justify-center rounded-lg bg-emerald-500/20 border border-emerald-500/30 font-display text-xs font-bold text-emerald-400">
              N
            </div>
            <div className="flex items-center gap-1.5 min-w-0">
              <span className="font-display text-sm font-bold tracking-tight text-white truncate">
                Nature OS
              </span>
              <span className="text-neutral-600">/</span>
              <span className="text-xs font-medium text-emerald-400 truncate">
                {activeNav.label}
              </span>
            </div>
          </div>

          <div className="flex items-center gap-2 shrink-0">
            <button
              type="button"
              onClick={handleFullscreenClick}
              className="flex items-center gap-1.5 rounded-lg bg-[#2E8B57] px-3 py-1.5 text-xs font-semibold text-white hover:bg-[#257347] transition-colors min-h-[38px] shadow-md"
              title={isOsFullscreen ? 'Exit OS Fullscreen' : 'Expand OS Fullscreen'}
            >
              {isOsFullscreen ? (
                <Minimize2 className="h-4 w-4" />
              ) : (
                <Maximize2 className="h-4 w-4" />
              )}
              <span>{isOsFullscreen ? 'Exit' : 'Expand'}</span>
            </button>

            <button
              type="button"
              onClick={() => setMobileMenuOpen((open) => !open)}
              aria-expanded={mobileMenuOpen}
              aria-label={mobileMenuOpen ? 'Close navigation menu' : 'Open navigation menu'}
              className={`flex items-center justify-center rounded-lg border px-2.5 py-1.5 transition-colors min-h-[38px] min-w-[38px] ${
                mobileMenuOpen
                  ? 'border-emerald-500/50 bg-emerald-500/15 text-emerald-300'
                  : 'border-white/15 bg-white/[0.04] text-neutral-200 hover:bg-white/[0.08]'
              }`}
            >
              {mobileMenuOpen ? <X className="h-4 w-4" /> : <Menu className="h-4 w-4" />}
            </button>
          </div>
        </header>

        {/* Mobile Dropdown Menu Drawer */}
        {mobileMenuOpen && (
          <>
            <div
              className="fixed inset-0 top-12 z-20 bg-black/60 backdrop-blur-xs md:hidden"
              onClick={() => setMobileMenuOpen(false)}
            />
            <div className="absolute top-12 inset-x-0 z-30 border-b border-white/15 bg-[#0B110F] p-3 shadow-2xl md:hidden">
              <nav className="flex flex-col gap-1" aria-label="Mobile Navigation">
                {NAV_ITEMS.map((item) => {
                  const Icon = item.icon;
                  const active = view === item.id;
                  return (
                    <button
                      key={item.id}
                      type="button"
                      onClick={(e) => {
                        e.currentTarget.blur();
                        selectView(item.id);
                      }}
                      className={`flex w-full items-center justify-between rounded-xl px-3.5 py-2.5 text-left transition-colors min-h-[46px] ${
                        active
                          ? 'bg-emerald-500/15 text-white border border-emerald-500/30'
                          : 'text-neutral-300 border border-transparent hover:bg-white/[0.05]'
                      }`}
                    >
                      <div className="flex items-center gap-3">
                        <Icon
                          className={`h-4 w-4 ${active ? 'text-emerald-400' : 'text-neutral-400'}`}
                        />
                        <div>
                          <div className="text-sm font-semibold">{item.label}</div>
                          <div className="text-[11px] text-neutral-500">{item.hint}</div>
                        </div>
                      </div>
                      {item.id === 'stress' && isStressed && (
                        <span className="font-mono text-[10px] font-semibold uppercase tracking-wider text-red-400 shrink-0">
                          LOAD
                        </span>
                      )}
                    </button>
                  );
                })}
              </nav>

              <div className="mt-3 flex items-center justify-between rounded-xl border border-white/10 bg-black/40 px-3.5 py-2 text-xs font-mono">
                <span className="text-emerald-400 tabular-nums">
                  {stats.fps > 0 ? stats.fps.toFixed(0) : '60'} FPS
                </span>
                <span className="text-neutral-300 tabular-nums">{stats.ramMb} MiB</span>
                <span className={`tabular-nums ${isStressed ? 'text-red-400' : 'text-neutral-400'}`}>
                  CPU {stats.cpuPct.toFixed(1)}%
                </span>
              </div>
            </div>
          </>
        )}

        {/* Desktop Compact Top Utility Bar */}
        <header className="hidden md:flex h-12 shrink-0 items-center justify-between border-b border-white/10 bg-[#0B100E] px-5">
          <div className="flex items-center gap-2.5">
            <span className="font-display text-sm font-semibold text-white">{activeNav.label}</span>
            <span className="text-neutral-600">·</span>
            <span className="text-xs text-neutral-400">{activeNav.hint}</span>
          </div>

          <div className="flex items-center gap-2 shrink-0">
            {view === 'display' && (
              <button
                type="button"
                onClick={(e) => {
                  e.currentTarget.blur();
                  canvasRef.current?.takeScreenshot();
                  window.dispatchEvent(new Event('nature-focus-canvas'));
                }}
                className="flex items-center gap-1.5 rounded-lg border border-white/10 bg-white/[0.03] px-3 py-1.5 text-xs font-medium text-neutral-300 hover:bg-white/[0.07] hover:text-white transition-colors min-h-[34px] whitespace-nowrap shrink-0"
                title="Save desktop screenshot"
              >
                <Camera className="h-3.5 w-3.5" />
                <span>Capture</span>
              </button>
            )}
            <button
              type="button"
              onClick={handleFullscreenClick}
              className="flex items-center gap-1.5 rounded-lg bg-[#2E8B57] px-3.5 py-1.5 text-xs font-semibold text-white hover:bg-[#257347] transition-colors min-h-[34px] shadow-md whitespace-nowrap shrink-0"
              title="Make the Nature OS screen fullscreen"
            >
              {isOsFullscreen ? (
                <Minimize2 className="h-3.5 w-3.5" />
              ) : (
                <Maximize2 className="h-3.5 w-3.5" />
              )}
              <span>{isOsFullscreen ? 'Exit Fullscreen' : 'Fullscreen'}</span>
            </button>
          </div>
        </header>

        {/* Main Viewport Stage */}
        <main className="relative flex-1 overflow-hidden flex flex-col min-h-0">
          {/* Keep OSDisplayCanvas mounted across tab switches so WebSocket compositor stream stays warm */}
          <div className={view === 'display' ? 'relative flex-1 flex flex-col min-h-0' : 'hidden'}>
            <OSDisplayCanvas
              ref={canvasRef}
              visible={view === 'display'}
              settings={siteSettings}
              registerCommandSender={registerCommandSender}
              onStatsUpdate={handleStatsUpdate}
              onFullscreenChange={handleFullscreenChange}
            />
          </div>

          {view === 'stress' && (
            <StressLab
              stats={stats}
              sendCommand={sendCommand}
              onSwitchToDisplay={() => selectView('display')}
            />
          )}

          {view === 'architecture' && (
            <ArchitectureDossier onOpenSourceFile={openSourceFile} />
          )}

          {view === 'source' && <SourceExplorer initialFile={inspectPath} />}

          {view === 'settings' && (
            <SiteSettings
              settings={siteSettings}
              onChange={handleSettingsChange}
              onReturnToDisplay={() => selectView('display')}
            />
          )}
        </main>
      </div>
    </div>
  );
}
