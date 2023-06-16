#include "extron_ui_qt/tab_overview.h"
#include "extron_ui_qt/clickable_label.h"
#include "extron_ui_qt/format.h"
#include "extron_core/data.h"

#include <qscrollarea>
#include <qboxlayout>
#include <qlabel>
#include <qpushbutton>
#include <QtCharts>

#include <array>
#include <iomanip>
#include <set>
#include <sstream>
#include <time.h>


using namespace extron::ui_qt;


tab_overview::tab_overview(QWidget *parent, core::extron_data *data):
  QWidget(parent),
  extron_data(data)
{
    ui.setupUi(this);

    this->in_exercise_list_mode = true;

    this->perform_setup_chart();
    
    this->week_colours = {
        // Agnès colour
      QColor(197, 195,  50), QColor(194, 146,   0), QColor(195,  84,   2), QColor(203,  68,  36), 
      QColor(190,  94, 122), QColor(195,  63, 113), QColor(150,  73, 141), QColor( 88,  53, 147), 
      QColor( 49,  98, 190), QColor( 30,  84, 190), QColor( 31, 137, 197), QColor( 38, 101, 154), 
      QColor(  0, 107,  90), QColor( 80, 181, 141), QColor( 49, 100,  67), QColor( 90, 127,  22), 
      QColor(186, 173, 129), QColor(157, 134,  92), QColor(165,  82,  28), QColor(146,  72,  63), 
      QColor(115,  67,  44), QColor( 51,  58,  76), QColor( 87,  93,  89), QColor(180, 189, 194)
    };
}


tab_overview::~tab_overview()
{
}


void tab_overview::perform_setup_chart()
{
    this->performance_chart = new QChart();
    this->performance_chart->legend()->hide();
    this->performance_chart->setTitle("Performance");

    this->performance_chart_time_axis = new QDateTimeAxis;
    this->performance_chart_time_axis->setTickCount(15);
    this->performance_chart_time_axis->setFormat("MMM yyyy");
    this->performance_chart->addAxis(this->performance_chart_time_axis, Qt::AlignBottom);

    this->performance_chart_value_axis = new QValueAxis;
    this->performance_chart_value_axis->setTickCount(5);
    this->performance_chart_value_axis->setLabelFormat("%i");
    this->performance_chart->addAxis(this->performance_chart_value_axis, Qt::AlignLeft);

    QChartView *chartView = new QChartView(this->performance_chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    this->ui.chart_frame->addWidget(chartView);
    
    QObject::connect(
      this->ui.chart_update, &QPushButton::released,
      this,                  &tab_overview::event_commence_update_balance
    );
}


void tab_overview::perform_build_overview_list()
{
    auto const& state = extron_data->get_derived_exercise_state();

    QWidget     *parent_widget_new      = this->ui.his_enum_create_new;
    QVBoxLayout *layout_new             = qobject_cast<QVBoxLayout*>(parent_widget_new->layout());

    QWidget     *parent_widget_existing = this->ui.his_enum_existing;
    QVBoxLayout *layout_existing        = qobject_cast<QVBoxLayout*>(parent_widget_existing->layout());

    QWidget     *parent_widget_special  = this->ui.his_enum_special;
    QVBoxLayout *layout_special         = qobject_cast<QVBoxLayout*>(parent_widget_special->layout());

    QLayoutItem *item;
    while (item = layout_new->takeAt(0)) {
        if (QWidget *w = item->widget())
          w->deleteLater();
        delete item;
    }
    while (item = layout_existing->takeAt(0)) {
        if (QWidget *w = item->widget())
          w->deleteLater();
        delete item;
    }
    while (item = layout_special->takeAt(0)) {
        if (QWidget *w = item->widget())
          w->deleteLater();
        delete item;
    }

    QFrame *frame_new_exercise = new QFrame();
    frame_new_exercise->setFrameStyle(QFrame::Panel | QFrame::Raised);

    QHBoxLayout *layout_new_exercise = new QHBoxLayout();

    QPushButton *button_see_exercises = new QPushButton();
    button_see_exercises->setText(QString::number(state.extercises.size()) + " exercises");
    layout_new_exercise->addWidget(button_see_exercises);
    QObject::connect(
      button_see_exercises, &QPushButton::released,
      this,                 &tab_overview::event_show_exercise_list
    );

    QPushButton *button_new_exercise = new QPushButton();
    button_new_exercise->setText("New Workout");
    layout_new_exercise->addWidget(button_new_exercise);
    QObject::connect(
      button_new_exercise, &QPushButton::released,
      this,                &tab_overview::event_create_new_workout_workout
    );

    QPushButton *button_new_journey = new QPushButton();
    button_new_journey->setText("New Journey");
    layout_new_exercise->addWidget(button_new_journey);
    QObject::connect(
      button_new_journey, &QPushButton::released,
      this,               &tab_overview::event_create_new_workout_journey
    );

    QPushButton *button_new_incidental = new QPushButton();
    button_new_incidental->setText("New Incidental");
    layout_new_exercise->addWidget(button_new_incidental);
    QObject::connect(
      button_new_incidental, &QPushButton::released,
      this,                  &tab_overview::event_create_new_workout_incidental
    );

    frame_new_exercise->setLayout(layout_new_exercise);
    
    layout_new->addWidget(frame_new_exercise);

    if (state.workouts.size() > 0) {
        perform_build_overview_week_list(layout_existing);
        perform_build_overview_special_list(layout_special);
    }

    layout_existing->addItem(new QSpacerItem(0,10, QSizePolicy::Expanding, QSizePolicy::Expanding));
    layout_special->addItem(new QSpacerItem(0,10, QSizePolicy::Expanding, QSizePolicy::Expanding));
}


void tab_overview::perform_build_overview_week_list(QVBoxLayout *layout)
{
    auto const& state = extron_data->get_derived_exercise_state();

    if (state.workouts.size() == 0)
      return;

    std::time_t end_of_week = state.workouts.rbegin()->first.time;
    
    end_of_week -= (
      (end_of_week - 4 * 24 * 3600) % (7 * 24 * 3600)
    );
    end_of_week += (7 * 24 * 3600);
    
    core::data_history::workout_time const earliest_time = state.workouts.begin()->first;

    while (end_of_week > earliest_time.time) {
        core::data_history::workout_time w_start_week = { end_of_week - (7 * 24 * 3600), 0 };
        core::data_history::workout_time w_end_week = { end_of_week, std::numeric_limits<unsigned int>::max() };
        
        auto start_itt = state.workouts.lower_bound(w_start_week);
        auto end_itt   = state.workouts.upper_bound(w_end_week);
        
        if (start_itt == state.workouts.end())
          start_itt    = state.workouts.begin();
        
        size_t count     = 0;
        float sum_effort = 0.f;
        
        while (start_itt != end_itt) {
            sum_effort += start_itt->second.total_effort;
            ++count; ++start_itt;
        }
        
        std::stringstream week_ss;
        std::tm time_tmp;
        gmtime_s(&time_tmp, &w_start_week.time);
        week_ss << std::put_time(&time_tmp, "Week %W");
        
        QFrame *frame_week = new QFrame();
        frame_week->setFrameStyle(QFrame::Panel | QFrame::Raised);
        
        QHBoxLayout *layout_week = new QHBoxLayout();
        
        QLabel *week_indication = new QLabel();
        week_indication->setText(QString::fromStdString(week_ss.str()));
        layout_week->addWidget(week_indication);
        
        std::stringstream ss;
        ss << std::setprecision(1) << std::fixed;
        ss << count << u8" workouts — ";
        ss << sum_effort << " effort";
        
        date_range_push_button *button_show_workouts = new date_range_push_button(w_start_week.time, w_end_week.time);
        button_show_workouts->setText(QString::fromStdString(ss.str()));
        layout_week->addWidget(button_show_workouts);
        QObject::connect(
          button_show_workouts, &date_range_push_button::date_released,
          this, &tab_overview::event_show_workouts
        );
        
        frame_week->setLayout(layout_week);
        layout->addWidget(frame_week);
        
        end_of_week -= (7 * 24 * 3600);
    }
}


void tab_overview::perform_build_overview_special_list(QVBoxLayout *layout)
{
    QPushButton *button_inspire = new QPushButton();
    button_inspire->setText(u8"✨ Inspiration ✨");
    button_inspire->setMinimumHeight(50);
    layout->addWidget(button_inspire);
    QObject::connect(
      button_inspire, &QPushButton::released,
      this, &tab_overview::event_show_special_inspiration
    );
}


void tab_overview::perform_build_exercise_list()
{
    if (!this->in_exercise_list_mode)
      return;

    using exe_data = core::derived_exercise_state::exercise_data;
    struct exe_data_ex : exe_data {
      std::string category_display_name, category_sort_name;
    };

    QWidget *parent_widget = this->ui.right_area_scroll;

    auto const &state = this->extron_data->get_derived_exercise_state();
    std::vector<exe_data_ex> exercises;
    for (auto const& exe : state.extercises) {
        exe_data_ex data_ex;
        *((exe_data*)&data_ex) = exe.second;
        
        auto const found_cat = state.categories.find(data_ex.category);
        if (found_cat != state.categories.end()) {
            data_ex.category_display_name = found_cat->second.display_name;
            data_ex.category_sort_name = found_cat->second.sort_name;
        }
        else {
            data_ex.category_display_name = "No Category";
        }
        
        exercises.push_back(data_ex);
    }

    std::sort(
      exercises.begin(), exercises.end(),
      [=](exe_data_ex const &lh, exe_data_ex const &rh) -> bool {
          if (lh.category != rh.category)
            return lh.category_sort_name < rh.category_sort_name;
          return lh.sort_name < rh.sort_name;
      }
    );
    
    QGridLayout *layout = new QGridLayout();

    std::string prev_category = "";
    int cur_row = 0;
    for (auto const& exercise : exercises) {
        if (exercise.category != prev_category) {
            QLabel *cat_label = new QLabel();
            cat_label->setText(QString::fromStdString(exercise.category_display_name));
            cat_label->setStyleSheet("font-size: 16px");
            cat_label->setContentsMargins(0, 10, 0, 0);
            layout->addWidget(cat_label, cur_row++, 0);
            prev_category = exercise.category;
        }
        
        QLabel *exe_label = new QLabel();
        QString text = u8"   ";
        text += QString::fromStdString(exercise.display_name);
        exe_label->setText(text);
        layout->addWidget(exe_label, cur_row, 0);
        
        std::string progress_emoji_output = format::format_progress_to_emoji(exercise.full_relative_advance);
        std::string weight_emoji_output   = format::format_balance_to_emoji(exercise.relative_balance);
        
        QLabel *emoji_label = new QLabel();
        emoji_label->setText(QString::fromStdString(progress_emoji_output));
        emoji_label->setAlignment(Qt::AlignCenter);
        layout->addWidget(emoji_label, cur_row, 2);
        
        QLabel *weight_emoji_label = new QLabel();
        weight_emoji_label->setText(QString::fromStdString(weight_emoji_output));
        weight_emoji_label->setAlignment(Qt::AlignCenter);
        layout->addWidget(weight_emoji_label, cur_row, 3);
        
        QLabel *median_label = new QLabel();
        median_label->setText(QString::fromStdString(format::format_median_range(exercise.weighed_median_range)));
        median_label->setAlignment(Qt::AlignCenter);
        layout->addWidget(median_label, cur_row, 4);
        
        ++cur_row;
    }

    layout->addItem(new QSpacerItem(0,10, QSizePolicy::Expanding, QSizePolicy::Expanding), cur_row, 5);

    delete parent_widget->layout();
    qDeleteAll(parent_widget->children());
    parent_widget->setLayout(layout);
}


void tab_overview::perform_build_time_range_list()
{
    if (this->in_exercise_list_mode)
      return;

    auto const &state = this->extron_data->get_derived_exercise_state();

    QWidget *parent_widget = this->ui.right_area_scroll;
    QVBoxLayout *layout = new QVBoxLayout();

    QLayoutItem *item;
    while (item = layout->takeAt(0)) {
      if (QWidget *w = item->widget())
        w->deleteLater();
      delete item;
    }

    core::data_history::workout_time range_start = { show_range_start, 0 };
    core::data_history::workout_time range_end   = { show_range_end, std::numeric_limits<unsigned int>::max() };
      
    auto start_itt = state.workouts.lower_bound(range_start);
    auto end_itt   = state.workouts.upper_bound(range_end);

    if (start_itt == state.workouts.end())
      start_itt    = state.workouts.begin();

    while (start_itt != end_itt) {
        --end_itt;
        
        std::tm time_tmp;
        localtime_s(&time_tmp, &end_itt->first.time);

        std::stringstream title_ss;
        title_ss << std::setprecision(1) << std::fixed;
        title_ss << std::put_time(&time_tmp, "%A %Y-%m-%d %H:%M");
        title_ss << u8" — " << end_itt->second.type << u8" — " << end_itt->second.total_exercises << u8" exercises — ";
        title_ss << end_itt->second.total_effort << " effort";
        
        QFrame *frame_workout = new QFrame();
        frame_workout->setFrameStyle(QFrame::Panel | QFrame::Raised);
        
        QVBoxLayout *layout_workout = new QVBoxLayout();
        
        clickable_label_time *title = new clickable_label_time("", end_itt->first);
        title->setText(QString::fromStdString(title_ss.str()));
        layout_workout->addWidget(title);
        layout_workout->setAlignment(Qt::AlignCenter);
        QObject::connect(
          title, &clickable_label_time::clicked,
          this,  &tab_overview::event_show_exercise
        );
        
        frame_workout->setLayout(layout_workout);
        
        layout->addWidget(frame_workout);
    }

    layout->addItem(new QSpacerItem(0,10, QSizePolicy::Expanding, QSizePolicy::Expanding));

    delete parent_widget->layout();
    qDeleteAll(parent_widget->children());
    parent_widget->setLayout(layout);
}


void tab_overview::perform_build_special_inspiration_list()
{
    if (this->in_exercise_list_mode)
      return;

    auto const &state = this->extron_data->get_derived_exercise_state();

    QWidget *parent_widget = this->ui.right_area_scroll;
    QVBoxLayout *layout = new QVBoxLayout();

    QLayoutItem *item;
    while (item = layout->takeAt(0)) {
      if (QWidget *w = item->widget())
        w->deleteLater();
      delete item;
    }

    std::vector<std::pair<float, core::data_history::workout_time>> sorted_workouts;
    sorted_workouts.reserve(sorted_workouts.size());

    for (auto const elem : state.workouts)
      sorted_workouts.push_back({ elem.second.total_effort, elem.first });
    std::sort(
      sorted_workouts.begin(),
      sorted_workouts.end(),
      [](auto lh, auto rh) {
        return lh.first > rh.first;
      }
    );

    std::default_random_engine generator;
    generator.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());

    std::vector<core::data_history::workout_time> selected_workouts;
    selected_workouts.reserve(10);
    for (std::size_t i = 0; i < 10; ++i) {
        std::uniform_int_distribution<int> dist(
          int(sorted_workouts.size() * (i+0)) / 10,
          int(sorted_workouts.size() * (i+1)) / 10 - 1
        );
        selected_workouts.push_back(sorted_workouts[dist(generator)].second);
    }

    core::data_history::workout_time range_start = { show_range_start, 0                                        };
    core::data_history::workout_time range_end   = { show_range_end,   std::numeric_limits<unsigned int>::max() };

    for (auto elem : selected_workouts) {
        auto const& workout = state.workouts.find(elem);
        
        std::tm time_tmp;
        localtime_s(&time_tmp, &workout->first.time);
        
        std::stringstream title_ss;
        title_ss << std::setprecision(1) << std::fixed;
        title_ss << std::put_time(&time_tmp, "%A %Y-%m-%d %H:%M");
        title_ss << u8" — " << workout->second.type << u8" — " << workout->second.total_exercises << u8" exercises — ";
        title_ss << workout->second.total_effort << " effort";
        
        QFrame *frame_workout = new QFrame();
        frame_workout->setFrameStyle(QFrame::Panel | QFrame::Raised);
        
        QVBoxLayout *layout_workout = new QVBoxLayout();
        
        clickable_label_time *title = new clickable_label_time("", workout->first);
        title->setText(QString::fromStdString(title_ss.str()));
        layout_workout->addWidget(title);
        layout_workout->setAlignment(Qt::AlignCenter);
        QObject::connect(
          title, &clickable_label_time::clicked,
          this,  &tab_overview::event_show_exercise
        );
        
        frame_workout->setLayout(layout_workout);
        
        layout->addWidget(frame_workout);
    }

    layout->addItem(new QSpacerItem(0,10, QSizePolicy::Expanding, QSizePolicy::Expanding));

    delete parent_widget->layout();
    qDeleteAll(parent_widget->children());
    parent_widget->setLayout(layout);
}


void tab_overview::perform_build_chart()
{
    auto const &state = this->extron_data->get_derived_exercise_state();

    this->performance_chart->removeAllSeries();
    if (this->extron_data->get_derived_exercise_state().chart_effort.size() == 0)
      return;

    std::time_t range_min = this->get_start_of_week(std::min(state.chart_effort.begin()->first,  state.workouts.begin()->first.time));
    std::time_t range_max = this->get_end_of_week(  std::max(state.chart_effort.rbegin()->first, state.workouts.rbegin()->first.time));

    // Area fill chart

    std::vector<std::pair<float, float>> weight_element;
    QLineSeries *area_series_mid_low = nullptr;
    QLineSeries *area_series_mid = nullptr;
    QLineSeries *area_series_mid_high = nullptr;
    QAreaSeries *area_series_wide = nullptr;
    QLineSeries *area_series_wide_lower = nullptr;
    QLineSeries *area_series_wide_upper = nullptr;
    bool first_week = true;
    static float sigma = 0.05f;
    std::time_t end_of_week = this->get_end_of_week(state.workouts.begin()->first.time);

    std::vector<QLineSeries*> area_series_mid_to_add;
    
    for (auto const &effort : state.chart_effort) {
        weight_element.clear();
        float sum_weight = 0.f;
        float sum_weight_mid = 0.f;

        for (auto const &sub_effort : state.chart_effort) {
            float abs_diff = std::abs(float(effort.first - sub_effort.first)) / (90.f * 24.f * 3600.f);

            if (abs_diff > 1.f)
              continue;

            float weight = 1.0f / (sigma * std::sqrt(2.0f * M_PI)) * std::exp(-(std::pow((float(abs_diff)) / sigma, 2.f) / 2.f));
            weight_element.push_back({sub_effort.second, weight});
            sum_weight += weight;
            sum_weight_mid += weight;// * weight;
        }

        if (sum_weight == 0.f)
          continue;

        std::sort(weight_element.begin(), weight_element.end());

        float weight_mid_low    = 0.33f * sum_weight_mid;
        float weight_mid        = 0.5f * sum_weight_mid;
        float weight_mid_high   = 0.66f * sum_weight_mid;
        float weight_wide_low   = 0.1f * sum_weight;
        float weight_wide_high  = 0.9f * sum_weight;

        float bound_mid_low = weight_element.front().first;
        float bound_mid = weight_element.back().first;
        float bound_mid_high = weight_element.back().first;
        float bound_wide_lower = weight_element.front().first;
        float bound_wide_higher = weight_element.back().first;

        sum_weight = 0.f;
        sum_weight_mid = 0.f;

        for (auto &elem : weight_element) {
          sum_weight += elem.second;
          sum_weight_mid += elem.second;// * elem.second;

          if (sum_weight_mid <= weight_mid_low)
            bound_mid_low = elem.first;
          if (sum_weight_mid <= weight_mid)
            bound_mid = elem.first;
          if (sum_weight_mid <= weight_mid_high)
            bound_mid_high = elem.first;
          if (sum_weight < weight_wide_low)
            bound_wide_lower = elem.first;
          if (sum_weight < weight_wide_high)
            bound_wide_higher = elem.first;
        }

        if (first_week || end_of_week < effort.first) {
            if (!first_week) {
                area_series_mid_low->append(   QDateTime::fromSecsSinceEpoch(effort.first).toMSecsSinceEpoch(), bound_mid_low);
                area_series_mid->append(       QDateTime::fromSecsSinceEpoch(effort.first).toMSecsSinceEpoch(), bound_mid);
                area_series_mid_high->append(  QDateTime::fromSecsSinceEpoch(effort.first).toMSecsSinceEpoch(), bound_mid_high);
                area_series_wide_lower->append(QDateTime::fromSecsSinceEpoch(effort.first).toMSecsSinceEpoch(), bound_wide_lower);
                area_series_wide_upper->append(QDateTime::fromSecsSinceEpoch(effort.first).toMSecsSinceEpoch(), bound_wide_higher);
                
                this->performance_chart->addSeries(area_series_wide);
                area_series_wide->attachAxis(this->performance_chart_time_axis);
                area_series_wide->attachAxis(this->performance_chart_value_axis);
            }
            first_week = false;

            QColor week_colour_light = this->get_week_colour(effort.first);
            week_colour_light.setAlphaF(.15f);

            area_series_mid_low = new QLineSeries();
            area_series_mid_low->setColor(week_colour_light);
            area_series_mid_to_add.push_back(area_series_mid_low);
            QPen pen = area_series_mid_low->pen();
            pen.setWidth(1);
            area_series_mid_low->setPen(pen);
            area_series_mid = new QLineSeries();
            area_series_mid->setColor(this->get_week_colour(effort.first));
            area_series_mid_to_add.push_back(area_series_mid);
            pen = area_series_mid->pen();
            pen.setWidth(2);
            area_series_mid->setPen(pen);
            area_series_mid_high = new QLineSeries();
            area_series_mid_high->setColor(week_colour_light);
            area_series_mid_to_add.push_back(area_series_mid_high);
            pen = area_series_mid_high->pen();
            pen.setWidth(1);
            area_series_mid_high->setPen(pen);
            area_series_wide = new QAreaSeries();
            area_series_wide_lower = new QLineSeries();
            area_series_wide_upper = new QLineSeries();
            area_series_wide->setLowerSeries(area_series_wide_lower);
            area_series_wide->setUpperSeries(area_series_wide_upper);
            area_series_wide->setPen(Qt::NoPen);
            auto area_colour = this->get_week_colour_light(effort.first);
            area_colour.setAlphaF(0.5f);
            area_series_wide->setColor(area_colour);

            end_of_week += 7 * 24 * 3600;
        }
        
        area_series_mid_low->append(   QDateTime::fromSecsSinceEpoch(effort.first).toMSecsSinceEpoch(), bound_mid_low);
        area_series_mid->append(       QDateTime::fromSecsSinceEpoch(effort.first).toMSecsSinceEpoch(), bound_mid);
        area_series_mid_high->append(  QDateTime::fromSecsSinceEpoch(effort.first).toMSecsSinceEpoch(), bound_mid_high);
        area_series_wide_lower->append(QDateTime::fromSecsSinceEpoch(effort.first).toMSecsSinceEpoch(), bound_wide_lower);
        area_series_wide_upper->append(QDateTime::fromSecsSinceEpoch(effort.first).toMSecsSinceEpoch(), bound_wide_higher);
    }

    if (!first_week) {
      this->performance_chart->addSeries(area_series_wide);
      area_series_wide->attachAxis(this->performance_chart_time_axis);
      area_series_wide->attachAxis(this->performance_chart_value_axis);
    }

    // Effort line series (light)

    QLineSeries *series = new QLineSeries();
    this->performance_chart->addSeries(series);
    series->attachAxis(this->performance_chart_time_axis);
    series->attachAxis(this->performance_chart_value_axis);
    series->setColor(this->get_week_colour_light(state.workouts.begin()->first.time));
    
    std::time_t start_of_week = this->get_start_of_week(state.workouts.begin()->first.time);
    end_of_week = this->get_end_of_week(state.workouts.begin()->first.time);
    float avg_effort = 0.;
    int avg_effort_count = 0;
    int week_idx = 0;

    for (auto const &effort : state.chart_effort) {
        if (effort.first < end_of_week) {
            avg_effort_count += 1;
            avg_effort += effort.second;
            continue;
        }
        
        avg_effort /= avg_effort_count;
        
        QPen pen = series->pen();
        pen.setWidth(1);
        series->setPen(pen);
        
        series->append(QDateTime::fromSecsSinceEpoch(start_of_week).toMSecsSinceEpoch(), avg_effort);
        series->append(QDateTime::fromSecsSinceEpoch(end_of_week).toMSecsSinceEpoch(),   avg_effort);
        
        avg_effort = effort.second;
        avg_effort_count = 1;
        
        series = new QLineSeries();
        this->performance_chart->addSeries(series);
        series->attachAxis(this->performance_chart_time_axis);
        series->attachAxis(this->performance_chart_value_axis);
        auto area_colour = this->get_week_colour(effort.first);
        area_colour.setAlphaF(0.5f);
        series->setColor(area_colour);
        series->setName(QString("Week soft ") + QString::number(week_idx++));
        
        start_of_week = end_of_week;
        end_of_week = this->get_end_of_week(effort.first);
    }

    avg_effort /= avg_effort_count;
    series->append(QDateTime::fromSecsSinceEpoch(start_of_week).toMSecsSinceEpoch(), avg_effort);
    series->append(QDateTime::fromSecsSinceEpoch(end_of_week).toMSecsSinceEpoch(),   avg_effort);

    // Help lines

    QLineSeries *help_line = new QLineSeries();
    help_line->append(QDateTime::fromSecsSinceEpoch(state.chart_effort.front().first).toMSecsSinceEpoch(), 145.0f);
    help_line->append(QDateTime::fromSecsSinceEpoch(state.chart_effort.back().first).toMSecsSinceEpoch(),  145.0f);
    help_line->setColor(QColor::fromRgbF(0.f, 0.f, 0.f, .4f));
    this->performance_chart->addSeries(help_line);
    help_line->attachAxis(this->performance_chart_time_axis);
    help_line->attachAxis(this->performance_chart_value_axis);
    
    // Area series mid

    for (auto area_series_mid_elem : area_series_mid_to_add) {
        this->performance_chart->addSeries(area_series_mid_elem);
        area_series_mid_elem->attachAxis(this->performance_chart_time_axis);
        area_series_mid_elem->attachAxis(this->performance_chart_value_axis);
    }
    
    // Scatter exercise plot

    using scatter_array_type = std::array<QScatterSeries*, 6>;
    scatter_array_type scatter_series_work;
    scatter_array_type scatter_series_journey;
    scatter_array_type scatter_series_misc;
    std::vector<QScatterSeries*> scatter_series_all;
    std::vector<scatter_array_type*> scatter_array_all = {
      &scatter_series_work, &scatter_series_journey, &scatter_series_misc
    };

    for (auto& elem : scatter_series_work) {
        elem = new QScatterSeries();
        elem->setMarkerShape(QScatterSeries::MarkerShapeCircle);
        elem->setMarkerSize(4.5);
        elem->setColor(QColor::fromRgb(250, 220, 80));
        elem->setBorderColor(QColor::fromRgb(128, 128, 128));
        scatter_series_all.push_back(elem);
    }
    
    for (auto& elem : scatter_series_journey) {
        elem = new QScatterSeries();
        elem->setMarkerShape(QScatterSeries::MarkerShapeCircle);
        elem->setMarkerSize(4.5);
        elem->setColor(QColor::fromRgb(250, 220, 80));
        elem->setBorderColor(QColor::fromRgb(128, 128, 128));
        scatter_series_all.push_back(elem);
    }
    
    for (auto& elem : scatter_series_misc) {
        elem = new QScatterSeries();
        elem->setMarkerShape(QScatterSeries::MarkerShapeCircle);
        elem->setMarkerSize(3.0);
        elem->setColor(QColor::fromRgb(250, 220, 80));
        elem->setBorderColor(QColor::fromRgb(128, 128, 128));
        scatter_series_all.push_back(elem);
    }
    
    for (auto elem : scatter_array_all) {
        elem->at(1)->setColor(QColor::fromRgb(160, 255, 160));
        elem->at(1)->setBorderColor(QColor::fromRgb(50, 100, 50));
        elem->at(2)->setColor(QColor::fromRgb(0, 160, 255));
        elem->at(2)->setBorderColor(QColor::fromRgb(0, 50, 100));
        elem->at(3)->setColor(QColor::fromRgb(255, 100, 100));
        elem->at(3)->setBorderColor(QColor::fromRgb(100, 0, 0));
        elem->at(4)->setColor(QColor::fromRgb(255, 180, 100));
        elem->at(4)->setBorderColor(QColor::fromRgb(100, 50, 50));
        elem->at(5)->setColor(QColor::fromRgb(128, 255, 255));
        elem->at(5)->setBorderColor(QColor::fromRgb(64, 100, 150));
    }

    std::set<float> sorted_max_effort;
    for (auto const &workout : this->extron_data->get_derived_exercise_state().workouts) {
        sorted_max_effort.emplace(workout.second.total_effort);
    }

    for (auto& elem : scatter_series_journey) {
        elem->setBorderColor(QColor::fromRgb(0, 0, 0));
    }
    for (auto& elem : scatter_series_misc) {
        elem->setBorderColor(QColor::fromRgb(150, 150, 150));
    }

    int max_corrected = 200;
    if (sorted_max_effort.size() > 0) {
        int idx = (int)(((sorted_max_effort.size()-1) * 8) / 10);
        max_corrected = *std::next(sorted_max_effort.begin(), idx);
    }
    
    max_corrected += 145 - (max_corrected % 145);
	int const hz_line_count = 1 + max_corrected / 145;
    float const max_value = float(max_corrected);
    float const max_value_plot_item = max_value - 5.f;
    
    for (auto const &workout : this->extron_data->get_derived_exercise_state().workouts) {
        int workout_type = 0;
        
        if (workout.second.dominant_exercise.find("yoga") == 0)
          workout_type = 1;
        if (workout.second.dominant_exercise.find("cycle") == 0)
          workout_type = 2;
        if (workout.second.dominant_exercise.find("run") == 0)
          workout_type = 3;
        if (workout.second.dominant_exercise.find("walk") == 0)
          workout_type = 4;
        if (workout.second.dominant_exercise.find("swim") == 0)
          workout_type = 5;
        
	    if (workout.second.type == "Workout")
	      scatter_series_work[workout_type]->append(   QDateTime::fromSecsSinceEpoch(workout.first.time).toMSecsSinceEpoch(), std::min(max_value_plot_item, workout.second.total_effort));
	    else if (workout.second.type == "Journey")
	      scatter_series_journey[workout_type]->append(QDateTime::fromSecsSinceEpoch(workout.first.time).toMSecsSinceEpoch(), std::min(max_value_plot_item, workout.second.total_effort));
	    else
	      scatter_series_misc[workout_type]->append(   QDateTime::fromSecsSinceEpoch(workout.first.time).toMSecsSinceEpoch(), std::min(max_value_plot_item, workout.second.total_effort));
    }
    
    for (const auto& elem : scatter_series_all)
      this->performance_chart->addSeries(elem);
    for (const auto& elem : scatter_series_all) {
        elem->attachAxis(this->performance_chart_time_axis);
        elem->attachAxis(this->performance_chart_value_axis);
    }

    this->performance_chart_value_axis->setRange(0.f, max_value);
    this->performance_chart_value_axis->setTickCount(hz_line_count);
    this->performance_chart_value_axis->setMinorTickCount(2);

    int week_count = ((range_max - range_min) / (7 * 24 * 3600)) + 1;
    int weeks_per_tick = std::max(1, week_count / 15);
    int weeks_tick_count = week_count / weeks_per_tick + std::min(1, week_count % weeks_per_tick);
    range_max = range_min + (7 * 24 * 3600) * weeks_per_tick * weeks_tick_count;

    this->performance_chart_time_axis->setRange(
      QDateTime::fromSecsSinceEpoch(range_min), QDateTime::fromSecsSinceEpoch(range_max)
    );
    this->performance_chart_time_axis->setTickCount(weeks_tick_count+1);
}


void tab_overview::event_must_rebuild_exercise_list()
{
    this->perform_build_overview_list();
    this->perform_build_exercise_list();
    this->perform_build_time_range_list();
    this->perform_build_chart();
}


void tab_overview::event_show_exercise_list()
{
    this->in_exercise_list_mode = true;

    this->perform_build_exercise_list();
}


void tab_overview::event_create_new_workout_workout()
{
    emit signal_create_new_workout("Workout");
}


void tab_overview::event_create_new_workout_incidental()
{
    emit signal_create_new_workout("Incidental");
}


void tab_overview::event_create_new_workout_journey()
{
    emit signal_create_new_workout("Journey");
}


void tab_overview::event_show_workouts(std::time_t start, std::time_t end)
{
    this->in_exercise_list_mode = false;
    this->show_range_start = start;
    this->show_range_end = end;

    this->perform_build_time_range_list();
}


void tab_overview::event_show_special_inspiration()
{
    this->in_exercise_list_mode = false;

    perform_build_special_inspiration_list();
}


void tab_overview::event_show_exercise(core::data_history::workout_time time)
{
    emit signal_show_workout(time);
}


void tab_overview::event_commence_update_balance()
{
    emit signal_commence_update_balance();
}


std::time_t tab_overview::get_start_of_week(std::time_t const &time) const
{
    return time - ((time - 4 * 24 * 3600) % (7 * 24 * 3600));
}


std::time_t tab_overview::get_end_of_week(std::time_t const &time) const
{
    return get_start_of_week(time) + (7 * 24 * 3600);
}


QColor tab_overview::get_week_colour(std::time_t const &time) const
{
    return this->week_colours[(time / (24 * 7 * 3600)) % this->week_colours.size()];
}


QColor tab_overview::get_week_colour_light(std::time_t const &time) const
{
    QColor colour = this->get_week_colour(time);

    colour.setRedF(std::pow(0.05 + colour.redF(), 0.20));
    colour.setGreenF(std::pow(0.05 + colour.greenF(), 0.20));
    colour.setBlueF(std::pow(0.05 + colour.blueF(), 0.20));

    return colour;
}


QColor tab_overview::get_week_colour_light2(std::time_t const &time) const
{
    QColor colour = this->get_week_colour_light(time);

    colour.setRedF(std::pow(0.05 + colour.redF(), 0.15));
    colour.setGreenF(std::pow(0.05 + colour.greenF(), 0.15));
    colour.setBlueF(std::pow(0.05 + colour.blueF(), 0.15));

    return colour;
}