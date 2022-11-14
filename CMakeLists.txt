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
include_directories(../../lib/include)
add_library(RemoteCaptury SHARED RemoteCaptury.cpp)
add_library(RemoteCapturyStatic STATIC RemoteCaptury.cpp)

message(STATUS "---------------- RemoteCaptury : Python setup starts ... -----------------")

find_package(Python3 COMPONENTS Development.Module Numpy)
message(STATUS "Python3_FOUND ? : ${Python3_FOUND}")

if (NOT Python3_FOUND)
	message(WARNING "Python3 not found, trying via PythonLibs")

	# finding python libs
	find_package(PythonLibs 3 REQUIRED)
	message(STATUS "PythonLibs_FOUND ? : ${PythonLibs_FOUND}")

	# numpy inclusion
	find_path(PYTHON_NUMPY_INCLUDE_DIR numpy/arrayobject.h  HINTS "${PYTHON_INCLUDE_PATH}" NO_DEFAULT_PATH)
	message(STATUS "PYTHON_NUMPY_INCLUDE_DIR ? : ${PYTHON_NUMPY_INCLUDE_DIR}")
	message(STATUS "Python3::NumPy version : ${Python3_NumPy_VERSION}")

	# python 3
	if (PYTHONLIBS_FOUND AND (NOT TARGET Python3::Python))
		add_library(Python3::Python UNKNOWN IMPORTED)
		set_target_properties(Python3::Python PROPERTIES
			IMPORTED_LOCATION ${PYTHON_LIBRARY}
			INTERFACE_INCLUDE_DIRECTORIES ${PYTHON_INCLUDE_DIR})
		set(Python3_FOUND ON)
	endif (PYTHONLIBS_FOUND AND (NOT TARGET Python3::Python))

endif ()
message(STATUS "---------------- RemoteCaptury : Python setup ends. -----------------")
#[[ if (NOT TARGET Python3::Python)
	find_package(PythonLibs)
	if (PYTHONLIBS_FOUND)
		add_library(Python3::Python UNKNOWN IMPORTED)
		set_target_properties(Python3::Python PROPERTIES
			IMPORTED_LOCATION ${PYTHON_LIBRARY}
			INTERFACE_INCLUDE_DIRECTORIES ${PYTHON_INCLUDE_DIR})
		set(Python3_FOUND ON)
	endif (PYTHONLIBS_FOUND)

endif (NOT TARGET Python3::Python) ]]

if (TARGET Python3::Python)
	add_library(RemoteCapturyPython SHARED RemoteCapturyPython.cpp RemoteCaptury.cpp)
	target_link_libraries(RemoteCapturyPython Python3::Python Python3::NumPy)
	set_target_properties(RemoteCapturyPython
		PROPERTIES
			PREFIX ""
			OUTPUT_NAME "remotecaptury"
			LINKER_LANGUAGE C
	)
	if (WIN32)
		set_target_properties(RemoteCapturyPython
			PROPERTIES
				SUFFIX ".pyd"
				FOLDER "plugins"
		)
	endif (WIN32)
endif (TARGET Python3::Python)
