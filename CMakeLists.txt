cmake_minimum_required(VERSION 3.10)
project(AutoRenameApp VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# We are building a Windows DLL
add_library(AutoRename SHARED
    src/dllmain.cpp
    src/AutoRenameExt.cpp
    src/ClassFactory.cpp
    src/ContextExtractor.cpp
    AutoRename.def
)

target_link_libraries(AutoRename 
    PRIVATE 
    shlwapi 
    propsys 
    ole32 
    shell32 
    user32
)

# Export definition for COM
set_target_properties(AutoRename PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
)

add_executable(TestApp src/TestApp.cpp src/ContextExtractor.cpp)
target_link_libraries(TestApp Shlwapi Propsys)


add_executable(IFilterTest src/IFilterTest.cpp)
target_link_libraries(IFilterTest Ole32 Ntquery)

