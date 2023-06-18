init_directory(extron_ui_qt)

# Define the Extron UI QT project
init_project(extron_ui_qt "Extron/Qt")

# Sources
setup_project_source(extron_ui_qt "core"
  src:
    main.cpp
)
setup_project_source(extron_ui_qt "window"
  src:
    clickable_label.cpp
    format.cpp
    tab_overview.cpp
    tab_workout.cpp
    window.cpp
  include:
    clickable_label.h
    format.h
    tab_overview.h
    tab_workout.h
    window.h
)
setup_project_source_qt_ui(extron_ui_qt "ui"
  window.ui
  version.ui
  tab_overview.ui
  tab_workout.ui
)
setup_project_source(extron_ui_qt "assets"
  src:
    ${rel_can_assets}/app_resource/resource.rc
)

# Target
configure_project_executable(extron_ui_qt)
configure_project_console(extron_ui_qt)
configure_cxx_target(extron_ui_qt)
configure_project_tinyxml2(extron_ui_qt)
configure_qt_target(extron_ui_qt)
	
add_dependencies(extron_ui_qt extron_core)
add_dependencies(extron_ui_qt version)
target_link_libraries(extron_ui_qt PRIVATE extron_core)
