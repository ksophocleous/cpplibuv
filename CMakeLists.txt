cmake_minimum_required (VERSION 2.8)

project(cpplibuv)

cmake_policy(SET CMP0015 NEW)

set(VENDOR_ROOT "${PROJECT_SOURCE_DIR}/vendor")
set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/")

if (UNIX)
	set(LIBS "uv")
	set(LIBS_DIR "${VENDOR_ROOT}/libuv/.libs")
	
	set(COMPILER_SPECIFIC_COMPILE_FLAGS "")
	set(COMPILER_SPECIFIC_LINK_FLAGS "")
else()
	set(LIBS "libuv")
	set(LIBS_DIR "${VENDOR_ROOT}/libuv/Release/lib")

	set(LIBS ${LIBS} "ws2_32" "kernel32" "Iphlpapi" "Psapi.lib")

	set(COMPILER_SPECIFIC_COMPILE_FLAGS "")
	set(COMPILER_SPECIFIC_LINK_FLAGS "/LTCG /NODEFAULTLIB:LIBCMT")
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_SPECIFIC_COMPILE_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COMPILER_SPECIFIC_LINK_FLAGS}" )

include_directories("${VENDOR_ROOT}/libuv/include")
include_directories("${PROJECT_SOURCE_DIR}/include")

link_directories (${LIBS_DIR})

add_executable(cpplibuv main.cpp)
target_link_libraries(cpplibuv ${LIBS})

install (TARGETS cpplibuv DESTINATION bin)
