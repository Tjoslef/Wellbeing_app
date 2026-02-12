#include "main.hpp"
#include <mutex>
#include <sqlite3.h>
#include <vector>

using json = nlohmann::json;
class DB{
    public:
    explicit DB(const std::string& db_path);
    void DB_call(const WindowInfo &input, std::chrono::system_clock::duration time_spent,std::chrono::system_clock::time_point last_update);
    std::vector<WindowInfo> getAllLogs(tm *ltm_today);
    std::string getLogsAsJson();
    void cleanUp();
    ~DB();
    private:
    sqlite3* DB_point;
    std::mutex database_zapis;
};

inline void to_json(json& j, const WindowInfo& w) {
    j = json{
        {"id", w.ID},
        {"title", w.Title},
        {"time_spend", w.timestamp}
    };
}
