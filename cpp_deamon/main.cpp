#include <csignal>
#include <iostream>
#include <stop_token>
#include <thread>
#include "d_bus_com.hpp"
#include "tracker.hpp"
#include "httplib.h"
#include "main.hpp"
std::promise<void> exit_signal;
void signal_handler(int signal_handler){
    std::cout << "\n[Signal] Zachycen Ctrl+C. Ukončuji..." << std::endl;
    exit_signal.set_value();
}
int main(){
    Tracker new_session;
    new_session.database.cleanUp();
    httplib::Server svr;
    std::jthread server_thread([&new_session,&svr](std::stop_token token){
        std::stop_callback callback(token,[&svr](){
            std::cout << "Zastavuji HTTP server..." << std::endl;
            svr.stop();
        });
        svr.Get("/logs", [&new_session](const httplib::Request&, httplib::Response& res) {
            res.set_header("Access-Control-Allow-Origin", "*");
            try {
                std::string json_data = new_session.database.getLogsAsJson();
                res.set_content(json_data, "application/json");
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content("Internal Server Error", "text/plain");
            }
        });
        std::cout << "Server běží na http://localhost:8080/logs" << std::endl;
        svr.listen("0.0.0.0", 8080);
    });
    std::signal(SIGINT,signal_handler);
    WindowInfo window = {};
    auto future = exit_signal.get_future();
    std::cout << "before windowchanged";
    new_session.dbus.onWindowChanged([&new_session](const WindowInfo& info) {
       //     std::cout << "Změna zachycena! Nový titul: " << info.Title << std::endl;

            new_session.saveEvent(info);
    });
    std::cout << "before onTimerTick";
    new_session.dbus.onTimerTick([&new_session]() {
            new_session.periodUpdate();
    });
    bool is_user_idle = false;
std::cout << "before idle handler";
    new_session.dbus.IdleHandler([&new_session](bool status) {
        if (status) {
                std::cout << "Uživatel je neaktivní - pozastavuji měření..." << std::endl;
                new_session.dbus.setRunning(false); // Musíš si udělat metodu pro bezpečné nastavení
            } else {
                std::cout << "Uživatel je zpět! Pokračuji..." << std::endl;
                new_session.dbus.setRunning(true);
            }
    });
    window = new_session.dbus.getCurrentWindow();
    new_session.dbus.run();
    server_thread.detach();
    future.wait();
    server_thread.request_stop();
    new_session.dbus.stop();
    return 0;
}
