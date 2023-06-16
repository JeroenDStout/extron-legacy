#include "extron_ui_qt/window.h"
#include "extron_core/data.h"
#include "version/git_version.h"

#include <qfilesystemwatcher>
#include <qfiledialog>
#include <qsettings>
#include <qtabwidget>

#include <iostream>


using namespace extron::ui_qt;


main_window::main_window()
{
    ui_window.setupUi(this);
    
    this->upon_open();
}


main_window::~main_window()
{
}


void main_window::upon_open()
{
    auto &settings = this->get_qsettings();
    this->restoreGeometry(settings.value("geometry").toByteArray());
    std::string const main_file_path = settings.value("main_file_path").toString().toStdString();
    
    if (main_file_path.size() > 0) {
        this->perform_clear_loaded_data();
        if (this->perform_description_file_load(main_file_path)) {
            this->perform_create_tab_overview();
            this->event_must_rebuild_all();
        }
    }
}


void main_window::upon_close()
{
    auto &settings = this->get_qsettings();
    settings.setValue("geometry", saveGeometry());
    settings.setValue("main_file_path", QString::fromStdString(current_description_file_path));
}


void main_window::upon_diag_open()
{
    std::cout << "main_window: File open dialogue" << std::endl;

    QString const in_path_q = QFileDialog::getOpenFileName(this, tr("Open tron"), "", tr("Extron file (*.xt.xml)"));
    if (in_path_q.length() == 0)
      return;

    std::string const in_path = in_path_q.toStdString();
    if (in_path == current_description_file_path)
      return;

    this->perform_clear_loaded_data();
    if (!this->perform_description_file_load(in_path)) {
      this->perform_clear_loaded_data();
      return;
    }

    this->perform_create_tab_overview();
    this->event_must_rebuild_all();
}


void main_window::upon_about()
{
    std::cout << "main_window: Displaying main window about" << std::endl;

    QDialog *about = new QDialog();

    ui_version.setupUi(about);
    ui_version.version_main->setText(QString("Extron Legacy\n") + gaos::version::get_git_essential_version());
    ui_version.version_compile->setText(gaos::version::get_compile_stamp());
    ui_version.version_git->setText(gaos::version::get_git_history());

    about->setWindowModality(Qt::WindowModality::ApplicationModal);
    about->show();
}


void main_window::perform_close_tabs()
{
    while (this->get_ui_main_tabs()->count() > 0) {
	    auto *tab = this->get_ui_main_tabs()->widget(0);
	    this->get_ui_main_tabs()->removeTab(0);
	    delete tab;
	}
}


void main_window::perform_clear_loaded_data()
{
    this->perform_close_tabs();
      
    this->current_description_file_path.clear();
    this->current_history_file_path.clear();
    this->extron_data.reset(new extron::core::extron_data);

    this->file_watcher.reset(new QFileSystemWatcher());
    connect(file_watcher.get(), SIGNAL(fileChanged(const QString &)), this, SLOT(qte_file_changed(const QString &)));
}


bool main_window::perform_description_file_load(std::string const& desc_file_path)
{
    std::cout << "main_window: Loading description file " << desc_file_path << std::endl;

    // TBA

    return true;
}


bool main_window::perform_history_file_load(std::string const& hist_file_path)
{
    std::cout << "main_window: Loading history file " << hist_file_path << std::endl;

    // TBA

    return true;
}


void main_window::perform_create_tab_overview()
{
    std::cout << "main_window: Creating overview tab" << std::endl;

    // TBA
}


void main_window::perform_create_tab_workout(core::data_history::workout_time time, std::string const& type)
{
    std::cout << "main_window: Creating workout tab" << time.time << ":" << time.uid << " (" << type << ")" << std::endl;

    // TBA
}


QSettings& main_window::get_qsettings()
{
    static QSettings Extron_Settings("Stout", "Extron");
    return Extron_Settings;
}


QTabWidget* main_window::get_ui_main_tabs()
{
    auto widget = this->ui_window.core->topLevelWidget()->findChild<QWidget*>(QString("main_tabs"));

    if (!widget)
      return nullptr;

    auto tabs = qobject_cast<QTabWidget*>(widget);
    return tabs;
}


void main_window::uii_file_open()
{
    this->upon_diag_open();
}


void main_window::uii_about()
{
    this->upon_about();
}


void main_window::qte_file_changed(QString const& file)
{
    this->file_watcher->addPath(file);

    if (this->current_description_file_path == file.toStdString())
      this->perform_description_file_load(this->current_description_file_path);

    this->event_must_rebuild_all();
}


void main_window::event_must_rebuild_all()
{
    emit signal_must_rebuild_exercise_list();
}


void main_window::closeEvent(QCloseEvent*)
{
    this->upon_close();
}