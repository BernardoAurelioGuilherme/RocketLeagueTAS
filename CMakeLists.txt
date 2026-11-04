cmake_minimum_required(VERSION 3.20)
project(RocketLeagueTAS VERSION 0.1.0 LANGUAGES CXX)

if(NOT MSVC)
	message(FATAL_ERROR "BakkesMod's pluginsdk.lib uses the Microsoft C++ ABI. Build this plugin with MSVC (or an MSVC-compatible toolchain), not MinGW.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BAKKESMOD_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/bakkesmod")

add_library(RocketLeagueTAS SHARED
	src/main.cpp
	include/imgui/imgui.cpp
	include/imgui/imgui_draw.cpp
	include/imgui/imgui_widgets.cpp
)

target_include_directories(RocketLeagueTAS PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/imgui"
	"${BAKKESMOD_SDK_DIR}/include"
	"${BAKKESMOD_SDK_DIR}/include/bakkesmod"
	"${BAKKESMOD_SDK_DIR}/include/bakkesmod/wrappers"
	"${BAKKESMOD_SDK_DIR}/include/bakkesmod/wrappers/items"
	"${BAKKESMOD_SDK_DIR}/include/bakkesmod/wrappers/items/dbs"
)

target_link_libraries(RocketLeagueTAS PRIVATE
	"${BAKKESMOD_SDK_DIR}/lib/pluginsdk.lib"
)

set_target_properties(RocketLeagueTAS PROPERTIES
	PREFIX ""
	OUTPUT_NAME "RocketLeagueTAS"
)

