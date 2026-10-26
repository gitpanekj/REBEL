add_library(graphlib serialization.cpp)
target_include_directories(graphlib PUBLIC ${CMAKE_SOURCE_DIR}/cpp_modules_src/include)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/cpp_modules/graphlib)

# TODO: implement binding to python if required
# file(WRITE "${CMAKE_SOURCE_DIR}/cpp_modules/expression/__init__.py"
#      "from graphlib.graphlib import *\n")
# pybind11_add_module(PyGraphlib py_graphlib.cpp)
# target_link_libraries(PyGraphlib PRIVATE graphlib)
# target_include_directories(PyGraphlib PRIVATE ${CMAKE_SOURCE_DIR}/cpp_modules_src/include)
# set_target_properties(PyGraphlib PROPERTIES
#     OUTPUT_NAME "graphlib"
#     PREFIX ""
#     SUFFIX "${PYTHON_EXTENSION_SUFFIX}")