
add_library(common
		config.cpp 
		MemoryPool.cpp
		)
add_executable(
		code
		coding.cc
)
#target_link_libraries(common pthread)
