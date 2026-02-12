#include "main.hpp"
#include "d_bus_com.hpp"
#include "db_call.hpp"
#include "json.hpp"
#include <vector>
class Tracker{
    public:
    DBusListener dbus;
    DB database;
    void saveEvent(WindowInfo windowInfo);
    void weekStat(std::vector<WindowInfo> logs);
    void periodUpdate();
    void signal_handler(int signal_handler);
    Tracker() : database("../data/App.db") {}
    private:
    WindowInfo current_window_;
    std::mutex mtx_;
    std::chrono::system_clock::time_point last_change;
};
