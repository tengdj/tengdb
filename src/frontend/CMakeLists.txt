
add_library(
		frontend
		sqlhelper.cpp
		SQLParser.cpp
		SQLParserResult.cpp
		SQLCodegen.cpp
		parser/bison_parser.cpp
		parser/flex_lexer.cpp
		sql/destruct.cpp
		sql/Expr.cpp
)

target_link_libraries(frontend common orc-ir orc codegen exec)
