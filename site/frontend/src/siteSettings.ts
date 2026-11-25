export type KeyboardMode = 'button' | 'auto-touch';
export type ScreenFitMode = 'adaptive' | 'fixed-16-9';
export type ScrollSpeed = 'compact' | 'normal' | 'fast';

export interface SiteSettings {
  keyboardMode: KeyboardMode;
  screenFitMode: ScreenFitMode;
  highDpi: boolean;
  showDesktopKeyboardBtn: boolean;
  longPressRightClick: boolean;
  scrollSpeed: ScrollSpeed;
}

export const DEFAULT_SITE_SETTINGS: SiteSettings = {
  keyboardMode: 'button',
  screenFitMode: 'adaptive',
  highDpi: true,
  showDesktopKeyboardBtn: false,
  longPressRightClick: true,
  scrollSpeed: 'normal',
};

const STORAGE_KEY = 'nature_os_site_settings_v1';

export function loadSiteSettings(): SiteSettings {
  try {
    const raw = window.localStorage.getItem(STORAGE_KEY);
    if (!raw) return DEFAULT_SITE_SETTINGS;
    const parsed = JSON.parse(raw) as Partial<SiteSettings>;
    return {
      keyboardMode: parsed.keyboardMode === 'auto-touch' ? 'auto-touch' : 'button',
      screenFitMode: parsed.screenFitMode === 'fixed-16-9' ? 'fixed-16-9' : 'adaptive',
      highDpi: parsed.highDpi !== undefined ? Boolean(parsed.highDpi) : true,
      showDesktopKeyboardBtn: Boolean(parsed.showDesktopKeyboardBtn),
      longPressRightClick: parsed.longPressRightClick !== undefined ? Boolean(parsed.longPressRightClick) : true,
      scrollSpeed:
        parsed.scrollSpeed === 'compact' || parsed.scrollSpeed === 'fast'
          ? parsed.scrollSpeed
          : 'normal',
    };
  } catch {
    return DEFAULT_SITE_SETTINGS;
  }
}

export function saveSiteSettings(settings: SiteSettings): void {
  try {
    window.localStorage.setItem(STORAGE_KEY, JSON.stringify(settings));
  } catch {
    // ignore storage quota / private mode issues
  }
}
