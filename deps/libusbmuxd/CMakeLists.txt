project(libusbmuxd)
cmake_minimum_required(VERSION 3.2)

set(libusbmuxd_HEADERS
	common/collection.h
	common/socket.h
	src/msc_config.h
)

set(libusbmuxd_SOURCES
	common/collection.c
	common/socket.c
	src/libusbmuxd.c
)

include_directories(libusbmuxd include
	common
	../libplist/include)

add_library(libusbmuxd STATIC
	${libusbmuxd_SOURCES}
	${libusbmuxd_HEADERS})



# install_obs_plugin_with_data(win-capture data)

add_subdirectory(../libplist libplist)
# add_subdirectory(get-graphics-offsets)
# add_subdirectory(inject-helper)

target_link_libraries(libusbmuxd
	libplist)