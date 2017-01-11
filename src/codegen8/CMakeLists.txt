
add_library(codegen 
		llvm-codegen.cpp
		)
target_link_libraries(codegen common LLVM pthread)
