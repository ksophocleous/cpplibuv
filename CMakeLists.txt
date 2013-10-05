cmake_minimum_required (VERSION 2.8)

project(cpplibuv)

cmake_policy(SET CMP0015 NEW)

set(VENDOR_ROOT "${PROJECT_SOURCE_DIR}/vendor")
set(LIBS "uv")
set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/")

if (UNIX)
	set(LIBS_DIR "${VENDOR_ROOT}/libuv/.libs")
else()
	set(LIBS_DIR "${VENDOR_ROOT}/libuv/Release/lib")
endif()

include_directories("${VENDOR_ROOT}/libuv/include")
include_directories("${PROJECT_SOURCE_DIR}/include")

link_directories (${LIBS_DIR})

add_executable(cpplibuv main.cpp)
target_link_libraries(cpplibuv libuv)

install (TARGETS cpplibuv DESTINATION bin)