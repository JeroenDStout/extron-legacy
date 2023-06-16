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
    window.cpp
  include:
    window.h
)
setup_project_source_qt_ui(extron_ui_qt "ui"
  window.ui
  version.ui
)

# Target
configure_project_executable(extron_ui_qt)
configure_project_console(extron_ui_qt)
configure_cxx_target(extron_ui_qt)
configure_qt_target(extron_ui_qt)
	
add_dependencies(extron_ui_qt extron_core)
add_dependencies(extron_ui_qt version)
