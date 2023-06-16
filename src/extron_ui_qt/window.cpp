#include "extron_ui_qt/window.h"
#include "version/git_version.h"


#include <iostream>


using namespace extron::ui_qt;


main_window::main_window()
{
    ui_window.setupUi(this);
}


void main_window::uii_about()
{
    std::cout << "Displaying main window about" << std::endl;

    QDialog *about = new QDialog();

    ui_version.setupUi(about);
    ui_version.version_main->setText(QString("Extron Legacy\n") + gaos::version::get_git_essential_version());
    ui_version.version_compile->setText(gaos::version::get_compile_stamp());
    ui_version.version_git->setText(gaos::version::get_git_history());

    about->setWindowModality(Qt::WindowModality::ApplicationModal);
    about->show();
}