add_library(graphlib graph_builder.cpp graph_view.cpp)
target_include_directories(graphlib PRIVATE ${CMAKE_SOURCE_DIR}/cpp_modules_src/include)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/cpp_modules/graphlib)

# file(WRITE "${CMAKE_SOURCE_DIR}/cpp_modules/expression/__init__.py"
#      "from expression.expression import *\n")
# pybind11_add_module(PyExpression py_expression.cpp)
# target_link_libraries(PyExpression PRIVATE expression)
# target_include_directories(PyExpression PRIVATE ${CMAKE_SOURCE_DIR}/cpp_modules_src/include)
# set_target_properties(PyExpression PROPERTIES
#     OUTPUT_NAME "expression"
#     PREFIX ""
#     SUFFIX "${PYTHON_EXTENSION_SUFFIX}")