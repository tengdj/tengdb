

#add_executable(query1 Query1.cpp)

add_executable(
		rle
		TestRLE.cpp
)

add_executable(
		rleir
		TestRLE-ir.cpp
)

add_executable(
		scanner
		TestScanNode.cpp
)

add_executable(
		sql
		TestSql.cpp
)

target_link_libraries(sql frontend codegen orc-ir orc exec)
#target_link_libraries(query1 common codegen orc orc-ir)
target_link_libraries(rleir common codegen orc orc-ir)
target_link_libraries(rle common orc)
target_link_libraries(scanner exec common codegen orc orc-ir)
