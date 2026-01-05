cmake_minimum_required(VERSION 3.15)
project(ProjectZ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GUI application (WinMain)
add_executable(Z WIN32
    src/main.cpp
    src/gui/MainWindow.cpp

    src/core/SensitiveZoneManager.cpp
    src/core/ForegroundTracker.cpp
    src/core/ActivitySessionManager.cpp
)

# Link required Windows libraries
target_link_libraries(Z
    user32
    comctl32
    psapi
)

# Ensure Unicode is used everywhere
target_compile_definitions(Z PRIVATE
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0601
)
