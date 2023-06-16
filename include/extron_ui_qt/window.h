#pragma once

#include "extron_core/data.h"
#include "extron_ui_qt/ui_window.h"
#include "extron_ui_qt/ui_version.h"

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
        
      public slots:
        void uii_about();
        void uii_file_open();
        void qte_file_changed(QString const&);

      signals:
        void signal_must_rebuild_exercise_list();

      private:
        std::string                         current_description_file_path;
        std::string                         current_history_file_path;
        std::unique_ptr<core::extron_data>  extron_data;
        std::unique_ptr<QFileSystemWatcher> file_watcher;

        Ui::extron_window ui_window;
        Ui::version       ui_version;
    };

}