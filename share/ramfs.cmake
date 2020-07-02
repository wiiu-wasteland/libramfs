 cmake_minimum_required(VERSION 3.2)

# add ramfs to project
macro(ramfs_add target ramfs_dir)
	# find absolute ramfs directory
	set(RAMFS_INPUT_DIR ${ramfs_dir})
	if(NOT IS_ABSOLUTE ${ramfs_dir})
		set(RAMFS_INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${ramfs_dir}")
	endif()

	# build tar object file from directory
	add_custom_command(
		OUTPUT app.ramfs.o
		COMMAND tar -H ustar -cvf ramfs.tar -C "${RAMFS_INPUT_DIR}" .
		COMMAND ${CMAKE_LINKER} --relocatable --format binary --output app.ramfs.o ramfs.tar
		COMMAND rm -f ramfs.tar
		DEPENDS ${RAMFS_INPUT_DIR}
	)

	# build a static library from the object
	add_library(
		RAMFS
		STATIC
		app.ramfs.o
	)

	set_source_files_properties(
		app.ramfs.o
		PROPERTIES
		EXTERNAL_OBJECT true
		GENERATED true
	)

	set_target_properties(
		RAMFS
		PROPERTIES
		LINKER_LANGUAGE C
	)

	# link the tar object static library to the final target


	# find the libramfs library and headers
	find_path(
		RAMFS_INCLUDE_DIR
		ramfs.h
	)

	find_library(
		RAMFS_LIBRARY
		ramfs
	)

	# add the libramfs library and headers to the final target
	target_include_directories(
		${target}
		PUBLIC
		${RAMFS_INCLUDE_DIR}
	)

	target_link_libraries(
		${target}
		${RAMFS_LIBRARY}
		RAMFS
	)
endmacro()
