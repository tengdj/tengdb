
add_library(codegen 
		llvm-codegen.cpp
		vectorization-ir.cpp
		)
target_link_libraries(codegen common LLVM pthread)
