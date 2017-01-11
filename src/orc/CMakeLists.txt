
add_library(orc
		RLE2.cpp
		)
target_link_libraries(orc libprotobuf.so pthread common)
