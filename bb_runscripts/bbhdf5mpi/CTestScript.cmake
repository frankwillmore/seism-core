cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)
########################################################
# This dashboard is maintained by The HDF Group
# For any comments please contact cdashhelp@hdfgroup.org
#
########################################################
# -----------------------------------------------------------
# -- Get environment
# -----------------------------------------------------------
if(NOT SITE_OS_NAME)
  ## machine name not provided - attempt to discover with uname
  ## -- set hostname
  ## --------------------------
  find_program(HOSTNAME_CMD NAMES hostname)
  exec_program(${HOSTNAME_CMD} ARGS OUTPUT_VARIABLE HOSTNAME)
  set(CTEST_SITE  "${HOSTNAME}${CTEST_SITE_EXT}")
  find_program(UNAME NAMES uname)
  macro(getuname name flag)
    exec_program("${UNAME}" ARGS "${flag}" OUTPUT_VARIABLE "${name}")
  endmacro(getuname)

  getuname(osname -s)
  getuname(osrel  -r)
  getuname(cpu    -m)
  message(STATUS "Dashboard script uname output: ${osname}-${osrel}-${cpu}\n")

  set(CTEST_BUILD_NAME  "${osname}-${osrel}")
else(NOT SITE_OS_NAME)
  ## machine name provided
  ## --------------------------
  if(CMAKE_HOST_UNIX)
    set(CTEST_BUILD_NAME "${SITE_OS_NAME}-${SITE_OS_VERSION}-${SITE_OS_BITS}-${SITE_COMPILER_NAME}-${SITE_COMPILER_VERSION}")
  else()
    set(CTEST_BUILD_NAME "${SITE_OS_NAME}-${SITE_OS_VERSION}-${SITE_COMPILER_NAME}")
  endif()
endif(NOT SITE_OS_NAME)
if(SITE_BUILDNAME_SUFFIX)
  set(CTEST_BUILD_NAME  "${SITE_BUILDNAME_SUFFIX}-${CTEST_BUILD_NAME}")
endif()
if(USE_AUTOTOOLS)
  set(CTEST_BUILD_NAME  "AT-${CTEST_BUILD_NAME}")
endif()
if(USE_ANTTOOLS)
  set(CTEST_BUILD_NAME  "ANT-${CTEST_BUILD_NAME}")
endif()
set(BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DSITE:STRING=${CTEST_SITE} -DBUILDNAME:STRING=${CTEST_BUILD_NAME}")

if(NOT USE_ANTTOOLS)
  #-----------------------------------------------------------------------------
  # MAC machines need special option
  #-----------------------------------------------------------------------------
  if(APPLE)
    # Compiler choice
    execute_process(COMMAND xcrun --find cc OUTPUT_VARIABLE XCODE_CC OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND xcrun --find c++ OUTPUT_VARIABLE XCODE_CXX OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(ENV{CC} "${XCODE_CC}")
    set(ENV{CXX} "${XCODE_CXX}")

    set(BUILD_OPTIONS "${BUILD_OPTIONS} -DCTEST_USE_LAUNCHERS:BOOL=ON -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=OFF")
  endif()
endif()

#-----------------------------------------------------------------------------
set(NEED_REPOSITORY_CHECKOUT 0)
set(CTEST_CMAKE_COMMAND "\"${CMAKE_COMMAND}\"")
if(CTEST_USE_TAR_SOURCE)
  ## Uncompress source if tar file provided
  ## --------------------------
  if(WIN32)
    message(STATUS "extracting... [${CMAKE_EXECUTABLE_NAME} x ${CTEST_USE_TAR_SOURCE}.zip]")
    execute_process(COMMAND ${CMAKE_EXECUTABLE_NAME} -E tar -xvf ${CTEST_DASHBOARD_ROOT}\\${CTEST_USE_TAR_SOURCE}.zip RESULT_VARIABLE rv)
  else()
    message(STATUS "extracting... [${CMAKE_EXECUTABLE_NAME} -E tar -xvf ${CTEST_USE_TAR_SOURCE}.tar]")
    execute_process(COMMAND ${CMAKE_EXECUTABLE_NAME} -E tar -xvf ${CTEST_DASHBOARD_ROOT}/${CTEST_USE_TAR_SOURCE}.tar RESULT_VARIABLE rv)
  endif()

  if(NOT rv EQUAL 0)
    message(STATUS "extracting... [error-(${rv}) clean up]")
    file(REMOVE_RECURSE "${CTEST_SOURCE_DIRECTORY}")
    message(FATAL_ERROR "error: extract of ${CTEST_USE_TAR_SOURCE} failed")
  endif()

  file(RENAME ${CTEST_DASHBOARD_ROOT}/${CTEST_USE_TAR_SOURCE} ${CTEST_SOURCE_DIRECTORY})
  set(LOCAL_SKIP_UPDATE "TRUE")
else(CTEST_USE_TAR_SOURCE)
  if(LOCAL_UPDATE)
    if(CTEST_USE_GIT_SOURCE)
      find_program(CTEST_GIT_COMMAND NAMES git git.cmd)
      set(CTEST_GIT_UPDATE_OPTIONS)

      if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}")
        set(NEED_REPOSITORY_CHECKOUT 1)
      endif()

      if(${NEED_REPOSITORY_CHECKOUT})
        if(REPOSITORY_BRANCH)
          set(CTEST_GIT_options "clone \"${REPOSITORY_URL}\" --branch  \"${REPOSITORY_BRANCH}\" --single-branch \"${CTEST_SOURCE_DIRECTORY}\" --recurse-submodules")
        else()
          set(CTEST_GIT_options "clone \"${REPOSITORY_URL}\" \"${CTEST_SOURCE_DIRECTORY}\" --recurse-submodules")
        endif()
        set(CTEST_CHECKOUT_COMMAND "${CTEST_GIT_COMMAND} ${CTEST_GIT_options}")
      endif()
      set(CTEST_UPDATE_COMMAND "${CTEST_GIT_COMMAND}")
    else(CTEST_USE_GIT_SOURCE)
      ## --------------------------
      ## use subversion to get source
      #-----------------------------------------------------------------------------
      ## cygwin does not handle the find_package() call
      ## --------------------------
      set(CTEST_UPDATE_COMMAND "SVNCommand")
      if(NOT SITE_CYGWIN})
        find_package (Subversion)
        set(CTEST_SVN_COMMAND "${Subversion_SVN_EXECUTABLE}")
        set(CTEST_UPDATE_COMMAND "${Subversion_SVN_EXECUTABLE}")
      else()
        set(CTEST_SVN_COMMAND "/usr/bin/svn")
        set(CTEST_UPDATE_COMMAND "/usr/bin/svn")
      endif()

      if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}")
        set(NEED_REPOSITORY_CHECKOUT 1)
      endif()

      if(NOT CTEST_REPO_VERSION)
        set(CTEST_REPO_VERSION "HEAD")
      endif()
      if(${NEED_REPOSITORY_CHECKOUT})
        set(CTEST_CHECKOUT_COMMAND
            "\"${CTEST_SVN_COMMAND}\" co ${REPOSITORY_URL} \"${CTEST_SOURCE_DIRECTORY}\" -r ${CTEST_REPO_VERSION}")
      else()
        if(CTEST_REPO_VERSION)
          set(CTEST_SVN_UPDATE_OPTIONS "-r ${CTEST_REPO_VERSION}")
        endif()
      endif()
    endif(CTEST_USE_GIT_SOURCE)
  endif(LOCAL_UPDATE)
endif(CTEST_USE_TAR_SOURCE)

#-----------------------------------------------------------------------------
## Clear the build directory
## --------------------------
set(CTEST_START_WITH_EMPTY_BINARY_DIRECTORY TRUE)
if(NOT EXISTS "${CTEST_BINARY_DIRECTORY}")
  file(MAKE_DIRECTORY "${CTEST_BINARY_DIRECTORY}")
else()
  ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})
endif()

if(NOT USE_ANTTOOLS)
  # Use multiple CPU cores to build
  include(ProcessorCount)
  ProcessorCount(N)
  if(NOT N EQUAL 0)
    if(NOT WIN32)
      set(CTEST_BUILD_FLAGS -j${N})
    endif()
    set(ctest_test_args ${ctest_test_args} PARALLEL_LEVEL ${N})
  endif()
endif()

if(USE_ANTTOOLS)
     set(BUILD_OPTIONS "${BUILD_OPTIONS} -DCTEST_USE_LAUNCHERS:BOOL=ON")
  #-----------------------------------------------------------------------------
  if(NOT WIN32)
    find_program(MAKE NAMES ant PATHS ${ANTPATH} PATH_SUFFIXES "bin")
  else()
    set(MAKE ant.bat)
  endif()

  # Send the main script as a note.
    list(APPEND CTEST_NOTES_FILES
      "${CTEST_SCRIPT_DIRECTORY}/${CTEST_SCRIPT_NAME}"
      "${CMAKE_CURRENT_LIST_FILE}"
    )
else()
  #-----------------------------------------------------------------------------
  # Send the main script as a note.
  if(USE_AUTOTOOLS)
    ## autotools builds need to use make and does not use the cacheinit.cmake file
    ## -- make command
    ## -----------------
    find_program(MAKE NAMES make)

    list(APPEND CTEST_NOTES_FILES
      "${CTEST_SCRIPT_DIRECTORY}/${CTEST_SCRIPT_NAME}"
      "${CMAKE_CURRENT_LIST_FILE}"
    )
  else()
    list(APPEND CTEST_NOTES_FILES
        "${CTEST_SCRIPT_DIRECTORY}/${CTEST_SCRIPT_NAME}"
        "${CMAKE_CURRENT_LIST_FILE}"
        "${CTEST_SOURCE_DIRECTORY}/config/cmake/cacheinit.cmake"
    )
  endif()
endif()

#-----------------------------------------------------------------------------
# Check for required variables.
# --------------------------
if(USE_ANTTOOLS)
  foreach(req
      CTEST_SITE
      CTEST_BUILD_NAME
    )
    if(NOT DEFINED ${req})
      message(FATAL_ERROR "The containing script must set ${req}")
    endif()
  endforeach(req)
else()
  foreach(req
      CTEST_CMAKE_GENERATOR
      CTEST_SITE
      CTEST_BUILD_NAME
    )
    if(NOT DEFINED ${req})
      message(FATAL_ERROR "The containing script must set ${req}")
    endif()
  endforeach(req)
endif()

if(USE_ANTTOOLS)
  #-----------------------------------------------------------------------------
  ## -- Configure Command
  set(CTEST_CONFIGURE_COMMAND            "${MAKE} -f ${CTEST_SOURCE_DIRECTORY} clean")

  ## -- Build Command
  set(CTEST_BUILD_COMMAND                "${MAKE} -f ${CTEST_SOURCE_DIRECTORY} compile -verbose")
  file(WRITE ${CTEST_BINARY_DIRECTORY}/CTestTestfile.cmake "#Generated testfile\n")
  foreach(testtarget ${BUILD_OPTIONS})
    file(APPEND ${CTEST_BINARY_DIRECTORY}/CTestTestfile.cmake "ADD_TEST(junit \"${MAKE}\" \"-f\" \"${CTEST_SOURCE_DIRECTORY}\" ${testtarget})\n")
  endforeach()
else()
  #-----------------------------------------------------------------------------
  # Initialize the CTEST commands
  #------------------------------
  if(USE_AUTOTOOLS)
    set(CTEST_CONFIGURE_COMMAND  "${CTEST_SOURCE_DIRECTORY}/configure ${BUILD_OPTIONS}")
    set(CTEST_BUILD_COMMAND      "${MAKE} ${CTEST_BUILD_FLAGS}")
    ## -- CTest Config
    #configure_file($ENV{HOME}/CTestConfiguration/CTestConfig.cmake    ${CTEST_SOURCE_DIRECTORY}/CTestConfig.cmake)
    configure_file(${CTEST_SOURCE_DIRECTORY}/config/cmake/CTestCustom.cmake ${CTEST_BINARY_DIRECTORY}/CTestCustom.cmake)
    ## -- CTest Testfile
  #  configure_file(${CTEST_SCRIPT_DIRECTORY}/CTestTestfile.cmake ${CTEST_BINARY_DIRECTORY}/CTestTestfile.cmake)
    file(WRITE ${CTEST_BINARY_DIRECTORY}/CTestTestfile.cmake "ADD_TEST(makecheck \"${MAKE}\" \"${CTEST_BUILD_FLAGS}\" \"-i\" \"check\")
    set_tests_properties(makecheck \"PROPERTIES\" \"TIMEOUT\" \"1800\")")
  else(USE_AUTOTOOLS)
    if(LOCAL_MEMCHECK_TEST)
      find_program(CTEST_MEMORYCHECK_COMMAND NAMES valgrind)
      set (CTEST_CONFIGURE_COMMAND
          "${CTEST_CMAKE_COMMAND} -C \"${CTEST_SOURCE_DIRECTORY}/config/cmake/mccacheinit.cmake\" -DCMAKE_BUILD_TYPE:STRING=${CTEST_CONFIGURATION_TYPE} ${BUILD_OPTIONS} \"-G${CTEST_CMAKE_GENERATOR}\" \"${CTEST_SOURCE_DIRECTORY}\""
      )
    else()
      if(LOCAL_COVERAGE_TEST)
        find_program(CTEST_COVERAGE_COMMAND NAMES gcov)
      endif()
      set (CTEST_CONFIGURE_COMMAND
          "${CTEST_CMAKE_COMMAND} -C \"${CTEST_SOURCE_DIRECTORY}/config/cmake/cacheinit.cmake\" -DCMAKE_BUILD_TYPE:STRING=${CTEST_CONFIGURATION_TYPE} ${BUILD_OPTIONS} \"-G${CTEST_CMAKE_GENERATOR}\" \"${CTEST_SOURCE_DIRECTORY}\""
      )
    endif()
  endif(USE_AUTOTOOLS)
endif()
#-----------------------------------------------------------------------------
## -- set output to english
set($ENV{LC_MESSAGES}  "en_EN")

# Print summary information.
foreach(v
    CTEST_SITE
    CTEST_BUILD_NAME
    CTEST_SOURCE_DIRECTORY
    CTEST_BINARY_DIRECTORY
    CTEST_CMAKE_GENERATOR
    CTEST_CONFIGURATION_TYPE
    CTEST_GIT_COMMAND
    CTEST_CHECKOUT_COMMAND
    CTEST_CONFIGURE_COMMAND
    CTEST_SCRIPT_DIRECTORY
    CTEST_USE_LAUNCHERS
  )
  set(vars "${vars}  ${v}=[${${v}}]\n")
endforeach(v)
message(STATUS "Dashboard script configuration:\n${vars}\n")

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
  ## NORMAL process
  ## -- LOCAL_UPDATE updates the source folder from svn
  ## -- LOCAL_SUBMIT reports to CDash server
  ## -- LOCAL_SKIP_TEST skips the test process (only builds)
  ## -- LOCAL_MEMCHECK_TEST executes the Valgrind testing
  ## -- LOCAL_COVERAGE_TEST executes code coverage process
  ## --------------------------
  ctest_start (${MODEL} TRACK ${MODEL})
  if(LOCAL_UPDATE)
    ctest_update (SOURCE "${CTEST_SOURCE_DIRECTORY}")
  endif()
  if(USE_ANTTOOLS)
    ctest_configure (BUILD "${CTEST_BINARY_DIRECTORY}" RETURN_VALUE res)
    if(LOCAL_SUBMIT)
      ctest_submit (PARTS Update Configure)
    endif()
  else()
    configure_file(${CTEST_SOURCE_DIRECTORY}/config/cmake/CTestCustom.cmake ${CTEST_BINARY_DIRECTORY}/CTestCustom.cmake)
    ctest_read_custom_files ("${CTEST_BINARY_DIRECTORY}")
    ctest_configure (BUILD "${CTEST_BINARY_DIRECTORY}" RETURN_VALUE res)
    if(LOCAL_SUBMIT)
      ctest_submit (PARTS Update Configure Notes)
    endif()
  endif()
  if(${res} LESS 0 OR ${res} GREATER 0)
    file(APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed Configure: ${res}\n")
  endif()

  if(NOT LOCAL_SKIP_BUILD)
    ctest_build (BUILD "${CTEST_BINARY_DIRECTORY}" APPEND RETURN_VALUE res NUMBER_ERRORS errval)
    if(LOCAL_SUBMIT)
      ctest_submit (PARTS Build)
    endif()
    if(${res} LESS 0 OR ${res} GREATER 0 OR ${errval} GREATER 0)
      file(APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed ${errval} Build: ${res}\n")
    endif()
  endif()

  if(NOT LOCAL_SKIP_TEST)
    if(NOT LOCAL_MEMCHECK_TEST)
      ctest_test (BUILD "${CTEST_BINARY_DIRECTORY}" APPEND ${ctest_test_args} RETURN_VALUE res)
      if(LOCAL_SUBMIT)
        ctest_submit (PARTS Test)
      endif()
      if(${res} LESS 0 OR ${res} GREATER 0)
        file(APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed Tests: ${res}\n")
      endif()
    else()
      ctest_memcheck (BUILD "${CTEST_BINARY_DIRECTORY}" APPEND ${ctest_test_args})
      if(LOCAL_SUBMIT)
        ctest_submit (PARTS MemCheck)
      endif()
    endif()
    if(LOCAL_COVERAGE_TEST)
      ctest_coverage (BUILD "${CTEST_BINARY_DIRECTORY}" APPEND)
      if(LOCAL_SUBMIT)
        ctest_submit (PARTS Coverage)
      endif()
    endif()
  endif(NOT LOCAL_SKIP_TEST)

  if(NOT LOCAL_MEMCHECK_TEST AND NOT LOCAL_NO_PACKAGE AND NOT LOCAL_SKIP_BUILD)
    ##-----------------------------------------------
    ## Package the product
    ##-----------------------------------------------
    if(USE_ANTTOOLS)
      if(WIN32)
        execute_process(COMMAND ${MAKE} -f ${CTEST_SOURCE_DIRECTORY} createWindowsInstaller
            WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY}
            RESULT_VARIABLE apackResult
            OUTPUT_VARIABLE apackLog
            ERROR_VARIABLE apackLog.err
        )
      else(WIN32)
        execute_process(COMMAND ${MAKE} -f ${CTEST_SOURCE_DIRECTORY} packageUnix
            WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY}
            RESULT_VARIABLE apackResult
            OUTPUT_VARIABLE apackLog
            ERROR_VARIABLE apackLog.err
        )
      endif(WIN32)
      file(WRITE ${CTEST_BINARY_DIRECTORY}/apack.log "${apackLog.err}" "${apackLog}")
      if(apackResult GREATER 0)
        file(APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed packaging: ${apackResult}\n")
      endif()
    else()
      if(USE_SOURCE_PACK)
        execute_process(COMMAND cpack --config CPackSourceConfig.cmake -V
            WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY}
            RESULT_VARIABLE cpackResult
            OUTPUT_VARIABLE cpackLog
            ERROR_VARIABLE cpackLog.err
        )
      else()
        execute_process(COMMAND cpack -C ${CTEST_CONFIGURATION_TYPE} -V
            WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY}
            RESULT_VARIABLE cpackResult
            OUTPUT_VARIABLE cpackLog
            ERROR_VARIABLE cpackLog.err
        )
      endif()
      file(WRITE ${CTEST_BINARY_DIRECTORY}/cpack.log "${cpackLog.err}" "${cpackLog}")
      if(cpackResult GREATER 0)
        file(APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed packaging: ${cpackResult}:${cpackLog.err} \n")
      endif()
    endif()

    if(LOCAL_SIGN_PACKAGE)
     if(WIN32)
        get_filename_component(WINSDK_DIR "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows;CurrentInstallFolder]" REALPATH CACHE)
        get_filename_component(WIN8KIT_DIR "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot]" REALPATH CACHE)
        get_filename_component(WINKIT_DIR "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot81]" REALPATH CACHE)
        if("${SITE_OS_BITS}" MATCHES "32")
            find_program(SIGN_TOOL NAMES signtool.exe PATHS ${WINKIT_DIR}/bin/x86 ${WINSDK_DIR}/bin ${WIN8KIT_DIR}/bin/x86)
        else()
            find_program(SIGN_TOOL NAMES signtool.exe PATHS ${WINKIT_DIR}/bin/x64 ${WINSDK_DIR}/bin ${WIN8KIT_DIR}/bin/x64)
        endif()
        if(EXISTS "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}.exe")
          set(SIGN_FILE "${BINFILEBASE}.exe")
          execute_process(COMMAND ${SIGN_TOOL} sign /v /d "${SIGN_DESC}" /f ${CTEST_SCRIPT_DIRECTORY}/HDF_Certificate.p12 /p "5k insomnia" /t http://timestamp.verisign.com/scripts/timstamp.dll ${SIGN_FILE}
              WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY}
              RESULT_VARIABLE signResult
              OUTPUT_VARIABLE signLog
              ERROR_VARIABLE signLog.err
          )
          file(APPEND ${CTEST_BINARY_DIRECTORY}/cpack.log "${signLog.err}" "${signLog}")
          if(signResult GREATER 0)
            file(APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed signing: ${signResult}\n")
          endif()
          message (STATUS "Signing result: ${signResult}\n")
        endif()
        if(EXISTS "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}.msi")
          set(SIGN_FILE "${BINFILEBASE}.msi")
          execute_process(COMMAND ${SIGN_TOOL} sign /v /d "${SIGN_DESC}" /f ${CTEST_SCRIPT_DIRECTORY}/HDF_Certificate.p12 /p "5k insomnia" /t http://timestamp.verisign.com/scripts/timstamp.dll ${SIGN_FILE}
              WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY}
              RESULT_VARIABLE signResult
              OUTPUT_VARIABLE signLog
              ERROR_VARIABLE signLog.err
          )
          file(APPEND ${CTEST_BINARY_DIRECTORY}/cpack.log "${signLog.err}" "${signLog}")
          if(signResult GREATER 0)
            file(APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed signing: ${signResult}\n")
          endif()
          message (STATUS "Signing result: ${signResult}\n")
        endif()
      endif()
    endif(LOCAL_SIGN_PACKAGE)

  endif(NOT LOCAL_MEMCHECK_TEST AND NOT LOCAL_NO_PACKAGE AND NOT LOCAL_SKIP_BUILD)
#-----------------------------------------------------------------------------