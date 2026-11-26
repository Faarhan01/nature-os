import React, {forwardRef, useCallback, useEffect, useImperativeHandle, useRef, useState} from 'react';
import {unzlibSync} from 'fflate';
import {CornerDownLeft, Delete, Keyboard, Maximize2, Minimize2, X} from 'lucide-react';
import {DEFAULT_SITE_SETTINGS, SiteSettings} from './siteSettings';

export interface CanvasHandle {
  takeScreenshot: () => void;
  requestKeyframe: () => void;
  enterFullscreen: () => void;
}

export interface OSStats {
  fps: number;
  ramMb: number;
  cpuPct: number;
  tileCount: number;
  totalRamMb?: number;
  osRssMb?: number;
  frameMs?: number;
  cores?: [number, number, number, number];
}

interface OSDisplayCanvasProps {
  visible?: boolean;
  settings?: SiteSettings;
  registerCommandSender?: (fn: (cmd: string) => void) => void;
  onStatsUpdate?: (stats: OSStats) => void;
  onFullscreenChange?: (isFullscreen: boolean) => void;
}

interface TextRect {
  x: number;
  y: number;
  w: number;
  h: number;
}

function paintTilesToCanvas(
  canvas: HTMLCanvasElement,
  inflated: Uint8Array,
  tileCount: number
): void {
  const ctx = canvas.getContext('2d', {alpha: false});
  if (!ctx) return;
  const iv = new DataView(inflated.buffer, inflated.byteOffset, inflated.byteLength);
  let off = 0;
  for (let i = 0; i < tileCount; i++) {
    if (off + 8 > inflated.byteLength) break;
    const x = iv.getUint16(off, true);
    const y = iv.getUint16(off + 2, true);
    const w = iv.getUint16(off + 4, true);
    const h = iv.getUint16(off + 6, true);
    off += 8;
    const bcount = w * h * 4;
    if (off + bcount > inflated.byteLength) break;
    const img = ctx.createImageData(w, h);
    img.data.set(new Uint8ClampedArray(inflated.buffer, inflated.byteOffset + off, bcount));
    ctx.putImageData(img, x, y);
    off += bcount;
  }
}

export const OSDisplayCanvas = forwardRef<CanvasHandle, OSDisplayCanvasProps>(function OSDisplayCanvas(
  {
    visible = true,
    settings = DEFAULT_SITE_SETTINGS,
    registerCommandSender,
    onStatsUpdate,
    onFullscreenChange,
  },
  ref
) {
  const containerRef = useRef<HTMLDivElement>(null);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const stagingCanvasRef = useRef<HTMLCanvasElement | null>(null);
  const mobileInputRef = useRef<HTMLInputElement>(null);
  const wsRef = useRef<WebSocket | null>(null);
  const lastMoveTimeRef = useRef(0);
  const settingsRef = useRef(settings);
  settingsRef.current = settings;

  const osSizeRef = useRef({w: 1280, h: 720});
  const targetSizeRef = useRef({w: 1280, h: 720});
  const textRectsRef = useRef<TextRect[]>([]);
  const activeTargetRef = useRef(0);
  const osDraggingRef = useRef(false);
  const prevMobileValRef = useRef('');
  const hasFirstFrameRef = useRef(false);

  const touchStateRef = useRef<{
    active: boolean;
    pointerId: number;
    isTouch: boolean;
    startX: number;
    startY: number;
    lastY: number;
    scrollAccum: number;
    osX: number;
    osY: number;
    scrolled: boolean;
    longPressed: boolean;
    longTimer: number | null;
  }>({
    active: false,
    pointerId: -1,
    isTouch: false,
    startX: 0,
    startY: 0,
    lastY: 0,
    scrollAccum: 0,
    osX: 0,
    osY: 0,
    scrolled: false,
    longPressed: false,
    longTimer: null,
  });

  const [status, setStatus] = useState<'connecting' | 'connected' | 'reconnecting'>('connecting');
  const [hasFirstFrame, setHasFirstFrame] = useState(false);
  const [nativeFullscreen, setNativeFullscreen] = useState(false);
  const [pseudoFullscreen, setPseudoFullscreen] = useState(false);
  const [osDims, setOsDims] = useState({w: 1280, h: 720});
  const [containerBox, setContainerBox] = useState({w: 1280, h: 720});
  const [showMobileKeyboard, setShowMobileKeyboard] = useState(false);
  const [mobileText, setMobileText] = useState('');

  const isOsFullscreen = nativeFullscreen || pseudoFullscreen;

  const sendCommand = useCallback((cmd: string) => {
    const ws = wsRef.current;
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(cmd);
    }
  }, []);

  const focusCanvas = useCallback(() => {
    canvasRef.current?.focus({preventScroll: true});
  }, []);

  const enterFullscreen = useCallback(() => {
    const el = containerRef.current;
    if (!el) return;

    if (nativeFullscreen || pseudoFullscreen) {
      setPseudoFullscreen(false);
      if (document.fullscreenElement) {
        document.exitFullscreen?.().catch(() => {});
      }
      requestAnimationFrame(focusCanvas);
      return;
    }

    if (typeof el.requestFullscreen === 'function') {
      el.requestFullscreen()
        .then(() => {
          setNativeFullscreen(true);
          requestAnimationFrame(focusCanvas);
        })
        .catch(() => {
          setPseudoFullscreen(true);
          requestAnimationFrame(focusCanvas);
        });
    } else {
      setPseudoFullscreen(true);
      requestAnimationFrame(focusCanvas);
    }
  }, [nativeFullscreen, pseudoFullscreen, focusCanvas]);

  useImperativeHandle(ref, () => ({
    takeScreenshot: () => {
      const canvas = canvasRef.current;
      if (!canvas) return;
      const dataUrl = canvas.toDataURL('image/png');
      const a = document.createElement('a');
      a.href = dataUrl;
      a.download = `nature-os-noble-${Date.now()}.png`;
      a.click();
    },
    requestKeyframe: () => {
      sendCommand('F');
    },
    enterFullscreen,
  }));

  useEffect(() => {
    const onFsChange = () => {
      const active = document.fullscreenElement === containerRef.current;
      setNativeFullscreen(active);
      if (!active) {
        setPseudoFullscreen(false);
      }
    };
    document.addEventListener('fullscreenchange', onFsChange);
    return () => document.removeEventListener('fullscreenchange', onFsChange);
  }, []);

  useEffect(() => {
    onFullscreenChange?.(isOsFullscreen);
  }, [isOsFullscreen, onFullscreenChange]);

  // Escape key exits pseudo-fullscreen cleanly on desktop
  useEffect(() => {
    if (!pseudoFullscreen) return;
    const onEsc = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        setPseudoFullscreen(false);
      }
    };
    window.addEventListener('keydown', onEsc);
    return () => window.removeEventListener('keydown', onEsc);
  }, [pseudoFullscreen]);

  // Initialize visible canvas dimensions imperatively ONCE (never controlled via JSX props so React never wipes the canvas!)
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    canvas.width = 1280;
    canvas.height = 720;
    const ctx = canvas.getContext('2d', {alpha: false});
    if (ctx) {
      ctx.fillStyle = '#060908';
      ctx.fillRect(0, 0, canvas.width, canvas.height);
    }
  }, []);

  useEffect(() => {
    registerCommandSender?.(sendCommand);
  }, [registerCommandSender, sendCommand]);

  useEffect(() => {
    if (!visible || showMobileKeyboard) return;
    const id = requestAnimationFrame(() => focusCanvas());
    return () => cancelAnimationFrame(id);
  }, [visible, hasFirstFrame, showMobileKeyboard, focusCanvas]);

  useEffect(() => {
    const onFocusReq = () => {
      if (visible && !showMobileKeyboard) {
        requestAnimationFrame(() => focusCanvas());
      }
    };
    window.addEventListener('nature-focus-canvas', onFocusReq);
    return () => window.removeEventListener('nature-focus-canvas', onFocusReq);
  }, [visible, showMobileKeyboard, focusCanvas]);

  // Smooth adaptive viewport observer + Retina HiDPI resolution sync
  useEffect(() => {
    if (!visible) return;
    const el = containerRef.current;
    if (!el) return;
    let resizeTimer: number | undefined;

    const measureAndSync = (immediate = false) => {
      const rect = el.getBoundingClientRect();
      if (rect.width < 100 || rect.height < 100) return;

      const fs = document.fullscreenElement === el || pseudoFullscreen;
      const pad = fs ? 0 : window.innerWidth < 640 ? 12 : 28;
      const aw = Math.max(320, Math.floor(rect.width - pad));
      const ah = Math.max(320, Math.floor(rect.height - pad));
      setContainerBox({w: aw, h: ah});

      let tw = 1280;
      let th = 720;
      if (settings.screenFitMode === 'adaptive') {
        const dpr = settings.highDpi ? Math.min(Math.max(window.devicePixelRatio || 1, 1), 2) : 1;
        let rw = aw * dpr;
        let rh = ah * dpr;
        const minShort = Math.min(rw, rh);
        if (settings.highDpi && minShort < 600) {
          const boost = 600 / minShort;
          rw *= boost;
          rh *= boost;
        }
        const cap = Math.min(1, 1440 / rw, 1440 / rh);
        tw = Math.max(360, Math.round(rw * cap));
        th = Math.max(360, Math.round(rh * cap));
      }

      if (tw !== targetSizeRef.current.w || th !== targetSizeRef.current.h) {
        targetSizeRef.current = {w: tw, h: th};
        if (resizeTimer) window.clearTimeout(resizeTimer);
        if (immediate) {
          sendCommand(`Z ${tw} ${th}`);
        } else {
          resizeTimer = window.setTimeout(() => {
            sendCommand(`Z ${tw} ${th}`);
          }, 40);
        }
      }
    };

    measureAndSync(true);
    sendCommand('F');
    const ro = new ResizeObserver(() => measureAndSync(false));
    ro.observe(el);
    const onResize = () => measureAndSync(false);
    window.addEventListener('resize', onResize);
    window.addEventListener('orientationchange', onResize);
    return () => {
      if (resizeTimer) window.clearTimeout(resizeTimer);
      ro.disconnect();
      window.removeEventListener('resize', onResize);
      window.removeEventListener('orientationchange', onResize);
    };
  }, [visible, isOsFullscreen, pseudoFullscreen, settings.screenFitMode, settings.highDpi, sendCommand]);

  // High-speed synchronous binary decoder with atomic offscreen staging swap on resize (zero black cover/flicker)
  useEffect(() => {
    let disposed = false;
    let reconnectTimer: number | undefined;

    const commitFrame = (fw: number, fh: number, inflated: Uint8Array | null, tileCount: number) => {
      const canvas = canvasRef.current;
      if (!canvas) return;

      const sizeChanged = fw > 0 && fh > 0 && (canvas.width !== fw || canvas.height !== fh);

      if (sizeChanged) {
        canvas.width = fw;
        canvas.height = fh;
        osSizeRef.current = {w: fw, h: fh};
        setOsDims({w: fw, h: fh});
        if (inflated && tileCount > 0) {
          if (!stagingCanvasRef.current) {
            stagingCanvasRef.current = document.createElement('canvas');
          }
          const staging = stagingCanvasRef.current;
          if (staging.width !== fw || staging.height !== fh) {
            staging.width = fw;
            staging.height = fh;
          }
          paintTilesToCanvas(staging, inflated, tileCount);
          const ctx = canvas.getContext('2d', {alpha: false});
          ctx?.drawImage(staging, 0, 0);
        }
      } else if (inflated && tileCount > 0) {
        paintTilesToCanvas(canvas, inflated, tileCount);
      }

      if (!hasFirstFrameRef.current && tileCount > 0) {
        hasFirstFrameRef.current = true;
        setHasFirstFrame(true);
      }
    };

    const decodeBinaryFrame = (buf: ArrayBuffer) => {
      const canvas = canvasRef.current;
      if (!canvas || buf.byteLength < 16) return;
      const view = new DataView(buf);
      const m0 = view.getUint8(0);
      const m1 = view.getUint8(1);
      const m2 = view.getUint8(2);
      const m3 = view.getUint8(3);
      if (m0 !== 0x4e || m1 !== 0x41 || m2 !== 0x54) return;

      // NAT3: v3 protocol with dynamic width/height, input rects, flags & /proc telemetry
      if (m3 === 0x33 && buf.byteLength >= 36) {
        const fw = view.getUint16(4, true);
        const fh = view.getUint16(6, true);
        const fps = view.getUint16(8, true) / 10;
        const ramMb = view.getUint16(10, true);
        const cpuPct = view.getUint16(12, true) / 10;
        const flags = view.getUint8(14);
        const inputCount = view.getUint8(15);
        const activeTarget = view.getUint16(16, true);
        const tileCount = view.getUint16(18, true);
        const zlen = view.getUint32(20, true);
        const totalRamMb = view.getUint16(24, true);
        const osRssMb = view.getUint16(26, true);
        const frameMs = view.getUint16(28, true) / 10;
        const cores: [number, number, number, number] = [
          view.getUint8(30),
          view.getUint8(31),
          view.getUint8(32),
          view.getUint8(33),
        ];

        osDraggingRef.current = (flags & 0x04) !== 0;
        activeTargetRef.current = activeTarget;

        const rectsOff = 36;
        const payloadOff = rectsOff + inputCount * 8;
        if (buf.byteLength < payloadOff + zlen) return;

        const nextRects: TextRect[] = [];
        for (let i = 0; i < inputCount; i++) {
          const rOff = rectsOff + i * 8;
          nextRects.push({
            x: view.getUint16(rOff, true),
            y: view.getUint16(rOff + 2, true),
            w: view.getUint16(rOff + 4, true),
            h: view.getUint16(rOff + 6, true),
          });
        }
        textRectsRef.current = nextRects;

        onStatsUpdate?.({fps, ramMb, cpuPct, tileCount, totalRamMb, osRssMb, frameMs, cores});

        let inflated: Uint8Array | null = null;
        if (tileCount > 0 && zlen > 0) {
          inflated = unzlibSync(new Uint8Array(buf, payloadOff, zlen));
        }
        commitFrame(fw, fh, inflated, tileCount);
        if (!hasFirstFrameRef.current && tileCount > 0) {
          hasFirstFrameRef.current = true;
          setHasFirstFrame(true);
        }
        return;
      }

      // NAT2 fallback
      if (m3 === 0x32 && buf.byteLength >= 20) {
        const fw = view.getUint16(4, true);
        const fh = view.getUint16(6, true);
        const fps = view.getUint16(8, true) / 10;
        const ramMb = view.getUint16(10, true);
        const cpuPct = view.getUint16(12, true) / 10;
        const tileCount = view.getUint16(14, true);
        const zlen = view.getUint32(16, true);

        onStatsUpdate?.({fps, ramMb, cpuPct, tileCount});

        let inflated: Uint8Array | null = null;
        if (buf.byteLength >= 20 + zlen && tileCount > 0) {
          inflated = unzlibSync(new Uint8Array(buf, 20, zlen));
        }
        commitFrame(fw, fh, inflated, tileCount);
      }
    };

    const connect = () => {
      if (disposed) return;
      const proto = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
      const ws = new WebSocket(`${proto}//${window.location.host}/api/os-stream`);
      ws.binaryType = 'arraybuffer';
      wsRef.current = ws;

      ws.onopen = () => {
        if (disposed) return;
        setStatus('connected');
        const {w, h} = targetSizeRef.current;
        sendCommand(`Z ${w} ${h}`);
        sendCommand('F');
      };

      ws.onmessage = (ev) => {
        if (ev.data instanceof ArrayBuffer) {
          try {
            decodeBinaryFrame(ev.data);
          } catch (err) {
            console.error('Frame decode error:', err);
          }
        }
      };

      ws.onclose = () => {
        if (disposed) return;
        setStatus('reconnecting');
        reconnectTimer = window.setTimeout(connect, 900);
      };
    };

    connect();
    return () => {
      disposed = true;
      if (reconnectTimer) clearTimeout(reconnectTimer);
      wsRef.current?.close();
    };
  }, [onStatsUpdate, sendCommand]);

  const toOsCoords = (clientX: number, clientY: number) => {
    const canvas = canvasRef.current;
    if (!canvas) return {x: 0, y: 0};
    const rect = canvas.getBoundingClientRect();
    if (rect.width === 0 || rect.height === 0) return {x: 0, y: 0};
    const w = canvas.width > 0 ? canvas.width : osSizeRef.current.w;
    const h = canvas.height > 0 ? canvas.height : osSizeRef.current.h;
    const x = Math.max(0, Math.min(w - 1, Math.round(((clientX - rect.left) / rect.width) * w)));
    const y = Math.max(0, Math.min(h - 1, Math.round(((clientY - rect.top) / rect.height) * h)));
    return {x, y};
  };

  const clearLongTimer = () => {
    const t = touchStateRef.current;
    if (t.longTimer !== null) {
      window.clearTimeout(t.longTimer);
      t.longTimer = null;
    }
  };

  const scrollThresholdPx =
    settings.scrollSpeed === 'fast' ? 9 : settings.scrollSpeed === 'compact' ? 22 : 14;

  const handlePointerDown = (e: React.PointerEvent<HTMLCanvasElement>) => {
    const isTouch = e.pointerType === 'touch';
    if (!isTouch) {
      e.currentTarget.focus({preventScroll: true});
    }
    e.currentTarget.setPointerCapture(e.pointerId);
    const {x, y} = toOsCoords(e.clientX, e.clientY);

    clearLongTimer();
    const t = touchStateRef.current;
    t.active = true;
    t.pointerId = e.pointerId;
    t.isTouch = isTouch;
    t.startX = e.clientX;
    t.startY = e.clientY;
    t.lastY = e.clientY;
    t.scrollAccum = 0;
    t.osX = x;
    t.osY = y;
    t.scrolled = false;
    t.longPressed = false;

    sendCommand(`D ${x} ${y}`);

    if (isTouch && settingsRef.current.longPressRightClick) {
      t.longTimer = window.setTimeout(() => {
        if (t.active && !t.scrolled && !osDraggingRef.current) {
          t.longPressed = true;
          sendCommand(`R ${t.osX} ${t.osY}`);
        }
      }, 500);
    }
  };

  const handlePointerMove = (e: React.PointerEvent<HTMLCanvasElement>) => {
    const t = touchStateRef.current;
    if (t.active && t.pointerId === e.pointerId && t.isTouch) {
      const dist = Math.hypot(e.clientX - t.startX, e.clientY - t.startY);
      if (dist > 8) {
        clearLongTimer();
        if (!osDraggingRef.current && !t.longPressed) {
          t.scrolled = true;
          const dyPx = e.clientY - t.lastY;
          t.lastY = e.clientY;
          t.scrollAccum += dyPx;
          while (Math.abs(t.scrollAccum) >= scrollThresholdPx) {
            if (t.scrollAccum < 0) {
              sendCommand('W 25');
              t.scrollAccum += scrollThresholdPx;
            } else {
              sendCommand('W -25');
              t.scrollAccum -= scrollThresholdPx;
            }
          }
          return;
        }
      }
    }

    const now = performance.now();
    if (now - lastMoveTimeRef.current >= 7) {
      lastMoveTimeRef.current = now;
      const {x, y} = toOsCoords(e.clientX, e.clientY);
      sendCommand(`M ${x} ${y}`);
    }
  };

  const hitsTextRect = (ox: number, oy: number) => {
    return textRectsRef.current.some(
      (r) => ox >= r.x && ox <= r.x + r.w && oy >= r.y && oy <= r.y + r.h
    );
  };

  const handlePointerUp = (e: React.PointerEvent<HTMLCanvasElement>) => {
    if (e.currentTarget.hasPointerCapture(e.pointerId)) {
      e.currentTarget.releasePointerCapture(e.pointerId);
    }
    clearLongTimer();
    const t = touchStateRef.current;
    const {x, y} = toOsCoords(e.clientX, e.clientY);
    const wasTapTouch = t.active && t.isTouch && !t.scrolled && !t.longPressed;
    const tapX = t.osX;
    const tapY = t.osY;
    t.active = false;

    sendCommand(`U ${x} ${y}`);

    if (!t.isTouch) {
      focusCanvas();
    } else if (wasTapTouch && settingsRef.current.keyboardMode === 'auto-touch') {
      if (hitsTextRect(x, y) || hitsTextRect(tapX, tapY) || activeTargetRef.current > 0) {
        setShowMobileKeyboard(true);
        mobileInputRef.current?.focus({preventScroll: true});
      }
    }
  };

  const handlePointerCancel = (e: React.PointerEvent<HTMLCanvasElement>) => {
    if (e.currentTarget.hasPointerCapture(e.pointerId)) {
      e.currentTarget.releasePointerCapture(e.pointerId);
    }
    clearLongTimer();
    touchStateRef.current.active = false;
    sendCommand('U 0 0');
  };

  const handleWheel = (e: React.WheelEvent<HTMLCanvasElement>) => {
    e.preventDefault();
    const mag = settings.scrollSpeed === 'fast' ? 40 : settings.scrollSpeed === 'compact' ? 18 : 26;
    const dy = Math.sign(e.deltaY) * mag;
    if (dy !== 0) sendCommand(`W ${dy}`);
  };

  const handleContextMenu = (e: React.MouseEvent<HTMLCanvasElement>) => {
    e.preventDefault();
    const {x, y} = toOsCoords(e.clientX, e.clientY);
    sendCommand(`R ${x} ${y}`);
  };

  const handleClick = (e: React.MouseEvent<HTMLCanvasElement>) => {
    e.currentTarget.focus({preventScroll: true});
    const {x, y} = toOsCoords(e.clientX, e.clientY);
    sendCommand(`D ${x} ${y}`);
    window.setTimeout(() => {
      sendCommand(`U ${x} ${y}`);
    }, 45);
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLCanvasElement>) => {
    if (e.key === 'Meta' || e.key === 'Control' || e.key === 'Alt' || e.key === 'Shift') return;
    if ((e.ctrlKey || e.metaKey) && (e.key === 'c' || e.key === 'C' || e.key === 'l' || e.key === 'L' || e.key === 'u' || e.key === 'U')) {
      e.preventDefault();
      sendCommand(`K Ctrl+${e.key.toUpperCase()}`);
      return;
    }
    if ((e.metaKey || e.altKey) && (e.key === 'ArrowLeft' || e.key === 'ArrowRight' || e.key === 'ArrowUp' || e.key === 'ArrowDown')) {
      e.preventDefault();
      const cmd = e.key === 'ArrowLeft' ? 'K SnapLeft' : e.key === 'ArrowRight' ? 'K SnapRight' : e.key === 'ArrowUp' ? 'K SnapUp' : 'K SnapDown';
      sendCommand(cmd);
      return;
    }
    if ((e.altKey || e.metaKey) && e.key === 'Tab') {
      e.preventDefault();
      sendCommand('K AltTab');
      return;
    }
    if (
      e.key === 'Backspace' ||
      e.key === 'Delete' ||
      e.key === 'Enter' ||
      e.key === 'ArrowUp' ||
      e.key === 'ArrowDown' ||
      e.key === 'ArrowLeft' ||
      e.key === 'ArrowRight' ||
      e.key === 'Home' ||
      e.key === 'End' ||
      e.key === 'PageUp' ||
      e.key === 'PageDown' ||
      e.key === 'F2' ||
      e.key === 'F11' ||
      e.key === 'Tab' ||
      e.key === 'Escape'
    ) {
      e.preventDefault();
      sendCommand(`K ${e.key}`);
      return;
    }
    if (e.key === ' ' || e.key === 'Spacebar' || e.code === 'Space') {
      e.preventDefault();
      sendCommand('K Space');
      return;
    }
    if (e.key.length === 1) {
      e.preventDefault();
      sendCommand(`K ${e.key}`);
    }
  };

  const handleMobileInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const next = e.target.value;
    const prev = prevMobileValRef.current;

    if (next.startsWith(prev)) {
      const added = next.slice(prev.length);
      for (const ch of added) {
        sendCommand(ch === ' ' ? 'K Space' : `K ${ch}`);
      }
    } else if (prev.startsWith(next)) {
      const delCount = prev.length - next.length;
      for (let i = 0; i < delCount; i++) {
        sendCommand('K Backspace');
      }
    } else {
      let common = 0;
      while (common < prev.length && common < next.length && prev[common] === next[common]) {
        common++;
      }
      for (let i = common; i < prev.length; i++) {
        sendCommand('K Backspace');
      }
      for (let i = common; i < next.length; i++) {
        sendCommand(next[i] === ' ' ? 'K Space' : `K ${next[i]}`);
      }
    }

    prevMobileValRef.current = next;
    setMobileText(next);
  };

  const handleMobileInputKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      e.preventDefault();
      sendCommand('K Enter');
      prevMobileValRef.current = '';
      setMobileText('');
    } else if (e.key === 'Backspace' && prevMobileValRef.current.length === 0) {
      e.preventDefault();
      sendCommand('K Backspace');
    } else if (e.key === 'Escape') {
      e.preventDefault();
      sendCommand('K Escape');
      setShowMobileKeyboard(false);
      mobileInputRef.current?.blur();
    } else if (
      e.key === 'ArrowUp' ||
      e.key === 'ArrowDown' ||
      e.key === 'ArrowLeft' ||
      e.key === 'ArrowRight' ||
      e.key === 'Tab'
    ) {
      e.preventDefault();
      sendCommand(`K ${e.key}`);
    }
  };

  const handleMobileSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    sendCommand('K Enter');
    prevMobileValRef.current = '';
    setMobileText('');
  };

  const closeKeyboardBar = () => {
    setShowMobileKeyboard(false);
    prevMobileValRef.current = '';
    setMobileText('');
    mobileInputRef.current?.blur();
    requestAnimationFrame(focusCanvas);
  };

  // Compute exact fitted CSS box that preserves OS aspect ratio with smooth transitions on sudden resizes
  const aspect = osDims.w / osDims.h;
  let dispW = containerBox.w;
  let dispH = Math.round(dispW / aspect);
  if (dispH > containerBox.h) {
    dispH = containerBox.h;
    dispW = Math.round(dispH * aspect);
  }

  return (
    <div
      ref={containerRef}
      className={`flex items-center justify-center select-none overflow-hidden ${
        isOsFullscreen
          ? 'fixed inset-0 z-50 h-dvh w-screen bg-[#060908] p-0 m-0'
          : 'relative h-full w-full p-1.5 sm:p-3.5 bg-[#060908]'
      }`}
    >
      <div
        style={{width: `${dispW}px`, height: `${dispH}px`}}
        className={`relative flex items-center justify-center overflow-hidden ${
          isOsFullscreen
            ? 'rounded-none border-0 shadow-none'
            : 'rounded-2xl border border-white/10 bg-[#060908] shadow-[0_32px_90px_-20px_rgba(0,0,0,0.9)]'
        }`}
      >
        <canvas
          ref={canvasRef}
          tabIndex={0}
          onPointerDown={handlePointerDown}
          onPointerMove={handlePointerMove}
          onPointerUp={handlePointerUp}
          onPointerCancel={handlePointerCancel}
          onWheel={handleWheel}
          onContextMenu={handleContextMenu}
          onClick={handleClick}
          onKeyDown={handleKeyDown}
          style={{touchAction: 'none', imageRendering: 'crisp-edges'}}
          className="h-full w-full cursor-default select-none outline-none"
        />

        {/* Floating Fullscreen Toggle (always reachable on touch when fullscreen, hover-reveal on desktop) */}
        <div
          className={`absolute top-12 sm:top-3 right-3 z-20 flex items-center gap-2 transition-opacity ${
            isOsFullscreen
              ? 'opacity-90 sm:opacity-35 sm:hover:opacity-100'
              : 'opacity-0 hover:opacity-100 focus-within:opacity-100'
          }`}
        >
          <button
            type="button"
            onClick={(e) => {
              e.currentTarget.blur();
              enterFullscreen();
            }}
            className="flex items-center gap-1.5 rounded-lg bg-black/70 backdrop-blur-md border border-white/20 px-2.5 py-1.5 text-xs font-medium text-white hover:bg-black/90 transition-colors shadow-lg min-h-[34px] whitespace-nowrap shrink-0"
            title={isOsFullscreen ? 'Exit Fullscreen' : 'Make OS Fullscreen'}
          >
            {isOsFullscreen ? <Minimize2 className="h-3.5 w-3.5" /> : <Maximize2 className="h-3.5 w-3.5" />}
            {isOsFullscreen && <span className="sm:hidden">Exit</span>}
          </button>
        </div>

        {/* Manual Type Button (when keyboardMode === 'button', or optionally on desktop) */}
        {(settings.keyboardMode === 'button' || settings.showDesktopKeyboardBtn) && (
          <div
            className={`absolute bottom-3 right-3 z-30 ${
              settings.showDesktopKeyboardBtn ? 'flex' : 'flex sm:hidden'
            }`}
          >
            <button
              type="button"
              onClick={() => {
                if (showMobileKeyboard) {
                  closeKeyboardBar();
                } else {
                  setShowMobileKeyboard(true);
                  mobileInputRef.current?.focus({preventScroll: true});
                }
              }}
              className={`flex items-center gap-1.5 rounded-full backdrop-blur-md px-3.5 py-2 text-xs font-semibold text-white shadow-xl border transition-colors min-h-[40px] ${
                showMobileKeyboard
                  ? 'bg-neutral-900/90 border-white/25'
                  : 'bg-emerald-600/90 border-emerald-400/30 hover:bg-emerald-500'
              }`}
            >
              <Keyboard className="h-3.5 w-3.5" />
              <span>{showMobileKeyboard ? 'Hide Keys' : 'Type'}</span>
            </button>
          </div>
        )}

        {/* Always-mounted Touch Keyboard Bar */}
        <form
          onSubmit={handleMobileSubmit}
          className={
            showMobileKeyboard
              ? 'absolute bottom-14 inset-x-3 sm:inset-x-auto sm:right-3 sm:w-[420px] z-40 flex flex-col gap-1.5 rounded-xl border border-white/20 bg-[#0B120F]/95 p-2 backdrop-blur-md shadow-2xl'
              : 'fixed bottom-2 left-2 h-1 w-1 opacity-0 pointer-events-none overflow-hidden'
          }
        >
          <div className="flex items-center gap-1.5">
            <input
              ref={mobileInputRef}
              type="text"
              autoCapitalize="none"
              autoCorrect="off"
              spellCheck={false}
              value={mobileText}
              onChange={handleMobileInputChange}
              onKeyDown={handleMobileInputKeyDown}
              placeholder="Live OS typing (Esc, ↑/↓, Enter)..."
              className="flex-1 rounded-lg border border-white/10 bg-black/60 px-3 py-2 text-xs text-white placeholder:text-neutral-500 focus:border-emerald-500/50 focus:outline-none min-h-[38px]"
            />
            <button
              type="submit"
              className="flex items-center gap-1 rounded-lg bg-emerald-600 px-3 py-2 text-xs font-semibold text-white hover:bg-emerald-500 min-h-[38px]"
              title="Send Enter"
            >
              <CornerDownLeft className="h-3.5 w-3.5" />
              <span>Enter</span>
            </button>
            <button
              type="button"
              onClick={closeKeyboardBar}
              className="flex items-center justify-center rounded-lg border border-white/10 bg-white/5 px-2.5 py-2 text-neutral-300 hover:bg-white/10 min-h-[38px]"
              title="Close Keyboard"
            >
              <X className="h-3.5 w-3.5" />
            </button>
          </div>
          {showMobileKeyboard && (
            <div className="flex items-center gap-1 overflow-x-auto text-[11px] font-mono">
              {(['Escape', 'Tab', 'Space', 'ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown'] as const).map((k) => (
                <button
                  key={k}
                  type="button"
                  onClick={() => {
                    sendCommand(`K ${k}`);
                    mobileInputRef.current?.focus({preventScroll: true});
                  }}
                  className="rounded-md border border-white/10 bg-white/5 px-2.5 py-1 text-neutral-200 hover:bg-white/10 min-h-[28px]"
                >
                  {k === 'Escape'
                    ? 'Esc'
                    : k === 'Space'
                      ? 'Space'
                      : k === 'ArrowLeft'
                        ? '←'
                        : k === 'ArrowRight'
                          ? '→'
                          : k === 'ArrowUp'
                            ? '↑'
                            : k === 'ArrowDown'
                              ? '↓'
                              : 'Tab'}
                </button>
              ))}
              <button
                type="button"
                onClick={() => {
                  sendCommand('K Backspace');
                  mobileInputRef.current?.focus({preventScroll: true});
                }}
                className="flex items-center gap-1 rounded-md border border-white/10 bg-white/5 px-2.5 py-1 text-neutral-200 hover:bg-white/10 min-h-[28px]"
              >
                <Delete className="h-3 w-3" />
                <span>Bksp</span>
              </button>
            </div>
          )}
        </form>

        {/* Initial boot card ONLY before first frame; non-blocking badge on brief reconnect */}
        {!hasFirstFrame && status !== 'connected' && (
          <div className="absolute inset-0 flex items-center justify-center bg-[#090E0C]/90 backdrop-blur-md z-30">
            <div className="text-center px-6 py-4 rounded-2xl border border-white/10 bg-[#0D1412]/95 shadow-2xl">
              <div className="flex items-center justify-center gap-2 mb-2">
                <span className="h-2.5 w-2.5 rounded-full bg-emerald-500"></span>
                <span className="text-sm font-semibold text-neutral-100">
                  Booting Nature OS C/C++ Compositor...
                </span>
              </div>
              <div className="font-mono text-xs text-neutral-400">
                nature-compositor · {osDims.w}×{osDims.h} · Cairo/NAT3 Stream
              </div>
            </div>
          </div>
        )}

        {hasFirstFrame && status === 'reconnecting' && (
          <div className="pointer-events-none absolute top-3 left-1/2 -translate-x-1/2 z-30 rounded-full border border-white/15 bg-black/75 px-3 py-1 font-mono text-[11px] text-amber-300 backdrop-blur-md">
            Reconnecting display stream...
          </div>
        )}
      </div>
    </div>
  );
});
