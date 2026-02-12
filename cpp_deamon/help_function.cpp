#include "main.hpp"
#include <iostream>
void printLog(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_end(args);
}
void printWindow(const WindowInfo &window){
    std::cerr << "Nazem :" << window.Title;
    std::cerr << "ID :" << window.ID;
    std::cerr << "Timespan :" << window.timestamp;
}
