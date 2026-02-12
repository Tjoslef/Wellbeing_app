#include "d_bus_com.hpp"
#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>
DBusListener::DBusListener() {
    connection_ = sdbus::createSessionBusConnection();

    // 2. Vytvoření silně typovaných objektů pro sdbus-cpp
    sdbus::ServiceName destination{SERVICE_NAME};
    sdbus::ObjectPath path{OBJECT_PATH};

    proxy_ = sdbus::createProxy(*connection_,destination, path);
}
WindowInfo DBusListener::getCurrentWindow(){
    std::mutex data_mutex;
    std::string title;
    std::string app_id;
    int64_t timestamp;
    WindowInfo new_input = {};
    std::lock_guard<std::mutex> lock(data_mutex);
    try {
        sdbus::Struct<std::string, std::string, int64_t> current;
        proxy_->callMethod("GetCurrentWindow")
                      .onInterface(INTERFACE_NAME)
                      .storeResultsTo(title, app_id, timestamp);

        std::cout << "Počáteční okno: " << title << " (" << app_id << ")" << std::endl;
        new_input.Title = title;
        new_input.ID = app_id;
        new_input.timestamp = timestamp;
        return new_input;
    } catch (const sdbus::Error& e) {
        std::cerr << "Chyba při volání metody: " << e.getMessage() << std::endl;
        return new_input = {};
    }
}
/*
 *
 */
void DBusListener::onWindowChanged(WindowChangedCallback callback) {
    sdbus::signal_handler signal_handler = [callback](sdbus::Signal signal) {
        WindowInfo new_input;
        std::string title;
        std::string app_id;
        int64_t timestamp;

        signal >> title >> app_id >> timestamp;

        new_input.Title = title;
        new_input.ID = app_id;
        new_input.timestamp = timestamp;
        callback(new_input);
    };
    sdbus::InterfaceName iface{INTERFACE_NAME};
    sdbus::SignalName sig{"WindowChanged"};

    proxy_->registerSignalHandler(iface, sig, std::move(signal_handler));
}
void DBusListener::IdleHandler(std::function<void(bool)> callback){
    sdbus::signal_handler idle_handler = [callback](sdbus::Signal signal) {
        bool idle;
        signal >> idle;
        callback(idle);
    };
    sdbus::InterfaceName iface{INTERFACE_NAME};
    sdbus::SignalName sig{"IdleStatusChanged"};
    std::cout << "IDLE state";
    proxy_->registerSignalHandler(iface,sig,std::move(idle_handler));
}
void DBusListener::run(){
    {
    if(running_)return;
    running_ = true;
    listening_thread_ = std::thread([this](){
        try {
            connection_->enterEventLoop();
        } catch (const std::exception& e) {
        std::cerr << "D-Bus Thread Error: " << e.what() << std::endl;
        }
    });
    }
    time_thread_ = std::thread([this](){
        std::unique_lock<std::mutex> lock(timer_mtx_);
        while(is_alive){
            cv.wait(lock, [this] {
                return (!is_alive) || running_;
            });
            if(!is_alive){
                break;
            }
            if(cv.wait_for(lock,std::chrono::minutes(1)) == std::cv_status::timeout){
                if (running_ && timer_callback_) {
                    timer_callback_();
                }
            }
        }
    });
}
void DBusListener::setRunning(bool is_active) {
    {
        std::lock_guard<std::mutex> lock(timer_mtx_);
        running_ = is_active;
    }
    cv.notify_one();
}
void DBusListener::stop(){
    if(!running_)return;
    connection_->leaveEventLoop();
    running_ = false;
    is_alive = false;
    if(listening_thread_.joinable()){
        listening_thread_.join();
    }
    cv.notify_all();
    if(time_thread_.joinable()){
        time_thread_.join();
    }
}
