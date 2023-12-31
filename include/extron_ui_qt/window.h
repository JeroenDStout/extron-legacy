#pragma once

#include "extron_core/data.h"
#include "extron_ui_qt/ui_window.h"
#include "extron_ui_qt/ui_version.h"
#include "extron_ui_qt/ui_tab_overview.h"

#include <qmainwindow>

class QFileSystemWatcher;
class QSettings;
class QTabWidget;


namespace extron::ui_qt {

    // Empty window
    class main_window : public QMainWindow
    {
        Q_OBJECT

      public:
        main_window();
        ~main_window();
        
      private:
        // situations
        void upon_open();
        void upon_close();
        void upon_diag_open();
        void upon_about();
        
        // functionality
        void perform_close_tabs();
        void perform_clear_loaded_data();
        bool perform_description_file_load(std::string const&);
        bool perform_history_file_load(std::string const&);
        bool perform_history_file_save(std::string const&);
        void perform_create_tab_overview();
        void perform_create_tab_workout(core::data_history::workout_time, std::string const& type);
        
        // Qt
        QSettings&  get_qsettings();
        QTabWidget* get_ui_main_tabs();
        void closeEvent(QCloseEvent *event) override;

        // events
        void event_must_rebuild_all();
        void event_create_new_workout(std::string const&);
        void event_show_workout(core::data_history::workout_time);
        void event_close_tab(QWidget*);
        void event_rename_tab(QWidget*, std::string const&);
        void event_history_requires_save();
	    void event_commence_update_balance();
        
      public slots:
        void uii_about();
        void uii_file_open();
        void qte_file_changed(QString const&);

      signals:
        void signal_must_rebuild_exercise_list();

      private:
        // util
        bool load_xml_document(std::string const &path, tinyxml2::XMLDocument&) const;
        bool save_xml_document(std::string const &path, tinyxml2::XMLDocument&) const;

      private:
        std::string                         current_description_file_path;
        std::string                         current_history_file_path;
        std::unique_ptr<core::extron_data>  extron_data;
        std::unique_ptr<QFileSystemWatcher> file_watcher;

        Ui::extron_window ui_window;
        Ui::version       ui_version;
    };

}