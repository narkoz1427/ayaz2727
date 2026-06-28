cmake_minimum_required(VERSION 3.22.1)
project(ayaznative)

set(CMAKE_CXX_STANDARD 17)

add_library(
    ayaznative
    SHARED
    src/jni_bridge.cpp
    src/game_reader.cpp
    src/farm_game_api.cpp
    src/farm_loot.cpp
    src/auto_farm.cpp
)

target_include_directories(ayaznative PRIVATE include)

find_library(log-lib log)
find_library(dl-lib  dl)

target_link_libraries(ayaznative
    ${log-lib}
    ${dl-lib}
)
