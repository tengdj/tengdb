
add_library(orcreader
		orc_proto.pb.cc
		ORCReader.cpp
		)
target_link_libraries(orcreader libprotobuf.so pthread)
