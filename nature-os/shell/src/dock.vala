/*
 * dock.vala: Floating low-memory launcher dock for Nature OS
 * Reads Ubuntu .desktop entries from /usr/share/applications and ~/.local/share/applications
 */

using Gtk;
using GLib;

namespace NatureShell {

    public class BotanicalDock : Gtk.Window {
        private Gtk.Box icon_row;
        private GLib.Settings desktop_settings;

        public BotanicalDock (Gtk.Application app) {
            Object (
                application: app,
                title: "nature-dock",
                decorated: false
            );

            desktop_settings = new GLib.Settings ("org.natureos.desktop");
            this.add_css_class ("nature-dock-window");

            icon_row = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 6);
            icon_row.add_css_class ("nature-dock-container");
            icon_row.set_margin_start (10);
            icon_row.set_margin_end (10);
            icon_row.set_margin_top (6);
            icon_row.set_margin_bottom (6);

            populate_pinned_apps ();
            this.set_child (icon_row);
        }

        private void populate_pinned_apps () {
            string[] pinned = desktop_settings.get_strv ("pinned-apps");
            foreach (string desktop_id in pinned) {
                var app_info = new GLib.DesktopAppInfo (desktop_id);
                if (app_info != null) {
                    var btn = new Gtk.Button ();
                    btn.set_tooltip_text (app_info.get_display_name ());
                    btn.add_css_class ("nature-dock-item");
                    var icon = new Gtk.Image.from_gicon (app_info.get_icon ());
                    icon.set_pixel_size (desktop_settings.get_int ("dock-icon-size"));
                    btn.set_child (icon);
                    btn.clicked.connect (() => {
                        try {
                            app_info.launch (null, null);
                        } catch (Error e) {
                            warning ("Launch failed for %s: %s", desktop_id, e.message);
                        }
                    });
                    icon_row.append (btn);
                }
            }
        }
    }
}
