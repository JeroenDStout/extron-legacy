#pragma once

#include "extron_ui_qt/ui_window.h"
#include "extron_ui_qt/ui_version.h"

#include <QtWidgets/QMainWindow>


namespace extron::ui_qt {

    // Empty window
    class main_window : public QMainWindow
    {
        Q_OBJECT

      public:
        main_window();
        
      public slots:
        void uii_about();

      private:
        Ui::extron_window ui_window;
        Ui::version       ui_version;
    };

}