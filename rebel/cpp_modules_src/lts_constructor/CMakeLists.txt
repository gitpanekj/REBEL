#set(LT_LLVM_CMAKE_CONFIG_DIR "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm/")
#list(APPEND CMAKE_PREFIX_PATH "${LT_LLVM_CMAKE_CONFIG_DIR}")

find_package(LLVM 22.0.0 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/cpp_modules_src/include)
message(STATUS "Using LLVM_INCLUDE_DIRS in: ${LLVM_INCLUDE_DIRS}")


add_library(lts STATIC lts_construction.cpp cfg_utils.cpp llvm_ir_processor.cpp lts_label_builder.cpp)
target_link_libraries(lts labeled_transition_system expression "LLVMWindowsManifest;LLVMXRay;LLVMLibDriver;LLVMDlltoolDriver;LLVMTextAPIBinaryReader;LLVMCoverage;LLVMLineEditor;LLVMX86TargetMCA;LLVMX86Disassembler;LLVMX86AsmParser;LLVMX86CodeGen;LLVMX86Desc;LLVMX86Info;LLVMOrcDebugging;LLVMOrcJIT;LLVMWindowsDriver;LLVMMCJIT;LLVMJITLink;LLVMInterpreter;LLVMExecutionEngine;LLVMRuntimeDyld;LLVMOrcTargetProcess;LLVMOrcShared;LLVMDWP;LLVMDebugInfoLogicalView;LLVMDebugInfoGSYM;LLVMOption;LLVMObjectYAML;LLVMObjCopy;LLVMMCA;LLVMMCDisassembler;LLVMLTO;LLVMPasses;LLVMHipStdPar;LLVMCFGuard;LLVMCoroutines;LLVMipo;LLVMVectorize;LLVMSandboxIR;LLVMLinker;LLVMInstrumentation;LLVMFrontendOpenMP;LLVMFrontendOffloading;LLVMFrontendOpenACC;LLVMFrontendHLSL;LLVMFrontendDriver;LLVMExtensions;LLVMDWARFLinkerParallel;LLVMDWARFLinkerClassic;LLVMDWARFLinker;LLVMGlobalISel;LLVMMIRParser;LLVMAsmPrinter;LLVMSelectionDAG;LLVMCodeGen;LLVMTarget;LLVMObjCARCOpts;LLVMCodeGenTypes;LLVMCGData;LLVMIRPrinter;LLVMInterfaceStub;LLVMFileCheck;LLVMFuzzMutate;LLVMScalarOpts;LLVMInstCombine;LLVMAggressiveInstCombine;LLVMTransformUtils;LLVMBitWriter;LLVMAnalysis;LLVMProfileData;LLVMSymbolize;LLVMDebugInfoBTF;LLVMDebugInfoPDB;LLVMDebugInfoMSF;LLVMDebugInfoDWARF;LLVMObject;LLVMTextAPI;LLVMMCParser;LLVMIRReader;LLVMAsmParser;LLVMMC;LLVMDebugInfoCodeView;LLVMBitReader;LLVMFuzzerCLI;LLVMCore;LLVMRemarks;LLVMBitstreamReader;LLVMBinaryFormat;LLVMTargetParser;LLVMTableGen;LLVMSupport;LLVMDemangle")


set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/cpp_modules/lts_constructor)
file(WRITE "${CMAKE_SOURCE_DIR}/cpp_modules/lts_constructor/__init__.py"
     "from lts_constructor.lts_constructor import *\n")

# Python bindings
pybind11_add_module(lts_constructor py_lts_constructor.cpp)
target_link_libraries(lts_constructor PRIVATE lts directed_labeled_graph)
target_include_directories(lts_constructor PRIVATE ${CMAKE_SOURCE_DIR}/cpp_modules_src/include)
set_target_properties(lts_constructor PROPERTIES
    OUTPUT_NAME "lts_constructor"
    PREFIX ""
    SUFFIX "${PYTHON_EXTENSION_SUFFIX}")
