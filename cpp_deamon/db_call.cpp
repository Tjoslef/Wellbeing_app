#include "db_call.hpp"
#include "main.hpp"
#include <chrono>
#include <ctime>
#include <iostream>
#include <mutex>
#include <ostream>
#include <sqlite3.h>
#include <vector>

DB::DB(const std::string& db_path) : DB_point(nullptr)
{
    int rc = sqlite3_open_v2(db_path.c_str(), &DB_point,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                             nullptr);
    std::cout << "SQLite thread mode: " << sqlite3_threadsafe() << std::endl;
    if (rc != SQLITE_OK) {
        std::cerr << "DB se nepodařilo otevřít! Chyba: " << sqlite3_errmsg(DB_point) << std::endl;
    } else {
        std::cout << "DB úspěšně otevřena na adrese: " << (void*)DB_point << std::endl;
    }
    char* err = nullptr;
    std::string sql =
      "CREATE TABLE IF NOT EXISTS window_logs ("
      "  entry_id INTEGER PRIMARY KEY, "
      "  id TEXT NOT NULL, "
      "  title TEXT NOT NULL, "
      "  total_time INTEGER NOT NULL DEFAULT 0, "
      "  last_update INTEGER NOT NULL, "
      "  log_date TEXT NOT NULL, "
      "  created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')), "
      "  UNIQUE (id, log_date)"
      ");";



    rc = sqlite3_exec(DB_point, sql.c_str(), nullptr, nullptr, &err);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << err << std::endl;
        sqlite3_free(err);
        throw std::runtime_error("Table create failed");
    }

    // Create indexes for better query performance
    std::vector<std::string> indexes = {
        "CREATE INDEX IF NOT EXISTS idx_window_logs_id ON window_logs(id);",
        "CREATE INDEX IF NOT EXISTS idx_window_logs_last_update ON window_logs(last_update);",
    };

    for (const auto& index_sql : indexes) {
        rc = sqlite3_exec(DB_point, index_sql.c_str(), nullptr, nullptr, &err);
        if (rc != SQLITE_OK) {
            std::cerr << "Index creation error: " << err << std::endl;
            sqlite3_free(err);
        }
    }
}

void DB::DB_call(const WindowInfo &input, std::chrono::system_clock::duration time_spent, std::chrono::system_clock::time_point last_update) {
    std::lock_guard<std::mutex> lock(database_zapis);

    // Validate connection and repair if needed
    if (!DB_point || sqlite3_errcode(DB_point) == SQLITE_MISUSE) {
        std::cout << "Database connection corrupted" << std::endl;
    }
    auto seconds_spent = std::chrono::duration_cast<std::chrono::seconds>(time_spent).count();
    auto timestamp = std::chrono::system_clock::to_time_t(last_update);
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char date[11];
    std::strftime(date,sizeof(date),"%Y-%m-%d", ltm);
    const char* sql = "INSERT INTO window_logs(id, title, total_time, last_update,log_date) "
                      "VALUES (?, ?, ?, ?, ?) "
                      "ON CONFLICT(id,log_date) DO UPDATE SET "
                      "total_time = total_time + excluded.total_time, "
                      "id = excluded.id, "
                      "last_update = excluded.last_update;";

    sqlite3_stmt* stmt;
std::cout << "DEBUG: DB_point address: " << (void*)DB_point << std::endl;
    if(sqlite3_prepare_v2(DB_point, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Chyba přípravy: " << sqlite3_errmsg(DB_point) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, input.ID.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, input.Title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, seconds_spent);
        sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(timestamp));
        sqlite3_bind_text(stmt, 5, date, -1, SQLITE_TRANSIENT);

    if(sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Chyba zápisu: " << sqlite3_errmsg(DB_point) << std::endl;
    }

    sqlite3_finalize(stmt);
}
DB::~DB(){
    sqlite3_close(DB_point);
}
std::vector<WindowInfo> DB::getAllLogs(tm *ltm_today){
    std::lock_guard<std::mutex> lock(database_zapis);
    std::vector<WindowInfo> result;
    const char* sql = "SELECT id, title, total_time FROM window_logs WHERE log_date = ? ORDER BY total_time DESC";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(DB_point, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Chyba čtení: " << sqlite3_errmsg(DB_point) << std::endl;
        return result;
    }

    char date_str[11];
    std::strftime(date_str, sizeof(date_str), "%Y-%m-%d", ltm_today);
    sqlite3_bind_text(stmt, 1, date_str, -1, SQLITE_TRANSIENT);
    while(sqlite3_step(stmt) == SQLITE_ROW){
        WindowInfo result_row;
        const unsigned char* id = sqlite3_column_text(stmt, 0);
        const unsigned char* title = sqlite3_column_text(stmt, 1);
        int total_time = sqlite3_column_int(stmt, 2);

        std::cout << "ID: " << (id ? (const char*)id : "NULL")
        << " | Titul: " << (title ? (const char*)title : "NULL")
        << " | Čas: " << total_time << "s" << std::endl;

        result_row.Title = (const char*)title;
        result_row.timestamp = total_time / 60; // Convert to minutes
        result_row.ID = (const char*)id;
        result.push_back(result_row);
    }

    sqlite3_finalize(stmt);
    return result;
}
std::string DB::getLogsAsJson() {
    json root;
    time_t now = time(0);
    json history = json::array();

    for (int i = 0; i < 7; ++i) {
        time_t day_timestamp = now - (i * 24 * 3600);
        tm *ltm_day = localtime(&day_timestamp);

        char date_str[11];
        std::strftime(date_str, sizeof(date_str), "%Y-%m-%d", ltm_day);

        std::vector<WindowInfo> day_logs = getAllLogs(ltm_day);
        json day_entry;
        day_entry["date"] = date_str;
        day_entry["logs"] = day_logs;

        history.push_back(day_entry);
    }

    root["today"] = history[0]["logs"];
    root["history"] = history;
    std::string json_output = root.dump(4); // 4 je počet mezer pro odsazení (pretty print)
    std::cout << json_output << std::endl;
    return json_output;
}
//on start up
void DB::cleanUp(){
    time_t now = time(0);
    const int SEVEN_DAYS_IN_SECONDS = 7 * 24 * 60 * 60;
    time_t cleanDate = now - SEVEN_DAYS_IN_SECONDS;
    tm *ltm_day = localtime(&cleanDate);
    char date[11];
    std::strftime(date,sizeof(date),"%Y-%m-%d", ltm_day);
    std::string sql = "DELETE FROM window_logs WHERE log_date <= ?";
    sqlite3_stmt* stmt;

      if (sqlite3_prepare_v2(DB_point, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
          std::cerr << "Chyba čtení: " << sqlite3_errmsg(DB_point) << std::endl;
          return;
      }
      sqlite3_bind_text(stmt, 1,date , -1, SQLITE_TRANSIENT);
      if(sqlite3_step(stmt) != SQLITE_DONE) {
             std::cerr << "Chyba zápisu: " << sqlite3_errmsg(DB_point) << std::endl;
      }
}
