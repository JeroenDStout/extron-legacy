message(STATUS "Setup TinyXML2")

include(FetchContent)
cmake_policy(SET CMP0135 NEW)

FetchContent_Declare(
  tinyxml2
  URL     https://github.com/leethomason/tinyxml2/archive/refs/tags/9.0.0.zip
)
FetchContent_MakeAvailable(tinyxml2)

set_target_properties(tinyxml2 PROPERTIES FOLDER "dependencies")

if(MSVC)
  message(STATUS " - Additional warnings are disabled for tinyxml2 to allow compilation")
  target_compile_options(tinyxml2
    PRIVATE
      "/wd4365;" # signed/unsigned mismatch
      "/wd4774;" # format string expected in argument 2 is not a string literal
  )
endif()

function(configure_project_tinyxml2 project_ref)
  if(MSVC)
    message(STATUS " - Additional warnings are disabled for ${project_ref} to allow compilation")
    target_compile_options(${project_ref} 
      PRIVATE
        "/wd4365;" # signed/unsigned mismatch
        "/wd4774;" # format string expected in argument 2 is not a string literal
    )
  endif()
  include_directories(${tinyxml2_SOURCE_DIR})
endfunction()