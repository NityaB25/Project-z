cmake_minimum_required(VERSION 3.20)

project(Z LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Z WIN32
    src/main.cpp
    src/gui/MainWindow.cpp
    src/core/SensitiveZoneManager.cpp
)

target_link_libraries(Z
    user32
    comctl32
)
