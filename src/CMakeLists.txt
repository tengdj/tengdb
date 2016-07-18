
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs all)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ${LLVM_CXX_FLAGS}")

add_executable(main 
		myorc.cpp 
		OrcFile.cc 
		Reader.cpp 
		orc_proto.pb.cc 
		specific-ir.cpp 
		config.cpp 
		llvm-codegen.cc 
		processColumn.cpp
		RLE2.cpp
		RLE2-ir.cpp
		util-ir.cpp
		)
target_link_libraries(main libprotobuf.a ${llvm_libs})

#add_executable(llvm llvm-codegen.cc llvm-codegen-test.cc config.cpp)
#target_link_libraries(llvm ${llvm_libs})

add_executable(testutil 
		uti-ir.cpp 
		util-ir.cpp
		RLE2-ir.cpp
		llvm-codegen.cc 
		config.cpp 
		Reader.cpp
		orc_proto.pb.cc 
		OrcFile.cc )
target_link_libraries(testutil ${llvm_libs} libprotobuf.a)