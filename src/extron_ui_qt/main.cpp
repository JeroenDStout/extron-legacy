#include "extron_ui_qt/window.h"
#include "version/git_version.h"

#include <QtWidgets/QApplication>

#include <iostream>


int main(int argc, char *argv[])
{
    std::cout
      << std::endl
      << "          * * * * * * * * * * * * " << std::endl
      << "         * * * Extron Legacy * * * " << std::endl
      << "          * * * * * * * * * * * * " << std::endl
      << std::endl
      << gaos::version::get_git_essential_version() << std::endl
      << gaos::version::get_compile_stamp() << std::endl
      << std::endl
      << gaos::version::get_git_history() << std::endl
      << std::endl;

    QApplication a(argc, argv);
    extron::ui_qt::main_window w;
    w.setWindowTitle(QString("Extron ") + gaos::version::get_git_essential_version());
    w.show();
    return a.exec();
}
