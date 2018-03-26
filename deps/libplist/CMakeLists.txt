project(libplist)

set(libplist_HEADERS
	src/base64.h
	src/bytearray.h
	src/hashtable.h
	src/msc_config.h
	src/plist.h
	src/ptrarray.h
	src/strbuf.h
	src/time64_limits.h
	src/time64.h
	include/plist/Array.h
	include/plist/Boolean.h
	include/plist/Data.h
	include/plist/Date.h
	include/plist/Dictionary.h
	include/plist/Integer.h
	include/plist/Key.h
	include/plist/Node.h
	include/plist/plist.h
	include/plist/plist++.h
	include/plist/Real.h
	include/plist/String.h
	include/plist/Structure.h
	include/plist/Uid.h
)

include_directories(libplist include
	src
	libcnary/include)

set(libplist_SOURCES
	src/Array.cpp
	src/base64.c
	src/Boolean.cpp
	src/bplist.c
	src/bytearray.c
	src/Data.cpp
	src/Date.cpp
	src/Dictionary.cpp
	src/hashtable.c
	src/Integer.cpp
	src/Key.cpp
	src/Node.cpp
	src/plist.c
	src/ptrarray.c
	src/Real.cpp
	src/String.cpp
	src/Structure.cpp
	src/time64.c
	src/Uid.cpp
	src/xplist.c
)

add_library(libplist STATIC
	${libplist_SOURCES}
	${libplist_HEADERS})



# install_obs_plugin_with_data(win-capture data)

add_subdirectory(libcnary libcnary)
# add_subdirectory(get-graphics-offsets)
# add_subdirectory(inject-helper)

target_link_libraries(libplist
	libcnary)