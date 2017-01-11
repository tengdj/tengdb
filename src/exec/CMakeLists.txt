
add_library(exec
		ScanNode.cpp 
		ORCScanner.cpp
		)
target_link_libraries(exec common orc orc-ir codegen)
