cmake_minimum_required(VERSION 3.2)
project(obs-ios-camera-source)

set(CMAKE_PREFIX_PATH "${QTDIR}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set (CMAKE_CXX_STANDARD 17)

if (WIN32 OR APPLE)
    include(external/FindLibObs.cmake)
endif()

find_package(LibObs REQUIRED)

find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil)
# find_ffmpeg_library(avcodec)
# find_ffmpeg_library(avutil)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(ARCH 64)
else()
	set(ARCH 32)
endif()

if(WIN32)
	find_library(WS2_32_LIBRARY ws2_32)
endif()

# ---- libcnary

set(libcnary_HEADERS
	deps/libplist/libcnary/include/iterator.h
	deps/libplist/libcnary/include/list.h
	deps/libplist/libcnary/include/node_iterator.h
	deps/libplist/libcnary/include/node_list.h
	deps/libplist/libcnary/include/node.h
	deps/libplist/libcnary/include/object.h
)

set(libcnary_SOURCES
	deps/libplist/libcnary/cnary.c
	deps/libplist/libcnary/iterator.c
	deps/libplist/libcnary/list.c
	deps/libplist/libcnary/node_iterator.c
	deps/libplist/libcnary/node_list.c
	deps/libplist/libcnary/node.c
)

include_directories(libcnary deps/libplist/libcnary/include)

add_library(libcnary STATIC
	${libcnary_SOURCES}
	${libcnary_HEADERS})

#------- libplist

set(libplist_HEADERS
	deps/libplist/src/base64.h
	deps/libplist/src/bytearray.h
	deps/libplist/src/hashtable.h
	deps/libplist/src/msc_config.h
	deps/libplist/src/plist.h
	deps/libplist/src/ptrarray.h
	deps/libplist/src/strbuf.h
	deps/libplist/src/time64_limits.h
	deps/libplist/src/time64.h
	deps/libplist/include/plist/Array.h
	deps/libplist/include/plist/Boolean.h
	deps/libplist/include/plist/Data.h
	deps/libplist/include/plist/Date.h
	deps/libplist/include/plist/Dictionary.h
	deps/libplist/include/plist/Integer.h
	deps/libplist/include/plist/Key.h
	deps/libplist/include/plist/Node.h
	deps/libplist/include/plist/plist.h
	deps/libplist/include/plist/plist++.h
	deps/libplist/include/plist/Real.h
	deps/libplist/include/plist/String.h
	deps/libplist/include/plist/Structure.h
	deps/libplist/include/plist/Uid.h
)

include_directories(libplist deps/libplist/include
	deps/libplist/src
	deps/libplist/libcnary/include)

set(libplist_SOURCES
	deps/libplist/src/Array.cpp
	deps/libplist/src/base64.c
	deps/libplist/src/Boolean.cpp
	deps/libplist/src/bplist.c
	deps/libplist/src/bytearray.c
	deps/libplist/src/Data.cpp
	deps/libplist/src/Date.cpp
	deps/libplist/src/Dictionary.cpp
	deps/libplist/src/hashtable.c
	deps/libplist/src/Integer.cpp
	deps/libplist/src/Key.cpp
	deps/libplist/src/Node.cpp
	deps/libplist/src/plist.c
	deps/libplist/src/ptrarray.c
	deps/libplist/src/Real.cpp
	deps/libplist/src/String.cpp
	deps/libplist/src/Structure.cpp
	deps/libplist/src/time64.c
	deps/libplist/src/Uid.cpp
	deps/libplist/src/xplist.c
)

add_library(libplist STATIC
	${libplist_SOURCES}
	${libplist_HEADERS})

target_link_libraries(libplist
	libcnary
	)

#------- libusbmuxd

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPACKAGE_STRING=\\\"obs-ios-camera\\\"")

set(libusbmuxd_HEADERS
	deps/libusbmuxd/common/collection.h
	deps/libusbmuxd/common/socket.h
	# deps/libusbmuxd/src/msc_config.h
)

set(libusbmuxd_SOURCES
	deps/libusbmuxd/common/thread.c
	deps/libusbmuxd/common/collection.c
	deps/libusbmuxd/common/socket.c
	deps/libusbmuxd/src/libusbmuxd.c
)

include_directories(libusbmuxd include
	deps/libusbmuxd/common
	deps/libusbmuxd/include)

add_library(libusbmuxd STATIC
	${libusbmuxd_SOURCES}
	${libusbmuxd_HEADERS})

target_link_libraries(libusbmuxd
	libplist
)

if(WIN32)
  target_link_libraries(libusbmuxd wsock32 ws2_32)
endif()


# -------- Portal Lib

set(portal_HEADERS
	deps/portal/src/Channel.hpp
	deps/portal/src/Device.hpp
	deps/portal/src/Portal.hpp
	deps/portal/src/Protocol.hpp
	deps/portal/src/logging.h
)

set(portal_SOURCES
	deps/portal/src/Channel.cpp
	deps/portal/src/Device.cpp
	deps/portal/src/Portal.cpp
	deps/portal/src/Protocol.cpp
)

include_directories(portal include
	deps/portal/src/
)

add_library(portal STATIC
	${portal_SOURCES}
	${portal_HEADERS})

target_link_libraries(portal
	libusbmuxd
)


## -- 

set(ENABLE_PROGRAMS false)

set(obs-ios-camera-source_SOURCES
	src/obs-ios-camera-plugin.cpp
	src/obs-ios-camera-source.cpp
	src/ffmpeg-decode.c
	src/VideoDecoder.cpp
	src/FFMpegVideoDecoder.cpp
	src/FFMpegAudioDecoder.cpp
	src/Thread.cpp)

set(obs-ios-camera-source_HEADERS
	src/obs-ios-camera-source.h
	src/ffmpeg-decode.h
	src/VideoDecoder.h
	src/FFMpegVideoDecoder.h
	src/FFMpegAudioDecoder.h
	src/Thread.hpp
	src/Queue.hpp)

if(APPLE)

	message("Linking Video Toolbox")

	find_library(AVFOUNDATION AVFoundation)
	find_library(COCOA Cocoa)
	find_library(COREFOUNDATION CoreFoundation)
	find_library(COREMEDIA CoreMedia)	
	find_library(COREVIDEO CoreVideo)
	find_library(COREMEDIAIO CoreMediaIO)
	find_library(VIDEOTOOLBOX VideoToolbox)

	include_directories(${AVFOUNDATION}
		${COCOA}
		${COREFOUNDATION}
		${COREMEDIA}
		${COREVIDEO}
		${COREMEDIAIO}
		${VIDEOTOOLBOX}
		${COCOA})

	list(APPEND obs-ios-camera-source_SOURCES
		src/VideoToolboxVideoDecoder.cpp)
	
	list(APPEND obs-ios-camera-source_HEADERS
		src/VideoToolboxVideoDecoder.h)

	add_definitions(-fobjc-arc)


endif()

# --- Platform-independent build settings ---
add_library(obs-ios-camera-source MODULE 
	${obs-ios-camera-source_SOURCES}
	${obs-ios-camera-source_HEADERS})


#find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil)

include_directories( 
	"${LIBOBS_INCLUDE_DIR}/../UI/obs-frontend-api"
	${FFMPEG_INCLUDE_DIRS}
)

if(MSVC)
	# https://stackoverflow.com/q/33290879
	# needed for some MSVC installations
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")

endif()


target_link_libraries(obs-ios-camera-source 
	libobs
	portal
	${FFMPEG_LIBRARIES}
)
 
# --- End of section ---

# --- Windows-specific build settings and tasks ---
if(WIN32)
	if(NOT DEFINED OBS_FRONTEND_LIB)
		set(OBS_FRONTEND_LIB "OBS_FRONTEND_LIB-NOTFOUND" CACHE FILEPATH "OBS frontend library")
		message(FATAL_ERROR "Could not find OBS Frontend API\'s library !")
	endif()

	if(MSVC)
        # Enable Multicore Builds and disable FH4 (to not depend on VCRUNTIME140_1.DLL)
	    add_definitions(/MP /d2FH4-)
    endif()

	if(ARCH EQUAL 64)
		set(OBS_ARCH_NAME "64bit")
		set(OBS_BUILDDIR_ARCH "build64")
	else()
		set(OBS_ARCH_NAME "32bit")
		set(OBS_BUILDDIR_ARCH "build32")
		add_definitions(/arch:SSE2)
	endif()

	include_directories(
		"${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/UI"
	)

	# target_link_libraries(obs-ios-camera-source
		# "${OBS_FRONTEND_LIB}")

	# --- Release package helper ---
	# The "release" folder has a structure similar OBS' one on Windows
	set(RELEASE_DIR "${PROJECT_SOURCE_DIR}/release")

	add_custom_command(TARGET obs-ios-camera-source POST_BUILD
		# If config is Release, package release files
		COMMAND if $<CONFIG:Release>==1 (
			"${CMAKE_COMMAND}" -E make_directory
				"${RELEASE_DIR}/data/obs-plugins/obs-ios-camera-source"
				"${RELEASE_DIR}/obs-plugins/${OBS_ARCH_NAME}")

		COMMAND if $<CONFIG:Release>==1 (
			"${CMAKE_COMMAND}" -E copy_directory
				"${PROJECT_SOURCE_DIR}/data"
				"${RELEASE_DIR}/data/obs-plugins/obs-ios-camera-source")

		COMMAND if $<CONFIG:Release>==1 (
			"${CMAKE_COMMAND}" -E copy
				"$<TARGET_FILE:obs-ios-camera-source>"
				"${RELEASE_DIR}/obs-plugins/${OBS_ARCH_NAME}")
				

        # If config is RelWithDebInfo, package release files
		COMMAND if $<CONFIG:RelWithDebInfo>==1 (
			"${CMAKE_COMMAND}" -E make_directory
			    "${RELEASE_DIR}/data/obs-plugins/obs-ios-camera-source"
			    "${RELEASE_DIR}/obs-plugins/${OBS_ARCH_NAME}")

		COMMAND if $<CONFIG:RelWithDebInfo>==1 (
            "${CMAKE_COMMAND}" -E copy_directory
			    "${PROJECT_SOURCE_DIR}/data"
			    "${RELEASE_DIR}/data/obs-plugins/obs-ios-camera-source")

		COMMAND if $<CONFIG:RelWithDebInfo>==1 (
            "${CMAKE_COMMAND}" -E copy
			    "$<TARGET_FILE:obs-ios-camera-source>"
			    "${RELEASE_DIR}/obs-plugins/${OBS_ARCH_NAME}")

		COMMAND if $<CONFIG:RelWithDebInfo>==1 (
            "${CMAKE_COMMAND}" -E copy
			    "$<TARGET_PDB_FILE:obs-ios-camera-source>"
			    "${RELEASE_DIR}/obs-plugins/${OBS_ARCH_NAME}")


		# Copy to obs-studio dev environment for immediate testing
		COMMAND if $<CONFIG:Debug>==1 (
			"${CMAKE_COMMAND}" -E copy
				"$<TARGET_FILE:obs-ios-camera-source>"
				"${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/obs-plugins/${OBS_ARCH_NAME}")

		COMMAND if $<CONFIG:Debug>==1 (
			"${CMAKE_COMMAND}" -E make_directory
				"${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/obs-ios-camera-source")

		COMMAND if $<CONFIG:Debug>==1 (
			"${CMAKE_COMMAND}" -E copy_directory
				"${PROJECT_SOURCE_DIR}/data"
				"${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/obs-ios-camera-source")
	)
	# --- End of sub-section ---

endif()
# --- End of section ---

# -- OS X specific build settings and tasks --
if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fvisibility=default")
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-function -Werror-implicit-function-declaration -Wno-missing-field-initializers ${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
	set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused-function -Werror-implicit-function-declaration -Wno-missing-braces -Wno-missing-field-initializers ${CMAKE_C_FLAGS} -std=gnu99 -fno-strict-aliasing -DHAVE_STPNCPY=1")
	
	set_target_properties(obs-ios-camera-source PROPERTIES PREFIX "")
	target_link_libraries(obs-ios-camera-source "${OBS_FRONTEND_LIB}"
		${AVFOUNDATION}
		${COCOA}
		${COREFOUNDATION}
		${COREMEDIA}
		${COREVIDEO}
		${COREMEDIAIO}
		${VIDEOTOOLBOX}
		${COCOA})
endif()
# -- End of section --
