
add_library(common
		config.cpp 
		MemoryPool.cpp
		orc_proto.pb.cc
		OrcFile.cpp
		Reader.cpp
		)
target_link_libraries(common libprotobuf.so pthread)
