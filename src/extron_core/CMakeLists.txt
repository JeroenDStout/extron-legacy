init_directory(extron_core)

# Define the Extron UI QT project
init_project(extron_core "Extron/Core")

# Sources
setup_project_source(extron_core "data"
  src:
    data.cpp
    data_description.cpp
    data_history.cpp
  include:
    data.h
    data_description.h
    data_history.h
)

# Target
configure_project_static_lib(extron_core)
configure_project_tinyxml2(extron_core)
configure_cxx_target(extron_core)