cmake_minimum_required(VERSION 3.2)
project(ramfs C)

set(RAMFS_VERSION "0.6")

add_library(ramfs STATIC
	source/ramfs.c
	source/devoptab.c
	source/node.c
	source/tar.c)

install(TARGETS ramfs DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/share" DESTINATION "${CMAKE_INSTALL_PREFIX}")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include" DESTINATION "${CMAKE_INSTALL_PREFIX}")
install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
configure(source/ramfs.pc.in "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/ramfs.pc")
