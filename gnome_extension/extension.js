import Gio from "gi://Gio";
import GLib from "gi://GLib";
import Shell from "gi://Shell";
import Meta from "gi://Meta";

import { Extension } from "resource:///org/gnome/shell/extensions/extension.js";

/* ---------------- D-BUS INTERFACE ---------------- */

const DBusInterface = `
<node>
  <interface name="org.gnome.WellbeingTracker">
    <method name="GetCurrentWindow">
      <arg type="s" direction="out" name="title"/>
      <arg type="s" direction="out" name="app_id"/>
      <arg type="x" direction="out" name="timestamp"/>
    </method>

    <signal name="WindowChanged">
      <arg type="s" name="title"/>
      <arg type="s" name="app_id"/>
      <arg type="x" name="timestamp"/>
    </signal>

    <signal name="IdleStatusChanged">
      <arg type="b" name="idle"/>
    </signal>
  </interface>
</node>`;

/* ---------------- HELPERS ---------------- */

function getNiceName(appId) {
  if (!appId) return "";

  const appSystem = Shell.AppSystem.get_default();
  const app = appSystem.lookup_app(appId);

  return app ? app.get_name() : appId;
}

/* ---------------- EXTENSION ---------------- */

export default class WellbeingTrackerExtension extends Extension {
  enable() {
    this._display = global.display;
    this._windowTracker = Shell.WindowTracker.get_default();

    try {
      /* D-Bus export */
      this._dbus = Gio.DBusExportedObject.wrapJSObject(DBusInterface, this);
      this._dbus.export(Gio.DBus.session, "/org/gnome/WellbeingTracker");

      this._ownerId = Gio.DBus.session.own_name(
        "org.gnome.WellbeingTracker",
        Gio.BusNameOwnerFlags.NONE,
        () => log("WellbeingTracker: D-Bus name acquired"),
        () => log("WellbeingTracker: D-Bus name lost"),
        (c, n, e) => logError(e, "WellbeingTracker: D-Bus error"),
      );

      /* Window focus tracking */
      this._focusId = this._display.connect("notify::focus-window", () =>
        this._onWindowChanged(),
      );

      /* Idle tracking */
      this._setupIdleTracking();

      log("WellbeingTracker: enabled");
    } catch (e) {
      logError(e, "WellbeingTracker: enable failed");
    }
  }

  /* ---------------- IDLE ---------------- */

  _setupIdleTracking() {
    this._idleMonitor = global.backend.get_core_idle_monitor();
    log(`IdleMonitor: ${this._idleMonitor}`);
    this._installIdleWatch();
  }

  _installIdleWatch() {
    if (this._idleWatch) {
      this._idleMonitor.remove_watch(this._idleWatch);
      this._idleWatch = null;
    }

    this._idleWatch = this._idleMonitor.add_idle_watch(120_000, () => {
      log("IDLE");
      this._emitIdle(true);

      // wait for activity, then re-arm idle watch
      this._activeWatch = this._idleMonitor.add_user_active_watch(() => {
        log("ACTIVE");
        this._emitIdle(false);
        this._installIdleWatch();
      });
    });
  }

  _emitIdle(idle) {
    try {
      this._dbus.emit_signal(
        "IdleStatusChanged",
        new GLib.Variant("(b)", [idle]),
      );
    } catch (e) {
      logError(e, "WellbeingTracker: Idle signal failed");
    }
  }

  /* ---------------- WINDOW ---------------- */

  _onWindowChanged() {
    const win = this._display.focus_window;
    if (!win) return;

    const title = win.get_title() ?? "";
    const app = this._windowTracker.get_window_app(win);
    const appId = app ? app.get_id() : "";
    const name = getNiceName(appId);
    const timestamp = GLib.get_real_time();

    try {
      this._dbus.emit_signal(
        "WindowChanged",
        new GLib.Variant("(ssx)", [title, name, timestamp]),
      );
    } catch (e) {
      logError(e, "WellbeingTracker: WindowChanged failed");
    }
  }

  /* ---------------- D-BUS METHOD ---------------- */

  GetCurrentWindow() {
    const win = this._display.focus_window;
    const title = win ? (win.get_title() ?? "") : "";
    const app = win ? this._windowTracker.get_window_app(win) : null;
    const appId = app ? app.get_id() : "";

    return new GLib.Variant("(ssx)", [
      title,
      getNiceName(appId),
      GLib.get_real_time(),
    ]);
  }

  /* ---------------- DISABLE ---------------- */

  disable() {
    if (this._focusId) {
      this._display.disconnect(this._focusId);
      this._focusId = null;
    }

    if (this._idleWatch) {
      this._idleMonitor.remove_watch(this._idleWatch);
      this._idleWatch = null;
    }

    if (this._activeWatch) {
      this._idleMonitor.remove_watch(this._activeWatch);
      this._activeWatch = null;
    }

    if (this._ownerId) {
      Gio.DBus.session.unown_name(this._ownerId);
      this._ownerId = null;
    }

    if (this._dbus) {
      this._dbus.unexport();
      this._dbus = null;
    }

    this._display = null;
    this._windowTracker = null;
    this._idleMonitor = null;

    log("WellbeingTracker: disabled");
  }
}
