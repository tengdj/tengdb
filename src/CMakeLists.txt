
set(LLVM_VERSION 3.9)
find_package(LLVM ${LLVM_VERSION} REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
#include_directories(/usr/local/llvm/include)

add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs all)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ${LLVM_CXX_FLAGS}")

#if(LLVM_VERSION EQUAL 3.9)
#include_directories(codegen9)
#ADD_SUBDIRECTORY(codegen9)
#MESSAGE("LLVM VERSION 3.9")
#else()
#include_directories(codegen8)
#ADD_SUBDIRECTORY(codegen8)
#MESSAGE("LLVM VERSION 3.8")
#endif()

ADD_SUBDIRECTORY(codegen)
ADD_SUBDIRECTORY(common)
ADD_SUBDIRECTORY(orc)
ADD_SUBDIRECTORY(orc-ir)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(exec)
ADD_SUBDIRECTORY(frontend)
ADD_SUBDIRECTORY(orcreader)
ADD_SUBDIRECTORY(llvm-vec)

