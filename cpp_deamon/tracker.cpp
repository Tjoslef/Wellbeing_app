#include "tracker.hpp"
#include <mutex>
void Tracker::saveEvent(WindowInfo windowInfo){
    std::lock_guard<std::mutex> lock(mtx_);
    auto now = std::chrono::system_clock::now();
    std::chrono::system_clock::duration time_spent = std::chrono::system_clock::duration::zero();
    if(!current_window_.ID.empty()){
    time_spent = now - last_change;
    database.DB_call(current_window_,time_spent,now);
    //volani databaze s widoInfo a time_spent pokud je time_spent = 0 pouze se vytvori pokud neni zapis o te aplikaci v databazi
    }
    current_window_ = windowInfo;
   last_change = now;
}
void Tracker::periodUpdate(){
    std::lock_guard<std::mutex> lock(mtx_);
    if(current_window_.ID.empty()){return;}
    std::chrono::system_clock::duration time_spent = std::chrono::system_clock::duration::zero();
    auto now = std::chrono::system_clock::now();
    time_spent = now - last_change;
    database.DB_call(current_window_,time_spent,now);
    last_change = now;
}
