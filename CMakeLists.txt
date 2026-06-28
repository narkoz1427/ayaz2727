cmake_minimum_required(VERSION 3.22.1)
project(ayaznative)

set(CMAKE_CXX_STANDARD 17)

add_library(
    ayaznative
    SHARED
    native/src/jni_bridge.cpp
    native/src/game_reader.cpp
    native/src/farm_game_api.cpp
    native/src/farm_loot.cpp
    native/src/auto_farm.cpp
)

target_include_directories(ayaznative PRIVATE native/include)

find_library(log-lib log)
find_library(dl-lib  dl)

target_link_libraries(ayaznative
    ${log-lib}
    ${dl-lib}
)
