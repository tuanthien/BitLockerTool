include_guard()

function(get_project_warnings result)
  set(MSVC_WARNINGS
      /W4 # Baseline reasonable warnings
      /w14242 # 'identifier': conversion from 'type1' to 'type1', possible loss of data
      /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
      /w14263 # 'function': member function does not override any base class virtual member function
      /w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not
              # be destructed correctly
      /w14287 # 'operator': unsigned/negative constant mismatch
      /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside
              # the for-loop scope
      /w14296 # 'operator': expression is always 'boolean_value'
      /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
      /w14545 # expression before comma evaluates to a function which is missing an argument list
      /w14546 # function call before comma missing argument list
      /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
      /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
      /w14555 # expression has no effect; expected expression with side- effect
      /w14619 # pragma warning: there is no warning number 'number'
      /w14640 # Enable warning on thread un-safe static member initialization
      /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
      /w14905 # wide string literal cast to 'LPSTR'
      /w14906 # string literal cast to 'LPWSTR'
      /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied

      /wd5030 # attribute is not recognized, prevent msvc from report on other compilers attribute
      /wd4068 # unknown pragma, prevent msvc from report on other compilers pragma
      /wd4297 # function assumed not to throw an exception but does, need to throw for EASSERT_EX in constexpr function
  )

  set(GCC_CLANG_WARNINGS
      -Wall
      -Wextra # reasonable and standard
      -Wshadow # warn the user if a variable declaration shadows one from a parent context
      #-Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps
                         # catch hard to track down memory errors
                         # boost::system::error_category has this warning suppressed but didn't work, so disable temporary
      -Wold-style-cast # warn for c-style casts
      -Wnarrowing # warning for narrow conversion
      -Wcast-align # warn for potential performance problem casts
      -Wunused # warn on anything being unused
      -Woverloaded-virtual # warn if you overload (not override) a virtual function
      -Wpedantic # warn if non-standard C++ is used
      -Wconversion # warn on type conversions that may lose data
      -Wsign-conversion # warn on sign conversions
      -Wnull-dereference # warn if a null dereference is detected
      -Wdouble-promotion # warn if float is implicit promoted to double
      #-Wpadded # warn on inefficiently padded member variable struct
      -Wformat=2 # warn on security issues around functions that format output (ie printf)
      -Wno-c++20-compat
      -Wno-c++20-extensions # @TODO why? we already set C++latest (maybe we should set C++2b?)
      -Wno-unknown-warning-option # disable this because it annoying if we support many compiler
  )
  
  set (CLANG_WARNINGS
    ${GCC_CLANG_WARNINGS}
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-global-constructors # @TODO gtest's TEST macro trigger this
    -Wno-newline-eof
    -Wno-documentation-unknown-command
  )

  if(WARNINGS_AS_ERRORS)
    set(CLANG_WARNINGS ${CLANG_WARNINGS} -Werror)
    set(MSVC_WARNINGS ${MSVC_WARNINGS} /WX)
  endif()

  set(GCC_WARNINGS
      ${GCC_CLANG_WARNINGS}
      -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
      -Wduplicated-cond # warn if if / else chain has duplicated conditions
      -Wduplicated-branches # warn if if / else branches have duplicated code
      -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
      -Wno-useless-cast # warn if you perform a cast to the same type, quite annoying in templated code
      -Wno-unknown-pragmas # warn when detect unknown pragma directives
      -Wno-missing-field-initializers # warn when missing any field in initializers, quite
                                      # annoying in a designated initializer list
  )

  if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(${result} ${MSVC_WARNINGS} PARENT_SCOPE)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(${result} ${CLANG_WARNINGS} PARENT_SCOPE)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(${result} ${GCC_WARNINGS} PARENT_SCOPE)
  else()
    message(AUTHOR_WARNING "No compiler warnings set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
  endif()

endfunction()

macro(setup_project_warnings projectName targetName)
  option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" FALSE)
  get_project_warnings(_projectWarnings)
  target_compile_options("${targetName}"
    INTERFACE 
      ${_projectWarnings}
      $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>)
  message (DEBUG "${projectName}: Compiler warnings: ${_projectWarnings}")
endmacro()  
