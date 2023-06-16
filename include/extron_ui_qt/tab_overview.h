#pragma once

#include "extron_core/data.h"
#include "extron_ui_qt/ui_tab_overview.h"

#include <QWidget>
#include <QtCharts>

#include <ctime>
#include <map>


class extron_data;
class QVBoxLayout;


namespace extron::ui_qt {


class tab_overview : public QWidget
{
    Q_OBJECT

  public:
    tab_overview(QWidget *parent, core::extron_data*);
    ~tab_overview();
    
    void perform_setup_chart();

    void perform_build_overview_list();
    void perform_build_overview_week_list(QVBoxLayout*);
    void perform_build_overview_special_list(QVBoxLayout*);
    void perform_build_exercise_list();
    void perform_build_time_range_list();
    void perform_build_special_inspiration_list();
    void perform_build_chart();

  signals:
    void signal_create_new_workout(std::string);
    void signal_show_workout(core::data_history::workout_time);
    void signal_commence_update_balance();

  public slots:
    void event_must_rebuild_exercise_list();
    void event_show_exercise_list();
    void event_create_new_workout_workout();
    void event_create_new_workout_incidental();
    void event_create_new_workout_journey();
    void event_show_workouts(std::time_t start, std::time_t end);
    void event_show_special_inspiration();
    void event_show_exercise(core::data_history::workout_time);
    void event_commence_update_balance();

  protected:
    std::time_t get_start_of_week(std::time_t const&) const;
    std::time_t get_end_of_week(std::time_t const&) const;
    QColor get_week_colour(std::time_t const&) const;
    QColor get_week_colour_light(std::time_t const&) const;
    QColor get_week_colour_light2(std::time_t const&) const;

    bool                in_exercise_list_mode;
    core::extron_data   *extron_data;
    std::time_t         show_range_start, show_range_end;

    QChart              *performance_chart;
    QDateTimeAxis       *performance_chart_time_axis;
    QValueAxis          *performance_chart_value_axis;
    std::vector<QColor> week_colours;

  private:
    Ui::tab_overview ui;
};


}