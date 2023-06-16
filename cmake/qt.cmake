message(STATUS "Setup Qt6")

#
# Packages
#

find_package(Qt6 REQUIRED COMPONENTS Widgets)

#
# Source
#

macro(setup_project_source_qt_ui project_ref project_source_group)
  message(STATUS "Wrap Qt UI for ${project_ref}/${project_source_group}")
  qt6_wrap_ui(out_var ${ARGN})
  setup_project_source(${project_ref} ${project_source_group} ${ARGN})
  
  set_project_source_list(${project_ref})
  setup_project_source_external_generated(${project_ref} ${project_source_group} ${out_var})
endmacro()

#
# Deploying
#

get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

function(configure_qt_target project_ref)
  message(STATUS "Configure Qt Target ${project_ref}")

  set_target_properties(${project_ref} PROPERTIES AUTOMOC ON)
  set_target_properties(${project_ref} PROPERTIES AUTORRC ON)
  set_target_properties(${project_ref} PROPERTIES AUTOUIC ON)
  target_link_libraries(${project_ref} PRIVATE Qt6::Widgets)
  
  if(MSVC)
    message(STATUS " - Additional warnings are disabled for ${project_ref} to allow Qt compilation")
    target_compile_options(${project_ref} 
        PRIVATE
            "/wd4365;" # signed/unsigned mismatch
            "/wd4371;" # layout of class may have changed
            "/wd4464;" # relative include path contains '..'
            "/wd4702;" # unreachable code
            "/wd5027;" # move assignment operator was implicitly defined as deleted
    )

    message(STATUS " - Add deployment for windows")
    add_custom_command(TARGET ${project_ref} POST_BUILD
        COMMAND "${_qt_bin_dir}/windeployqt.exe"         
                --verbose 0
                \"$<TARGET_FILE:${project_ref}>\"
        COMMENT "Deploying Qt libraries using windeployqt for ${project_ref}"
    )
  endif()

endfunction()
