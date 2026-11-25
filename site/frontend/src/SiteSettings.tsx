import React from 'react';
import {
  Check,
  Hand,
  Keyboard,
  Maximize,
  Monitor,
  MousePointerClick,
  RotateCcw,
  Sparkles,
} from 'lucide-react';
import {DEFAULT_SITE_SETTINGS, ScrollSpeed, SiteSettings as SiteSettingsState} from './siteSettings';

interface SiteSettingsProps {
  settings: SiteSettingsState;
  onChange: (next: SiteSettingsState) => void;
  onReturnToDisplay: () => void;
}

export function SiteSettings({settings, onChange, onReturnToDisplay}: SiteSettingsProps) {
  const update = <K extends keyof SiteSettingsState>(key: K, value: SiteSettingsState[K]) => {
    onChange({...settings, [key]: value});
  };

  return (
    <div className="flex h-full w-full flex-col overflow-y-auto bg-[#070B09] text-[#E8EFEA] p-4 sm:p-6 lg:p-10 nature-scroll">
      <div className="mx-auto w-full max-w-4xl space-y-6 pb-8">
        {/* Header */}
        <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-b border-white/10 pb-5">
          <div>
            <span className="font-mono text-[11px] uppercase tracking-wider text-emerald-400">
              Preview Companion Settings
            </span>
            <h1 className="mt-1 font-display text-2xl font-bold tracking-tight text-white">
              Input, Touch &amp; Display Preferences
            </h1>
            <p className="mt-1 text-sm text-neutral-400">
              Configure how the web previewer handles touch keyboards, aspect ratio adaptation, and pointer scrolling.
            </p>
          </div>

          <div className="flex items-center gap-2.5 shrink-0">
            <button
              type="button"
              onClick={() => onChange(DEFAULT_SITE_SETTINGS)}
              className="flex items-center gap-1.5 rounded-lg border border-white/10 bg-white/[0.03] px-3 py-2 text-xs font-medium text-neutral-300 hover:bg-white/[0.07] transition-colors min-h-[40px]"
            >
              <RotateCcw className="h-3.5 w-3.5" />
              <span>Reset</span>
            </button>
            <button
              type="button"
              onClick={onReturnToDisplay}
              className="flex items-center gap-1.5 rounded-lg bg-[#2E8B57] px-4 py-2 text-xs font-semibold text-white hover:bg-[#257347] transition-colors min-h-[40px]"
            >
              <Monitor className="h-3.5 w-3.5" />
              <span>Back to OS Display</span>
            </button>
          </div>
        </div>

        {/* Section 1: Mobile / Touch Keyboard Trigger Mode */}
        <section className="rounded-xl border border-white/10 bg-[#0B100E] p-5 space-y-4">
          <div className="flex items-start gap-3">
            <div className="flex h-9 w-9 shrink-0 items-center justify-center rounded-lg border border-emerald-500/25 bg-emerald-500/10 text-emerald-400">
              <Keyboard className="h-4.5 w-4.5" />
            </div>
            <div>
              <h2 className="font-display text-base font-semibold text-white">
                Touchscreen Virtual Keyboard Mode
              </h2>
              <p className="text-xs text-neutral-400 mt-0.5">
                Choose between manually toggling the keyboard via the Type button or automatically popping open the keyboard whenever your finger taps a text input inside Nature OS.
              </p>
            </div>
          </div>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
            <button
              type="button"
              onClick={() => update('keyboardMode', 'button')}
              className={`flex flex-col items-start text-left rounded-xl border p-4 transition-colors min-h-[108px] ${
                settings.keyboardMode === 'button'
                  ? 'border-emerald-500/60 bg-emerald-500/10'
                  : 'border-white/10 bg-black/25 hover:bg-white/[0.03]'
              }`}
            >
              <div className="flex w-full items-center justify-between gap-2">
                <div className="flex items-center gap-2">
                  <MousePointerClick className="h-4 w-4 text-emerald-400" />
                  <span className="font-display text-sm font-semibold text-white">
                    Type Button
                  </span>
                </div>
                {settings.keyboardMode === 'button' && (
                  <span className="flex h-5 w-5 items-center justify-center rounded-full bg-emerald-500 text-black">
                    <Check className="h-3 w-3 stroke-[3]" />
                  </span>
                )}
              </div>
              <p className="mt-2 text-xs leading-relaxed text-neutral-300">
                Shows the floating <span className="font-mono text-emerald-300">Type</span> toggle button on mobile screens so the virtual keyboard only opens when you tap it.
              </p>
            </button>

            <button
              type="button"
              onClick={() => update('keyboardMode', 'auto-touch')}
              className={`flex flex-col items-start text-left rounded-xl border p-4 transition-colors min-h-[108px] ${
                settings.keyboardMode === 'auto-touch'
                  ? 'border-emerald-500/60 bg-emerald-500/10'
                  : 'border-white/10 bg-black/25 hover:bg-white/[0.03]'
              }`}
            >
              <div className="flex w-full items-center justify-between gap-2">
                <div className="flex items-center gap-2">
                  <Sparkles className="h-4 w-4 text-emerald-400" />
                  <span className="font-display text-sm font-semibold text-white">
                    Auto-Open on Touch Input
                  </span>
                </div>
                {settings.keyboardMode === 'auto-touch' && (
                  <span className="flex h-5 w-5 items-center justify-center rounded-full bg-emerald-500 text-black">
                    <Check className="h-3 w-3 stroke-[3]" />
                  </span>
                )}
              </div>
              <p className="mt-2 text-xs leading-relaxed text-neutral-300">
                Automatically opens your device keyboard when tapping Terminal, Spotlight Search, Files Search, or the Sylva Browser URL bar with your finger.
              </p>
            </button>
          </div>
        </section>

        {/* Section 2: Screen Fit & Aspect Ratio */}
        <section className="rounded-xl border border-white/10 bg-[#0B100E] p-5 space-y-4">
          <div className="flex items-start gap-3">
            <div className="flex h-9 w-9 shrink-0 items-center justify-center rounded-lg border border-emerald-500/25 bg-emerald-500/10 text-emerald-400">
              <Maximize className="h-4.5 w-4.5" />
            </div>
            <div>
              <h2 className="font-display text-base font-semibold text-white">
                Screen Size &amp; Fullscreen Scaling
              </h2>
              <p className="text-xs text-neutral-400 mt-0.5">
                Controls how Nature OS adapts to square monitors, portrait phones, and ultrawide viewports without visual distortion.
              </p>
            </div>
          </div>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
            <button
              type="button"
              onClick={() => update('screenFitMode', 'adaptive')}
              className={`flex flex-col items-start text-left rounded-xl border p-4 transition-colors min-h-[96px] ${
                settings.screenFitMode === 'adaptive'
                  ? 'border-emerald-500/60 bg-emerald-500/10'
                  : 'border-white/10 bg-black/25 hover:bg-white/[0.03]'
              }`}
            >
              <div className="flex w-full items-center justify-between gap-2">
                <span className="font-display text-sm font-semibold text-white">
                  Adaptive Resolution (Recommended)
                </span>
                {settings.screenFitMode === 'adaptive' && (
                  <span className="flex h-5 w-5 items-center justify-center rounded-full bg-emerald-500 text-black">
                    <Check className="h-3 w-3 stroke-[3]" />
                  </span>
                )}
              </div>
              <p className="mt-1.5 text-xs leading-relaxed text-neutral-300">
                Resizes the native OS framebuffer dynamically to match any square, mobile, or widescreen viewport 1:1 with zero stretching.
              </p>
            </button>

            <button
              type="button"
              onClick={() => update('screenFitMode', 'fixed-16-9')}
              className={`flex flex-col items-start text-left rounded-xl border p-4 transition-colors min-h-[96px] ${
                settings.screenFitMode === 'fixed-16-9'
                  ? 'border-emerald-500/60 bg-emerald-500/10'
                  : 'border-white/10 bg-black/25 hover:bg-white/[0.03]'
              }`}
            >
              <div className="flex w-full items-center justify-between gap-2">
                <span className="font-display text-sm font-semibold text-white">
                  Fixed 16:9 (1280×720)
                </span>
                {settings.screenFitMode === 'fixed-16-9' && (
                  <span className="flex h-5 w-5 items-center justify-center rounded-full bg-emerald-500 text-black">
                    <Check className="h-3 w-3 stroke-[3]" />
                  </span>
                )}
              </div>
              <p className="mt-1.5 text-xs leading-relaxed text-neutral-300">
                Locks the OS desktop at 1280×720 and preserves exact 16:9 proportions with letterboxing on square or portrait screens.
              </p>
            </button>
          </div>
        </section>

        {/* Section 3: Touch Gestures & Scroll Speed */}
        <section className="rounded-xl border border-white/10 bg-[#0B100E] p-5 space-y-4">
          <div className="flex items-start gap-3">
            <div className="flex h-9 w-9 shrink-0 items-center justify-center rounded-lg border border-emerald-500/25 bg-emerald-500/10 text-emerald-400">
              <Hand className="h-4.5 w-4.5" />
            </div>
            <div>
              <h2 className="font-display text-base font-semibold text-white">
                Touch Gestures &amp; Scroll Sensitivity
              </h2>
              <p className="text-xs text-neutral-400 mt-0.5">
                Fine-tune finger drag scrolling, touch context menus, and desktop keyboard assist visibility.
              </p>
            </div>
          </div>

          <div className="divide-y divide-white/10 rounded-xl border border-white/10 bg-black/25">
            <div className="flex items-center justify-between gap-4 p-4 min-h-[56px]">
              <div>
                <div className="text-sm font-medium text-white">Retina HiDPI Clarity</div>
                <div className="text-xs text-neutral-400">Supersamples Cairo rendering for crisp text and icons on phone and Retina displays</div>
              </div>
              <button
                type="button"
                role="switch"
                aria-checked={settings.highDpi}
                onClick={() => update('highDpi', !settings.highDpi)}
                className={`relative inline-flex h-6 w-11 shrink-0 items-center rounded-full transition-colors ${
                  settings.highDpi ? 'bg-emerald-600' : 'bg-white/15'
                }`}
              >
                <span
                  className={`inline-block h-4.5 w-4.5 transform rounded-full bg-white transition-transform ${
                    settings.highDpi ? 'translate-x-5.5' : 'translate-x-1'
                  }`}
                />
              </button>
            </div>

            <div className="flex items-center justify-between gap-4 p-4 min-h-[56px]">
              <div>
                <div className="text-sm font-medium text-white">Long-Press Right-Click on Touch</div>
                <div className="text-xs text-neutral-400">Hold a finger steady for 500 ms to open desktop context menu</div>
              </div>
              <button
                type="button"
                role="switch"
                aria-checked={settings.longPressRightClick}
                onClick={() => update('longPressRightClick', !settings.longPressRightClick)}
                className={`relative inline-flex h-6 w-11 shrink-0 items-center rounded-full transition-colors ${
                  settings.longPressRightClick ? 'bg-emerald-600' : 'bg-white/15'
                }`}
              >
                <span
                  className={`inline-block h-4.5 w-4.5 transform rounded-full bg-white transition-transform ${
                    settings.longPressRightClick ? 'translate-x-5.5' : 'translate-x-1'
                  }`}
                />
              </button>
            </div>

            <div className="flex items-center justify-between gap-4 p-4 min-h-[56px]">
              <div>
                <div className="text-sm font-medium text-white">Show Type Assist on Desktop Screens</div>
                <div className="text-xs text-neutral-400">Also display the on-screen Type bar button on large desktop viewports</div>
              </div>
              <button
                type="button"
                role="switch"
                aria-checked={settings.showDesktopKeyboardBtn}
                onClick={() => update('showDesktopKeyboardBtn', !settings.showDesktopKeyboardBtn)}
                className={`relative inline-flex h-6 w-11 shrink-0 items-center rounded-full transition-colors ${
                  settings.showDesktopKeyboardBtn ? 'bg-emerald-600' : 'bg-white/15'
                }`}
              >
                <span
                  className={`inline-block h-4.5 w-4.5 transform rounded-full bg-white transition-transform ${
                    settings.showDesktopKeyboardBtn ? 'translate-x-5.5' : 'translate-x-1'
                  }`}
                />
              </button>
            </div>

            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 p-4 min-h-[56px]">
              <div>
                <div className="text-sm font-medium text-white">Scroll Sensitivity</div>
                <div className="text-xs text-neutral-400">Controls step speed for wheel and touch-drag scrolling</div>
              </div>
              <div className="flex items-center gap-1.5 rounded-lg border border-white/10 bg-black/40 p-1">
                {(['compact', 'normal', 'fast'] as ScrollSpeed[]).map((spd) => (
                  <button
                    key={spd}
                    type="button"
                    onClick={() => update('scrollSpeed', spd)}
                    className={`rounded-md px-3 py-1.5 text-xs font-medium capitalize transition-colors min-h-[32px] ${
                      settings.scrollSpeed === spd
                        ? 'bg-emerald-600 text-white'
                        : 'text-neutral-400 hover:text-white'
                    }`}
                  >
                    {spd === 'compact' ? 'Precision' : spd === 'normal' ? 'Balanced' : 'Fast'}
                  </button>
                ))}
              </div>
            </div>
          </div>
        </section>
      </div>
    </div>
  );
}
