cmake_minimum_required(VERSION 3.15)
project(wellbeing-tracker CXX)

# Enable compile commands export
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Cesta k instalaci (lokální uživatel)
set(EXTENSION_UUID "wellbeing_tracker@Tjoslef")
set(EXTENSION_DIR "$ENV{HOME}/.local/share/gnome-shell/extensions/${EXTENSION_UUID}")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(JS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gnome_extension")

# Najít závislosti
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDBUS REQUIRED IMPORTED_TARGET sdbus-c++)
find_package(SQLite3 REQUIRED)

# Executable
add_executable(wellbeing-tracker
    cpp_deamon/main.cpp
    cpp_deamon/d_bus_com.cpp
    cpp_deamon/tracker.cpp
    cpp_deamon/db_call.cpp
    cpp_deamon/help_function.cpp
)

# Include directories - add sdbus-c++ include dirs
target_include_directories(wellbeing-tracker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp_deamon
    /usr/include/sdbus-c++
)

target_link_libraries(wellbeing-tracker PRIVATE
    PkgConfig::SDBUS
    SQLite::SQLite3
    pthread
)

# Cíl pro instalaci extension
add_custom_target(install_extension
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTENSION_DIR}
    # Tady specifikuj i název souboru v cíli:
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/gnome_extension/extension.js ${EXTENSION_DIR}/extension.js
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/gnome_extension/metadata.json ${EXTENSION_DIR}/metadata.json
    COMMENT "Instalace extension do GNOME Shellu..."
)

# Automatické zapnutí
add_custom_target(enable_extension
    COMMAND gnome-extensions enable ${EXTENSION_UUID}
    DEPENDS install_extension
    COMMENT "Zapínám extension v GNOME..."
)

# Standardní instalace C++ binárky
install(TARGETS wellbeing-tracker DESTINATION bin)