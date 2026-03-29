cmake_minimum_required(VERSION 3.16)
project(AigleDor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.6 COMPONENTS Widgets Multimedia REQUIRED)

# --- Core Library ---
# We bundle the emulation core into a static library
file(GLOB_RECURSE CORE_SOURCES
    "Core/*.cpp"
    "Core/*.h"
)

add_library(OricCore STATIC ${CORE_SOURCES})
target_include_directories(OricCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Core)

# --- GUI Application ---
file(GLOB_RECURSE GUI_SOURCES
    "GUI/*.cpp"
    "GUI/*.h"
)

add_executable(AigleDor
    main.cpp
    ${GUI_SOURCES}
)

target_link_libraries(AigleDor
    PRIVATE
    OricCore
    Qt6::Widgets
    Qt6::Multimedia
)
