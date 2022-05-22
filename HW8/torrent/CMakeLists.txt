project(torrent C CXX)

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
	add_compile_options(-Wno-implicit-int-float-conversion)
endif()

add_executable(torrent "torrent.cpp")
target_link_libraries(torrent PRIVATE pthread torrent-rasterbar)