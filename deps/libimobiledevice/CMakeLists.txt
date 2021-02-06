project(libimobiledevice)
cmake_minimum_required(VERSION 3.2)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
set(OPENSSL_USE_STATIC_LIBS TRUE)

message("OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message("OpenSSL libraries: ${OPENSSL_LIBRARIES}")

message("other: ${OPENSSL_SSL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES}")

set(libimobiledevice_HEADERS
    #common/debug.h
    #common/socket.h
    #common/thread.h
    #common/userpref.h
    #common/utils.h

    src/afc.h
    src/companion_proxy.h
    src/debugserver.h
    src/device_link_service.h
    src/diagnostics_relay.h
    src/file_relay.h
    src/heartbeat.h
    src/house_arrest.h
    src/idevice.h
    src/installation_proxy.h
    src/lockdown.h
    src/misagent.h
    src/mobile_image_mounter.h
    src/mobileactivation.h
    src/mobilebackup.h
    src/mobilebackup2.h
    src/mobilesync.h
    src/notification_proxy.h
    src/preboard.h
    src/property_list_service.h
    src/restore.h
    src/sbservices.h
    src/screenshotr.h
    src/service.h
    src/syslog_relay.h
    src/webinspector.h
)

set(libimobiledevice_SOURCES
    common/debug.c
    common/socket.c
    common/thread.c
    common/userpref.c
    common/utils.c

    src/afc.c
    src/companion_proxy.c
    src/debugserver.c
    src/device_link_service.c
    src/diagnostics_relay.c
    src/file_relay.c
    src/heartbeat.c
    src/house_arrest.c
    src/idevice.c
    src/installation_proxy.c
    src/lockdown.c
    src/misagent.c
    src/mobile_image_mounter.c
    src/mobileactivation.c
    src/mobilebackup.c
    src/mobilebackup2.c
    src/mobilesync.c
    src/notification_proxy.c
    src/pinvoke.c
    src/preboard.c
    src/property_list_service.c
    src/restore.c
    src/sbservices.c
    src/screenshotr.c
    src/service.c
    src/syslog_relay.c
    src/webinspector.c
)

include_directories(libimobiledevice
    include/
    ${OPENSSL_INCLUDE_DIR}
)

add_library(libimobiledevice STATIC
	${libimobiledevice_SOURCES}
	${libimobiledevice_HEADERS}
)

target_include_directories(libimobiledevice
	PUBLIC include
)

target_link_libraries(libimobiledevice
PRIVATE
	libplist
    libusbmuxd
)

target_link_libraries(libimobiledevice PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

# https://gitlab.kitware.com/cmake/cmake/-/issues/19263
if(WIN32)
  target_link_libraries(libimobiledevice PRIVATE Crypt32 ws2_32)
endif()

check_function_exists(stpncpy HAVE_STPNCPY)
if(${HAVE_STPNCPY})
	add_compile_definitions(libimobiledevice HAVE_STPNCPY)
endif()

check_function_exists(asprintf HAVE_ASPRINTF)
if(${HAVE_ASPRINTF})
	add_compile_definitions(libimobiledevice HAVE_ASPRINTF)
endif()

check_function_exists(vasprintf HAVE_VASPRINTF)
if(${HAVE_VASPRINTF})
	add_compile_definitions(libimobiledevice HAVE_VASPRINTF)
endif()

add_compile_definitions(libimobiledevice HAVE_OPENSSL)
