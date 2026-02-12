#pragma once
#include <cstddef>
#include <cstdint>
#include <sdbus-c++.h>
#include <iostream>
#include <string>
#include <mutex>
#include <stdarg.h>
#include "json.hpp"
struct WindowInfo{
    std::string Title;
    std::string ID;
    int64_t timestamp;
};
void printLog(const char *fmt, ...);
void printWindow(const WindowInfo &window);
