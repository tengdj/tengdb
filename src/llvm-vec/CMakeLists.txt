
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

add_executable(
		llvm
		Testllvm.cpp		
)

add_executable(
		presum
		presum.cpp		
)

add_executable(
		testvec 
		TestVec.cc
		shuffle128.cc
		shuffle256.cc
)

target_link_libraries(llvm codegen exec orc-ir)

