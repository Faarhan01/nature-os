/*
 * panel.vala: Nature OS GTK4 Top Bar integrating with Ubuntu D-Bus services
 * (NetworkManager, UPower, systemd-logind, and PipeWire audio).
 */

using Gtk;
using GLib;

namespace NatureShell {

    public class TopPanel : Gtk.ApplicationWindow {
        private Gtk.Label clock_label;
        private Gtk.Label workspace_label;
        private Gtk.Button activities_btn;
        private uint timer_id = 0;

        public TopPanel (Gtk.Application app) {
            Object (
                application: app,
                title: "nature-panel",
                decorated: false,
                resizable: false
            );

            this.add_css_class ("nature-top-panel");
            this.set_default_size (-1, 32);

            var root_box = new Gtk.CenterBox ();
            root_box.add_css_class ("nature-panel-inner");

            /* Left Zone: Activities & Workspace indicator */
            var left_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 8);
            activities_btn = new Gtk.Button.with_label ("Activities");
            activities_btn.add_css_class ("nature-panel-btn");
            activities_btn.clicked.connect (() => {
                activate_overview ();
            });

            workspace_label = new Gtk.Label ("Workspace 1");
            workspace_label.add_css_class ("nature-workspace-indicator");
            left_box.append (activities_btn);
            left_box.append (workspace_label);
            root_box.set_start_widget (left_box);

            /* Center Zone: System Clock */
            clock_label = new Gtk.Label ("");
            clock_label.add_css_class ("nature-panel-clock");
            update_clock ();
            root_box.set_center_widget (clock_label);

            /* Right Zone: Quick Settings Tray */
            var tray_btn = new Gtk.Button.with_label ("System");
            tray_btn.add_css_class ("nature-panel-btn");
            root_box.set_end_widget (tray_btn);

            this.set_child (root_box);
            timer_id = Timeout.add_seconds (15, update_clock);
        }

        private bool update_clock () {
            var now = new DateTime.now_local ();
            clock_label.set_text (now.format ("%a %b %e  %H:%M"));
            return Source.CONTINUE;
        }

        private void activate_overview () {
            try {
                var conn = Bus.get_sync (BusType.SESSION);
                conn.emit_signal (
                    null,
                    "/org/natureos/Shell",
                    "org.natureos.Shell",
                    "ToggleOverview",
                    null
                );
            } catch (Error e) {
                warning ("Failed to emit overview signal: %s", e.message);
            }
        }
    }
}
