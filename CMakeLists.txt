cmake_minimum_required(VERSION 3.12)

if (OpenMP_C_FLAGS)
	string(REPLACE ${OpenMP_C_FLAGS} "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
endif (OpenMP_C_FLAGS)
if (OpenMP_CXX_FLAGS)
	string(REPLACE ${OpenMP_CXX_FLAGS} "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif (OpenMP_CXX_FLAGS)

project(RemoteCaptury)

set (CMAKE_CXX_STANDARD 11)

message(STATUS "working on remotecaptury library")
include_directories(../include)
add_library(RemoteCaptury SHARED RemoteCaptury.cpp)
add_library(RemoteCapturyStatic STATIC RemoteCaptury.cpp)

set(Python3_USE_STATIC_LIBS OFF)
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
	if (WIN32)
		set_target_properties(RemoteCapturyPython
			PROPERTIES
				SUFFIX ".pyd"
				FOLDER "plugins"
		)
	elseif (APPLE)
		set_target_properties(RemoteCapturyPython
			PROPERTIES
				SUFFIX ".so"
		)
	endif ()
endif (TARGET Python3::Python)

if (ANDROID_NDK_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Android.mk jni/Android.mk)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Application.mk jni/Application.mk)
	add_custom_target(RemoteCapturyAndroid
		COMMAND ${CMAKE_COMMAND} -E env NDK_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR} ${ANDROID_NDK_BUILD}
		DEPENDS RemoteCaptury.cpp RemoteCaptury.h
	)
endif(ANDROID_NDK_FOUND)

#
# add examples
#
find_package(Threads REQUIRED)

add_executable(PollingExample examples/polling.cpp)
target_include_directories(PollingExample PRIVATE ".")
target_link_libraries(PollingExample RemoteCapturyStatic Threads::Threads)

add_executable(CallbackExample examples/callback.cpp)
target_include_directories(CallbackExample PRIVATE ".")
target_link_libraries(CallbackExample RemoteCapturyStatic Threads::Threads)

add_executable(CurvesExample examples/curves.cpp)
target_include_directories(CurvesExample PRIVATE ".")
target_link_libraries(CurvesExample RemoteCapturyStatic Threads::Threads)

add_executable(SearchExample examples/search.cpp)
target_include_directories(SearchExample PRIVATE ".")
target_link_libraries(SearchExample RemoteCapturyStatic Threads::Threads)

#
# Objective-C wrapper target
#
find_program(GNUSTEP_CONFIG gnustep-config)
if (GNUSTEP_CONFIG)
	execute_process(COMMAND ${GNUSTEP_CONFIG} --objc-flags OUTPUT_VARIABLE GNUSTEP_OBJC_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND ${GNUSTEP_CONFIG} --base-libs OUTPUT_VARIABLE GNUSTEP_BASE_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
	set(GNUSTEP_FOUND TRUE)
endif ()

if (APPLE)
	enable_language(OBJCXX)
	enable_language(OBJC)
	add_library(RemoteCapturyObjC SHARED objc/RemoteCapturyObjC.mm RemoteCaptury.cpp)
	target_include_directories(RemoteCapturyObjC PUBLIC "." "objc")
	target_link_libraries(RemoteCapturyObjC "-framework Foundation")

	add_executable(ObjCExample examples/objc_example.m)
	target_include_directories(ObjCExample PRIVATE "." "objc")
	target_link_libraries(ObjCExample RemoteCapturyObjC "-framework Foundation")

	add_custom_target(Examples DEPENDS PollingExample CallbackExample CurvesExample SearchExample ObjCExample)
elseif (GNUSTEP_FOUND)
	enable_language(OBJCXX)
	enable_language(OBJC)
	file(GLOB GCC_OBJC_INC "/usr/lib/gcc/*/*/include")

	# Create a temporary include wrapper directory for objc/blocks_runtime.h if using libblocksruntime
	set(OBJC_BLOCKS_WRAPPER_DIR "${CMAKE_CURRENT_BINARY_DIR}/objc_blocks_wrapper")
	file(MAKE_DIRECTORY "${OBJC_BLOCKS_WRAPPER_DIR}/objc")
	file(WRITE "${OBJC_BLOCKS_WRAPPER_DIR}/objc/blocks_runtime.h" "#include <Block.h>\n")

	add_library(RemoteCapturyObjC SHARED objc/RemoteCapturyObjC.mm RemoteCaptury.cpp)
	target_include_directories(RemoteCapturyObjC PUBLIC "." "objc" ${GCC_OBJC_INC} "${OBJC_BLOCKS_WRAPPER_DIR}")
	separate_arguments(GNUSTEP_OBJC_FLAGS_LIST NATIVE_COMMAND "${GNUSTEP_OBJC_FLAGS}")
	separate_arguments(GNUSTEP_BASE_LIBS_LIST NATIVE_COMMAND "${GNUSTEP_BASE_LIBS}")
	target_compile_options(RemoteCapturyObjC PRIVATE $<$<COMPILE_LANGUAGE:OBJC,OBJCXX>:${GNUSTEP_OBJC_FLAGS_LIST}> $<$<COMPILE_LANGUAGE:OBJC,OBJCXX>:-fblocks>)
	target_link_libraries(RemoteCapturyObjC ${GNUSTEP_BASE_LIBS_LIST} objc BlocksRuntime)

	add_executable(ObjCExample examples/objc_example.m)
	set_source_files_properties(examples/objc_example.m PROPERTIES LANGUAGE OBJCXX)
	target_include_directories(ObjCExample PRIVATE "." "objc" ${GCC_OBJC_INC} "${OBJC_BLOCKS_WRAPPER_DIR}")
	target_compile_options(ObjCExample PRIVATE $<$<COMPILE_LANGUAGE:OBJC,OBJCXX>:${GNUSTEP_OBJC_FLAGS_LIST}> $<$<COMPILE_LANGUAGE:OBJC,OBJCXX>:-fblocks>)
	target_link_libraries(ObjCExample RemoteCapturyObjC ${GNUSTEP_BASE_LIBS_LIST} objc BlocksRuntime)

	add_custom_target(Examples DEPENDS PollingExample CallbackExample CurvesExample SearchExample ObjCExample)
else ()
	add_custom_target(Examples DEPENDS PollingExample CallbackExample CurvesExample SearchExample)
endif ()
