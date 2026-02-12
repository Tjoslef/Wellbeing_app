#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <sdbus-c++/sdbus-c++.h>
#include <functional>
#include "main.hpp"
using TimerCallback = std::function<void()>;
class DBusListener {
public:
    using WindowChangedCallback = std::function<void(const WindowInfo&)>;
    void IdleHandler(std::function<void(bool)> callback);
    DBusListener();

    // Získat aktuální okno při startu
    WindowInfo getCurrentWindow();

    // Registrovat callback pro změny oken
    void onWindowChanged(WindowChangedCallback callback);

    void onTimerTick(TimerCallback callback) {
            timer_callback_ = callback;

    }
    void setRunning(bool idle_status);
    // Spustit event loop (blokující)
    void run();
    void stop();
    std::atomic<bool> running_{false};
private:
    std::unique_ptr<sdbus::IConnection> connection_;
    std::unique_ptr<sdbus::IProxy> proxy_;
    std::thread listening_thread_;
    std::thread time_thread_;
    TimerCallback timer_callback_;
    std::mutex timer_mtx_;
    std::condition_variable cv;
    std::atomic<bool> is_alive{true};
    static constexpr const char* SERVICE_NAME = "org.gnome.WellbeingTracker";
    static constexpr const char* OBJECT_PATH = "/org/gnome/WellbeingTracker";
    static constexpr const char* INTERFACE_NAME = "org.gnome.WellbeingTracker";
};
