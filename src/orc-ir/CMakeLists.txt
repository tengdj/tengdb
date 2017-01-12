
add_library(
		orc-ir
		genfunc-ir.cpp
		util-ir.cpp
		)
		#LLVMCore LLVMSupport LLVMMCJIT LLVMPasses LLVMX86CodeGen LLVMX86AsmParser
target_link_libraries(orc-ir common codegen LLVM pthread)

#add_executable(llvm llvm-codegen.cpp llvm-codegen-test.cpp config.cpp)
#target_link_libraries(llvm ${llvm_libs})