cmake_minimum_required(VERSION 3.0)

if (OpenMP_C_FLAGS)
	string(REPLACE ${OpenMP_C_FLAGS} "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
endif (OpenMP_C_FLAGS)
if (OpenMP_CXX_FLAGS)
	string(REPLACE ${OpenMP_CXX_FLAGS} "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif (OpenMP_CXX_FLAGS)

project(RemoteCaptury)
message(STATUS "working on remotecaptury library")
include_directories(../include)
add_library(RemoteCaptury SHARED RemoteCaptury.cpp)
add_library(RemoteCapturyStatic STATIC RemoteCaptury.cpp)

find_package (Python3 COMPONENTS Development)
if (NOT TARGET Python3::Python)
	find_package(PythonLibs)
	if (PYTHONLIBS_FOUND)
		add_library(Python3::Python UNKNOWN IMPORTED)
		set_target_properties(Python3::Python PROPERTIES
			IMPORTED_LOCATION ${PYTHON_LIBRARY}
			INTERFACE_INCLUDE_DIRECTORIES ${PYTHON_INCLUDE_DIR})
		set(Python3_FOUND ON)
	endif (PYTHONLIBS_FOUND)
endif (NOT TARGET Python3::Python)

if (TARGET Python3::Python)
	add_library(RemoteCapturyPython SHARED RemoteCapturyPython.cpp RemoteCaptury.cpp)
	target_link_libraries(RemoteCapturyPython Python3::Python)
	set_target_properties(RemoteCapturyPython
		PROPERTIES
			PREFIX ""
			OUTPUT_NAME "remotecaptury"
			LINKER_LANGUAGE C
	)
endif (TARGET Python3::Python)
