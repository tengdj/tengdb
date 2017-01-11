


add_executable(
		rle
		TestRLE.cpp
)

add_executable(
		rleir
		TestRLE-ir.cpp
)

add_executable(
		sql
		TestSql.cpp
)

target_link_libraries(sql frontend codegen orc-ir orc orcreader exec)
target_link_libraries(rleir common codegen orc orc-ir orcreader)
target_link_libraries(rle common orc orcreader)
