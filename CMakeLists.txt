cmake_minimum_required(VERSION 3.5)

project(tcg_parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tcg_parser main.cpp
	device_path.hpp
	events.hpp
	tcg_parser.hpp
	acpi.hpp)
