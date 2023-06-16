init_directory(extron_core)

# Define the Extron UI QT project
init_project(extron_core "Extron/Core")

# Sources
setup_project_source(extron_core "data"
  src:
    extron_data.cpp
    extron_data_description.cpp
    extron_data_history.cpp
  include:
    extron_data.h
    extron_data_description.h
    extron_data_history.h
)

# Target
configure_project_static_lib(extron_core)
configure_cxx_target(extron_core)
