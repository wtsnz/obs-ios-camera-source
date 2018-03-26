project(libcnary)

set(libcnary_HEADERS
	include/iterator.h
	include/list.h
	include/node_iterator.h
	include/node_list.h
	include/node.h
	include/object.h
)

set(libcnary_SOURCES
	cnary.c
	iterator.c
	list.c
	node_iterator.c
	node_list.c
	node.c
)

include_directories(libcnary include)

add_library(libcnary STATIC
	${libcnary_SOURCES}
	${libcnary_HEADERS})

# target_link_libraries(libplist
# 	# 
# 	# ipc-util
# 	# psapi
# 	)

# install_obs_plugin_with_data(win-capture data)

# add_subdirectory(graphics-hook)
# add_subdirectory(get-graphics-offsets)
# add_subdirectory(inject-helper)
