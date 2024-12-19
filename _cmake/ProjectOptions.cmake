include_guard()

macro(setup_project_settings projectName)
  # Set a default build type if none was specified
  if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
    set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui, ccmake
    set_property(
      CACHE CMAKE_BUILD_TYPE
      PROPERTY STRINGS
      "Debug"
      "Release"
      "MinSizeRel"
      "RelWithDebInfo")
  endif()

  # Generate compile_commands.json to make it easier to work with clang based tools
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  option(ENABLE_IPO "Enable Interprocedural Optimization, aka Link Time Optimization (LTO)" OFF)

  if (ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(
      RESULT
      result
      OUTPUT
      output)
    if (result)
      set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else ()
      message(SEND_ERROR "IPO is not supported: ${output}")
    endif()
  endif()
endmacro()

macro(setup_project_options projectName targetName)
  set_property(TARGET ${targetName} PROPERTY CXX_STANDARD 23)
  target_compile_features(${targetName} INTERFACE cxx_std_23)
  set_property(TARGET ${targetName} PROPERTY CMAKE_CXX_EXTENSIONS OFF)
  set_property(TARGET ${targetName} PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)

  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${targetName}
      INTERFACE
      /nologo
      /utf-8
      /permissive-
      /Zc:throwingNew-
      /Zc:preprocessor
      /Zc:externConstexpr
      )

    target_compile_definitions(${targetName} INTERFACE _WIN32_WINNT=${BLT_WINAPI_VERSION})
    
  endif()

  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${targetName} INTERFACE -fvisibility=hidden -fchar8_t)
  endif()

  if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")  
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set_property(TARGET ${targetName} PROPERTY CXX_STANDARD 23)
        target_compile_features(${targetName} INTERFACE cxx_std_23)
        set_property(TARGET ${targetName} PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)
        set(EAO_${targetName}_MSVC_CXX_LATEST 23)
        target_compile_options(${targetName}
            INTERFACE
            /utf-8
            /permissive-
            /EHsc
            /clang:-ftemplate-backtrace-limit=0
            )

        target_compile_definitions(${targetName} INTERFACE UNICODE _WIN32_WINNT=${BLT_WINAPI_VERSION})
    endif()
    
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
        target_compile_options(${targetName} INTERFACE -fvisibility=hidden $<IF:$<BOOL:ENABLE_FTRACE_TIME>,-ftime-trace,"">)
        set_property(TARGET ${targetName} PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)
    endif()
  endif()
endmacro()
