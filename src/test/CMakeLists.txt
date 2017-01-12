
add_executable(
		sql
		TestSql.cpp
)

target_link_libraries(sql frontend exec)
