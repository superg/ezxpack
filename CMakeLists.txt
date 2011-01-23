cmake_minimum_required(VERSION 2.8)
project(EZXPack)

set (EZXPack_VERSION_MAJOR 1)
set (EZXPack_VERSION_MINOR 0)

configure_file(
	"${PROJECT_SOURCE_DIR}/src/ezxpack_cfg.h.in"
	"${PROJECT_BINARY_DIR}/src/ezxpack_cfg.h"
)
include_directories("${PROJECT_BINARY_DIR}/src")

add_subdirectory(src)
