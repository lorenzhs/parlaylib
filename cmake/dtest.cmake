# -------------------------------------------------------------------
#                   DTest: Dan's unit testing framework
#
# - Requires the GoogleTest framework (test cases must use it)
# - Every test case is instrumented with ASAN, UBSAN, TSAN, and MSAN.
# - Every test case can be ran with memcheck
# -------------------------------------------------------------------

# CTest options must go before include(CTest)
set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --error-exitcode=1 --show-leak-kinds=all --suppressions=${CMAKE_SOURCE_DIR}/.valgrind-suppress")

include(CTest)
include(CheckCXXCompilerFlag)
include(GoogleTest)

# Option to enable the sanitizer-instrumented tests
option(BUILD_ASAN "Build tests instrumented with ASAN" OFF)
option(BUILD_UBSAN "Build tests instrumented with UBSAN" OFF)
option(BUILD_TSAN "Build tests instrumented with TSAN" OFF)
option(BUILD_MSAN "Build tests instrumented with MSAN" OFF)
option(ENABLE_MEMCHECK "Enable memcheck target for tests" OFF)

# Option to use libc++
option(USE_LIBCXX "Use the libc++ standard library implementation" OFF)

# If MSAN is enabled, we need a path to an MSAN-instrumented libc++ binary
set(LIBCXX_MSAN_PATH "" CACHE STRING "Path to MSAN-instrumented libc++ library")

# Sanitizer blacklist file
set(SANITIZER_BLACKLIST "-fsanitize-blacklist=${CMAKE_SOURCE_DIR}/.sanitizer-blacklist")

# Sanitizer commands
set(ASAN_FLAGS
  -fsanitize=address
  -fsanitize-address-use-after-scope
  -fno-sanitize-recover=address
)
set(UBSAN_FLAGS
  -fsanitize=undefined,implicit-conversion,float-divide-by-zero
  -fno-sanitize-recover=undefined,implicit-conversion,float-divide-by-zero
)
set(MSAN_FLAGS
  -fsanitize=memory
  -fsanitize-memory-track-origins
  -fsanitize-memory-use-after-dtor
  -fno-sanitize-recover=memory
)
set(TSAN_FLAGS
  -fsanitize=thread
  -fno-sanitize-recover=thread
)

# Check whether a given set of flags is supported
function(flags_supported FLAGS VAR)
  set(CMAKE_REQUIRED_FLAGS "${FLAGS}")
  check_cxx_compiler_flag("${FLAGS}" ${VAR})
  unset(CMAKE_REQUIRED_FLAGS)
endfunction(flags_supported)

# Check that sanitizers are supported by the compiler
flags_supported("${SANITIZER_BLACKLIST}" BLACKLIST_SUPPORT)
if (BUILD_ASAN OR BUILD_UBSAN OR BUILD_TSAN OR BUILD_MSAN)
  if(NOT BLACKLIST_SUPPORT)
    message(STATUS "Warning: Sanitizer blacklists are not supported by the compiler. False positives may be encountered.")
    set(SANITIZER_BLACKLIST "")
  endif()
else()
  set(SANITIZER_BLACKLIST "")
endif()

# Check for ASAN support
if(BUILD_ASAN)
  flags_supported("${ASAN_FLAGS}" ASAN_SUPPORT)
  if(ASAN_SUPPORT)
    message(STATUS "AddressSanitizer:             Enabled")
  else()
    message(FATAL_ERROR "AddressSanitizer is not supported by the compiler")
  endif()
else()
  message(STATUS "AddressSanitizer:             Disabled (Enable with -DBUILD_ASAN=On)")
endif()

# Check for UBSAN support
if(BUILD_UBSAN)
  flags_supported("${UBSAN_FLAGS}" UBSAN_SUPPORT)
  if(UBSAN_SUPPORT)
    message(STATUS "UndefinedBehaviourSanitizer:  Enabled")
  else()
    message(STATUS "UndefinedBehaviourSanitizer:  Disabled (Not fully supported by the compiler)")
    set(BUILD_UBSAN Off)
  endif()
else()
  message(STATUS "UndefinedBehaviourSanitizer:  Disabled (Enable with -DBUILD_UBSAN=On)")
endif()

# Check for TSAN support
if(BUILD_TSAN)
  flags_supported("${TSAN_FLAGS}" TSAN_SUPPORT)
  if(TSAN_SUPPORT)
    message(STATUS "ThreadSanitizer:              Enabled")
  else()
    message(FATAL_ERROR "ThreadSanitizer is not supported by the compiler")
  endif()
else()
  message(STATUS "ThreadSanitizer:              Disabled (Enable with -DBUILD_TSAN=On)")
endif()

# Check for MSAN support
if(BUILD_MSAN)
  if(NOT LIBCXX_MSAN_PATH)
    message(FATAL_ERROR "You must provide an MSAN-instrumented binary of libc++ in order to use MemorySanitizer. Set the flag -DLIBCXX_MSAN_PATH=<path/to/instrumented/libcxx/lib>")
  elseif(NOT EXISTS "${LIBCXX_MSAN_PATH}/libc++.a")
    message(FATAL_ERROR "Could not find libc++.a in directory pointed to by LIBCXX_MSAN_PATH (${LIBCXX_MSAN_PATH})")
  endif()
  flags_supported("${MSAN_FLAGS}" MSAN_SUPPORT)
  if(MSAN_SUPPORT)
    message(STATUS "MemorySanitizer:              Enabled")
  else()
    message(FATAL_ERROR "MemorySanitizer is not supported by the compiler")
  endif()
else()
  message(STATUS "MemorySanitizer:              Disabled (Enable with -DBUILD_MSAN=On)")
endif()

# Check for memcheck installation
if(ENABLE_MEMCHECK)
  find_program(CTEST_MEMORYCHECK_COMMAND NAMES "valgrind")
  if(CTEST_MEMORYCHECK_COMMAND)
    message(STATUS "memcheck:                     Found (${CTEST_MEMORYCHECK_COMMAND})")
  else()
    message(FATAL_ERROR "Could not find valgrind executable for memcheck target")
  endif()
else()
  message(STATUS "memcheck:                     Disabled (Enable with -DENABLE_MEMCHECK)")
endif()

# Link the given target (and its dependents) against libc++
function(use_libcxx TARGET)
  target_compile_options(${TARGET} PUBLIC -stdlib=libc++)
  target_link_options(${TARGET} PUBLIC -stdlib=libc++)
  target_link_libraries(${TARGET} PUBLIC c++abi)
endfunction()

# Add MemorySanitizer instrumentation to the target
function(add_msan_instrumentation TARGET)
  target_compile_options(${TARGET} PRIVATE
    ${MSAN_FLAGS}
    ${SANITIZER_BLACKLIST}
    -stdlib=libc++
  )
  target_link_options(${TARGET} PRIVATE
    ${MSAN_FLAGS}
    ${SANITIZER_BLACKLIST}
    "-stdlib=libc++"
    "-L${LIBCXX_MSAN_PATH}"
    "-Wl,-rpath,${LIBCXX_MSAN_PATH}"
  )
  target_link_libraries(${TARGET} PUBLIC c++abi)
endfunction()

# Add clone targets of GoogleTest libraries with MSAN instrumentation
if(BUILD_MSAN)
  # Add clone targets
  set(googletest_INCLUDE_DIR
    "${googletest_SOURCE_DIR}/googletest/include"
    "${googletest_SOURCE_DIR}/googletest/"
  )
  cxx_library(gtest-msan "${cxx_strict}" ${googletest_SOURCE_DIR}/googletest/src/gtest-all.cc)
  cxx_library(gtest_main-msan "${cxx_strict}" ${googletest_SOURCE_DIR}/googletest/src/gtest_main.cc)
  target_include_directories(gtest-msan SYSTEM PUBLIC ${googletest_INCLUDE_DIR})
  target_include_directories(gtest_main-msan SYSTEM PUBLIC ${googletest_INCLUDE_DIR})
  target_link_libraries(gtest_main-msan PUBLIC gtest-msan)

  # Add MSAN instrumentation
  add_msan_instrumentation(gtest-msan)
  add_msan_instrumentation(gtest_main-msan)
endif()

# Compile with libc++
if (USE_LIBCXX)
  use_libcxx(gtest)
  use_libcxx(gtest_main)
endif()
  
# Add a test target for the given source file linked with the given
# sanitizer flags (or none for no sanitizer instrumentation)
# This creates a target with the name ${TESTNAME}-${ABBRV}
function(add_dtest TESTNAME SOURCENAMES SANITIZER_FLAGS ABBRV TESTLIB ADDITIONAL_LIBS ADDITIONAL_FLAGS)
  add_executable(${TESTNAME}-${ABBRV} ${SOURCENAMES})
  target_link_libraries(${TESTNAME}-${ABBRV} PRIVATE ${TESTLIB})
  target_link_libraries(${TESTNAME}-${ABBRV} PRIVATE ${ADDITIONAL_LIBS})
  target_compile_options(${TESTNAME}-${ABBRV} PRIVATE
    -Wall -Wextra -Wfatal-errors
    -g -Og -fno-omit-frame-pointer
    ${SANITIZER_FLAGS} ${SANITIZER_BLACKLIST}
    ${ADDITIONAL_FLAGS}
  )
  target_link_options(${TESTNAME}-${ABBRV} PRIVATE
    -fno-omit-frame-pointer
    ${SANITIZER_FLAGS} ${SANITIZER_BLACKLIST}
    ${ADDITIONAL_FLAGS}
  )
  if (USE_LIBCXX)
    use_libcxx(${TESTNAME}-${ABBRV})
  endif()
  gtest_discover_tests(${TESTNAME}-${ABBRV} TEST_SUFFIX "-${ABBRV}")
endfunction()

# Adds tests based on the given source file with sanitizer instrumentation
function(add_dtests)

  # Parse arguments
  set(options OPTIONAL "")
  set(oneValueArgs NAME)
  set(multiValueArgs FILES LIBS FLAGS)
  cmake_parse_arguments(CONFIGURE_ADD_DTESTS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  
  set(TESTNAME ${CONFIGURE_ADD_DTESTS_NAME})
  set(TESTFILES ${CONFIGURE_ADD_DTESTS_FILES})
  set(TESTLIBS ${CONFIGURE_ADD_DTESTS_LIBS})
  set(TESTFLAGS ${CONFIGURE_ADD_DTESTS_FLAGS})

  # Vanilla test -- no instrumentation
  add_dtest("${TESTNAME}" "${TESTFILES}" "" "nosan" gtest_main "${TESTLIBS}" "${TESTFLAGS}")

  # Test with ASAN (AddressSanitizer)
  if(BUILD_ASAN)
    add_dtest("${TESTNAME}" "${TESTFILES}" "${ASAN_FLAGS}" "asan" gtest_main "${TESTLIBS}" "${TESTFLAGS}")
  endif()

  # Test with UBSAN (UndefinedBehaviourSanitizer)
  if(BUILD_UBSAN)
    add_dtest("${TESTNAME}" "${TESTFILES}" "${UBSAN_FLAGS}" "ubsan" gtest_main "${TESTLIBS}" "${TESTFLAGS}")
  endif()

  # Test with TSAN (ThreadSanitizer)
  if(BUILD_TSAN)
    add_dtest("${TESTNAME}" "${TESTFILES}" "${TSAN_FLAGS}" "tsan" gtest_main "${TESTLIBS}" "${TESTFLAGS}")
  endif()

  # Test with MSAN (MemorySanitizer)
  if (BUILD_MSAN)
    add_dtest("${TESTNAME}" "${TESTFILES}" "${MSAN_FLAGS}" "msan" gtest_main-msan "${TESTLIBS}" "${TESTFLAGS}")
    add_msan_instrumentation(${TESTNAME}-msan)
  endif()
endfunction()

# Test targets for each sanitizer & memcheck
add_custom_target(check-all)

add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND} -R -nosan
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR} 
)
add_dependencies(check-all check)

if(BUILD_ASAN)
  add_custom_target(check-asan
    COMMAND ${CMAKE_CTEST_COMMAND} -R -asan
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  add_dependencies(check-all check-asan)
endif()

if(BUILD_UBSAN)
  add_custom_target(check-ubsan
    COMMAND ${CMAKE_CTEST_COMMAND} -R -ubsan
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  add_dependencies(check-all check-ubsan)
endif()

if(BUILD_TSAN)
  add_custom_target(check-tsan
    COMMAND ${CMAKE_CTEST_COMMAND} -R -tsan
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  add_dependencies(check-all check-tsan)
endif()

if(BUILD_MSAN)
  add_custom_target(check-msan
    COMMAND ${CMAKE_CTEST_COMMAND} -R -msan
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  add_dependencies(check-all check-msan)
endif()

if(ENABLE_MEMCHECK)
  add_custom_target(check-memcheck
    COMMAND ${CMAKE_CTEST_COMMAND} -R nosan -T memcheck
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  add_dependencies(check-all check-memcheck)
endif()
