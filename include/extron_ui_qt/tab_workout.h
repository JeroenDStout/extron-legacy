#pragma once

#include "extron_ui_qt/ui_tab_workout.h"
#include "extron_ui_qt/format.h"
#include "extron_core/data.h"

#include <QWidget>

class QLabel;


namespace extron::ui_qt {


class ui_tab_workout : public QWidget
{
    Q_OBJECT

  public:
    ui_tab_workout(QWidget *parent, core::extron_data*, core::data_history::workout_time);
    ~ui_tab_workout();
    
    void set_type(std::string const&);
    void perform_setup();

    core::data_history::workout_time get_old_workout_time() const;

  public slots:
    void event_must_rebuild_exercise_list();
    void event_must_rebuild_sample_list();
    void event_updated_type();
    void event_updated_time();
    void event_enable_exercise(std::string);
    void event_disable_exercise(std::string);
    void event_update_count_done();
    void event_update_count_target();
    void event_save();
    void event_revert();
    void event_close();
    void event_toggle_low_progress_only();

  signals:
    void signal_close_tab(QWidget*);
    void signal_rename_tab(QWidget*, std::string);
    void signal_history_requires_save();

  protected:
    struct related_widget_set {
      QLabel *performance_label;
    };
    std::map<std::string, related_widget_set> related_widgets;

    void perform_build_exercise_sample_list();
    void perform_build_exercise_chosen_list();
    void perform_build_exercise_info();
    void perform_build_performance();
    void perform_name_tab();

    format::workout_filter get_workout_filter();

    core::extron_data *extron_data;

    struct workout_setup_data : core::data_history::workout {
        workout_setup_data();
        core::data_history::workout_time time;
    };
    workout_setup_data original_data, new_data;

    QFont label_font, label_font_small;

    bool show_only_low_progress;

  private:
    Ui::ui_tab_workout ui;
};


}