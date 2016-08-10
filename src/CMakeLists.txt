
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs all)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ${LLVM_CXX_FLAGS}")

add_executable(main 
		Query1.cpp 
		OrcFile.cpp 
		Reader.cpp 
		orc_proto.pb.cc 
		config.cpp 
		llvm-codegen.cpp 
		RLE2.cpp
		RLE2-ir.cpp
		util-ir.cpp
		ColumnReader-ir.cpp
		MemoryPool.cpp
		)
		#LLVMCore LLVMSupport LLVMMCJIT LLVMPasses LLVMX86CodeGen LLVMX86AsmParser
target_link_libraries(main libprotobuf.so LLVMCore LLVMSupport LLVMMCJIT LLVMPasses LLVMX86CodeGen LLVMX86AsmParser)

#add_executable(llvm llvm-codegen.cpp llvm-codegen-test.cpp config.cpp)
#target_link_libraries(llvm ${llvm_libs})