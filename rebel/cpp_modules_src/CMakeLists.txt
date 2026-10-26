execute_process(
    COMMAND pyenv which python
    OUTPUT_VARIABLE PYENV_PYTHON_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Python interpreter: ${PYENV_PYTHON_PATH}")

execute_process(
    COMMAND ${PYENV_PYTHON_PATH} -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))"
    OUTPUT_VARIABLE PYTHON_EXTENSION_SUFFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Python shared library suffix: ${PYTHON_EXTENSION_SUFFIX}")

# pybind
add_subdirectory(pybind11)
# modules
add_subdirectory(graphlib)
add_subdirectory(directed_labeled_graph)
add_subdirectory(labeled_transition_system)
add_subdirectory(difference_constraint_program)
add_subdirectory(reset_chain_graph)
add_subdirectory(variable_flow_graph)
add_subdirectory(expression)
add_subdirectory(lts_constructor)
