#include "extron_ui_qt/window.h"
#include "extron_ui_qt/tab_overview.h"
#include "extron_ui_qt/tab_workout.h"
#include "extron_core/data.h"
#include "version/git_version.h"

#include <qfilesystemwatcher>
#include <qfiledialog>
#include <qmessagebox>
#include <qsettings>
#include <qtabwidget>

#include <iostream>
#include <sstream>
#include <thread>


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
    
    this->current_description_file_path = desc_file_path;
    this->file_watcher->addPath(QString::fromStdString(this->current_description_file_path));

    std::string hist_file_path = desc_file_path;
    hist_file_path.erase(hist_file_path.end() - 7, hist_file_path.end());
    hist_file_path += ".xh.xml";

    tinyxml2::XMLDocument desc_doc;
    if (!load_xml_document(this->current_description_file_path, desc_doc))
      return false;

    this->extron_data->replace_description(desc_doc.RootElement());

    if (this->current_history_file_path != hist_file_path)
      perform_history_file_load(hist_file_path);

    auto const& state = this->extron_data->get_derived_exercise_state();
    std::cout << "main_window: Loaded " << state.extercises.size() << " exercises" << std::endl;

    return true;
}


bool main_window::perform_history_file_load(std::string const& hist_file_path)
{
    std::cout << "main_window: Loading history file " << hist_file_path << std::endl;
    
    this->current_history_file_path = hist_file_path;
    this->file_watcher->addPath(QString::fromStdString(this->current_history_file_path));
    
    tinyxml2::XMLDocument hist_doc;
    if (!load_xml_document(this->current_history_file_path, hist_doc))
      return false;

    this->extron_data->replace_history(hist_doc.RootElement());
	this->event_commence_update_balance();
    
    auto const& state = this->extron_data->get_derived_exercise_state();
    std::cout << "main_window: Loaded " << state.workouts.size() << " workouts" << std::endl;

    return true;
}


bool main_window::perform_history_file_save([[maybe_unused]] std::string const& hist_file_path)
{
    std::cout << "main_window: Saving history file " << hist_file_path << std::endl;

    tinyxml2::XMLDocument hist_doc;
    hist_doc.InsertEndChild(hist_doc.NewElement("history"));
    this->extron_data->save_history(hist_doc.RootElement());
    
    if (!save_xml_document(hist_file_path, hist_doc))
      return false;
    return true;
}


void main_window::perform_create_tab_overview()
{
    auto* tab = new tab_overview(this, this->extron_data.get());

    QObject::connect(
      this, &main_window::signal_must_rebuild_exercise_list,
      tab,  &tab_overview::event_must_rebuild_exercise_list
    );
    QObject::connect(
      tab,  &tab_overview::signal_create_new_workout,
      this, &main_window::event_create_new_workout
    );
    QObject::connect(
      tab,  &tab_overview::signal_show_workout,
      this, &main_window::event_show_workout
    );
    QObject::connect(
      tab,  &tab_overview::signal_commence_update_balance,
      this, &main_window::event_commence_update_balance
    );

    this->get_ui_main_tabs()->addTab(tab, QString::fromStdString(std::string(this->extron_data->get_nickname()) += " overview"));
}


void main_window::perform_create_tab_workout(core::data_history::workout_time time, std::string const& type)
{
    std::cout << "main_window: Creating workout tab" << time.time << ":" << time.uid << " (" << type << ")" << std::endl;
    
    QWidget *old_tab;
    for (int old_idx = 0; old_tab = this->get_ui_main_tabs()->widget(old_idx); old_idx++) {
        auto *tab = qobject_cast<ui_tab_workout*>(old_tab);
        if (tab == nullptr)
          continue;
        
        if (tab->get_old_workout_time() != time)
          continue;
          
        this->get_ui_main_tabs()->setCurrentIndex(old_idx);
        return;
    }

    auto *tab = new ui_tab_workout(this, this->extron_data.get(), time);
    
    if (type.size() > 0)
      tab->set_type(type);

    QObject::connect(
      this, &main_window::signal_must_rebuild_exercise_list,
      tab,  &ui_tab_workout::event_must_rebuild_exercise_list
    );
    QObject::connect(
      tab,  &ui_tab_workout::signal_close_tab,
      this, &main_window::event_close_tab
    );
    QObject::connect(
      tab,  &ui_tab_workout::signal_rename_tab,
      this, &main_window::event_rename_tab
    );
    QObject::connect(
      tab,  &ui_tab_workout::signal_history_requires_save,
      this, &main_window::event_history_requires_save
    );

    auto idx = this->get_ui_main_tabs()->addTab(tab, QString::fromStdString("Work-out"));
    this->get_ui_main_tabs()->setCurrentIndex(idx);

    tab->perform_setup();
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


void main_window::event_create_new_workout(std::string const &workout_type)
{
    std::cout << "main_window: Creating new workout" << std::endl;

    this->perform_create_tab_workout({0, 0}, workout_type);
}


void main_window::event_show_workout(core::data_history::workout_time time)
{
    std::cout << "main_window: Showing workout" << std::endl;

    this->perform_create_tab_workout(time, "");
}


void main_window::event_close_tab(QWidget *widget)
{
    auto idx = this->ui_window.main_tabs->indexOf(widget);
    if (idx <= 0)
      return;
    this->ui_window.main_tabs->removeTab(idx);
    this->ui_window.main_tabs->setCurrentIndex(0);
}


void main_window::event_rename_tab(QWidget *widget, std::string const &name)
{
    auto idx = this->ui_window.main_tabs->indexOf(widget);
    if (idx == -1)
      return;
    this->ui_window.main_tabs->setTabText(idx, name.c_str());
}


void main_window::event_history_requires_save()
{
    this->perform_history_file_save(this->current_history_file_path);
    this->event_must_rebuild_all();
}


void main_window::event_commence_update_balance()
{
	auto proc = this->extron_data->create_balance_proc_structure();

	for (int i = 0; i < 10000; i++)
	  proc->update_step();

	std::map<std::string, float> map;
	for (std::size_t i = 0; i < proc->exercise_names.size(); i++)
	  map[proc->exercise_names[i]] = proc->exercise_weight[i];
	this->extron_data->adjust_balance(map);

	emit signal_must_rebuild_exercise_list();
}


void main_window::closeEvent(QCloseEvent*)
{
    this->upon_close();
}


bool main_window::load_xml_document(std::string const &path, tinyxml2::XMLDocument &document) const
{
    int retries = 200;
    while (retries-- > 0) {
      bool success = (tinyxml2::XML_SUCCESS == document.LoadFile(path.c_str()));
      if (success)
        return true;
      std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(10.));
    }

    std::stringstream ss;
    ss << "The file " << path << " could not be read.";

    QMessageBox msgBox;
    msgBox.setText(QString::fromStdString(ss.str()));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();

    return false;
}


bool main_window::save_xml_document(std::string const &path, tinyxml2::XMLDocument &document) const
{
    int retries = 200;
    while (retries-- > 0) {
        bool success = (tinyxml2::XML_SUCCESS == document.SaveFile(path.c_str()));
        if (success)
          return true;
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(10.));
    }

    std::stringstream ss;
    ss << "The file " << path << " could not be saved.";

    QMessageBox msgBox;
    msgBox.setText(QString::fromStdString(ss.str()));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();

    return false;
}