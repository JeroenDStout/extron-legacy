#include "extron_ui_qt/tab_workout.h"

#include "extron_ui_qt/clickable_label.h"

#include <qmessagebox>

#include <iomanip>
#include <sstream>
#include <random>

using namespace extron::ui_qt;


ui_tab_workout::ui_tab_workout(QWidget *parent, core::extron_data *data, core::data_history::workout_time time)
: QWidget(parent),
  extron_data(data)
{
    ui.setupUi(this);

    this->original_data.time = time;

    label_font.setPointSize(8);
    label_font.setFamily("verdana");

    this->show_only_low_progress = false;
  }


ui_tab_workout::~ui_tab_workout()
{
}
    

void ui_tab_workout::set_type(std::string const &type)
{
    this->new_data.type = type;

    int idx = this->ui.exercise_type->findText(QString::fromStdString(type));
    if (idx != -1) {
      this->ui.exercise_type->setCurrentIndex(idx);
      return;
    }
    this->ui.exercise_type->setCurrentIndex(0);
    
    this->perform_name_tab();
}


void ui_tab_workout::perform_setup()
{
    if (this->original_data.time.time != 0) {
        if (!extron_data->get_workout_data(this->original_data.time, &this->original_data)) {
            QMessageBox msgBox;
            msgBox.setText(QString::fromStdString("Error: This exercise was not found!"));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
        }
        this->new_data = this->original_data;
    }

    if (this->new_data.time.time == 0)
      time(&this->new_data.time.time);

    QDateTime date_time(QDateTime::fromSecsSinceEpoch((this->new_data.time.time)));
    this->ui.exercise_time->setDateTime(date_time);

    this->perform_name_tab();
    this->perform_build_exercise_sample_list();
    this->perform_build_exercise_chosen_list();
    this->perform_build_exercise_info();
    this->perform_build_performance();

    connect(
      this->ui.exercise_name_filter, &QLineEdit::textChanged,
      this, &ui_tab_workout::event_must_rebuild_sample_list
    );
    connect(
      this->ui.exercise_type, &QComboBox::currentTextChanged,
      this, &ui_tab_workout::event_updated_type
    );
    connect(
      this->ui.exercise_time, &QDateTimeEdit::dateTimeChanged,
      this, &ui_tab_workout::event_updated_time
    );
}


extron::core::data_history::workout_time ui_tab_workout::get_old_workout_time() const
{
    return this->original_data.time;
}
    

void ui_tab_workout::perform_build_exercise_sample_list()
{
    int static const row_height = 15;
    int static const row_height_small = 9;

    auto const filter = this->get_workout_filter();
    auto const &state = this->extron_data->get_derived_exercise_state();

    QWidget *parent_widget = this->ui.left_area_scroll;

    QGridLayout *layout = qobject_cast<QGridLayout*>(parent_widget->layout());

    QLayoutItem *item;
    while (item = layout->takeAt(0)) {
        if (QWidget *w = item->widget())
          w->deleteLater();
        delete item;
    }

    std::vector<format::exe_data_ex> exercises = format::get_filtered_sorted_exercises(state, filter);

    std::string prev_category = "";
    int cur_row = 0;
    for (auto const& exercise : exercises) {
        if (this->show_only_low_progress) {
            if (exercise.full_relative_advance > -1.f) continue;
            if (exercise.is_suspended) continue;
        }
        
        bool const has_been_selected = (
          this->new_data.workout_counts.find(exercise.name) != this->new_data.workout_counts.end()
        );
        
        if (exercise.category_display_name != prev_category) {
            QLabel *cat_label = new QLabel();
            cat_label->setText(QString::fromStdString(exercise.category_display_name));
            cat_label->setFont(label_font);
            cat_label->setStyleSheet("font-size: 16px; font-weight: bold");
            cat_label->setContentsMargins(0, 10, 0, 0);
            layout->addWidget(cat_label, cur_row++, 0, 1, 4);
            prev_category = exercise.category_display_name;
        }
        
        if (has_been_selected) {
            QLabel *exe_label = new QLabel();
            QString text = u8"      ▶";
            text += QString::fromStdString(exercise.display_name);
            exe_label->setText(text);
            exe_label->setStyleSheet("font-weight: bold");
            exe_label->setFixedHeight(row_height);
            exe_label->setFont(label_font);
            layout->addWidget(exe_label, cur_row, 1, 1, 4);
        }
        else {
            clickable_label_str *exe_label = new clickable_label_str("", exercise.name);
            QString text = u8"   ";
            text += QString::fromStdString(exercise.display_name);
            exe_label->setText(text);
            connect(
              exe_label, &clickable_label_str::clicked,
              this, &ui_tab_workout::event_enable_exercise
            );
            exe_label->setFixedHeight(row_height);
            exe_label->setFont(label_font);
            layout->addWidget(exe_label, cur_row, 2);
            
            if (!exercise.is_suspended) {
                std::string emoji_output = format::format_progress_to_emoji(exercise.full_relative_advance);
                std::string weight_emoji_output = format::format_balance_to_emoji(exercise.relative_balance);
                
                QLabel *emoji_label = new QLabel();
                emoji_label->setText(QString::fromStdString(emoji_output));
                emoji_label->setAlignment(Qt::AlignCenter);
                layout->addWidget(emoji_label, cur_row, 1);
                
                QLabel *median_label = new QLabel();
                median_label->setText(QString::fromStdString(format::format_median_range(exercise.weighed_median_range)));
                median_label->setAlignment(Qt::AlignCenter);
                layout->addWidget(median_label, cur_row, 3);
            }
        }
        
        ++cur_row;
    }

    layout->addItem(new QSpacerItem(0,10, QSizePolicy::Expanding, QSizePolicy::Expanding), cur_row, 4);
}


void ui_tab_workout::perform_build_exercise_chosen_list()
{
    auto const &state = this->extron_data->get_derived_exercise_state();

    QWidget *parent_widget = this->ui.right_area_scroll;

    QGridLayout *layout = qobject_cast<QGridLayout*>(parent_widget->layout());

    QLayoutItem *item;
    while (item = layout->takeAt(0)) {
      if (QWidget *w = item->widget())
        w->deleteLater();
      delete item;
    }
    related_widgets.clear();

    std::vector<format::exe_data_ex> exercises = format::get_filtered_sorted_exercises(state, {});
    std::string previous_category = "";

    int cur_row = 0;
    for (auto const& exercise : exercises) {
        auto const selected_ex = this->new_data.workout_counts.find(exercise.name);
        if (selected_ex == this->new_data.workout_counts.end())
          continue;
        
        if (previous_category != exercise.category_display_name) {
            previous_category = exercise.category_display_name;
            
            QLabel *exe_label = new QLabel();
            QString text = QString::fromStdString(exercise.category_display_name);
            exe_label->setText(text);
            exe_label->setStyleSheet("font-weight: bold");
            exe_label->setFont(label_font);
            layout->addWidget(exe_label, cur_row, 0);
        }
        
        clickable_label_str *exe_label = new clickable_label_str("", exercise.name);
        QString text = QString::fromStdString(exercise.display_name);
        exe_label->setText(text);
        exe_label->setCursor(Qt::PointingHandCursor);
        connect(
          exe_label, &clickable_label_str::clicked,
          this, &ui_tab_workout::event_disable_exercise
        );
        exe_label->setFont(label_font);
        layout->addWidget(exe_label, cur_row, 1);
        
        QLabel *performance_label = new QLabel();
        performance_label->setText("");
        performance_label->setFont(label_font);
        performance_label->setAlignment(Qt::AlignCenter);
        layout->addWidget(performance_label, cur_row, 2);
        
        QLineEdit *line_count_done = new clickable_line_edit();
        line_count_done->setObjectName(QString::fromStdString(exercise.name));
        line_count_done->setText(QString::number(selected_ex->second.count));
        line_count_done->setMaximumWidth(50);
        line_count_done->setAlignment(Qt::AlignCenter);
        connect(
          line_count_done, &QLineEdit::editingFinished,
          this, &ui_tab_workout::event_update_count_done
        );
        layout->addWidget(line_count_done, cur_row, 3);
        
        QLabel *label = new QLabel();
        label->setText("/");
        label->setFont(label_font);
        layout->addWidget(label, cur_row, 4);
        
        QLineEdit *line_count_target = new clickable_line_edit();
        line_count_target->setObjectName(QString::fromStdString(exercise.name));
        line_count_target->setText(QString::number(selected_ex->second.target));
        line_count_target->setMaximumWidth(50);
        line_count_target->setAlignment(Qt::AlignCenter);
        connect(
          line_count_target, &QLineEdit::editingFinished,
          this, &ui_tab_workout::event_update_count_target
        );
        layout->addWidget(line_count_target, cur_row, 5);
        
        std::string weight_emoji_output = format::format_balance_to_emoji(exercise.relative_balance);
        
        label = new QLabel();
        label->setText(QString::fromStdString(format::format_median_range(exercise.weighed_median_range)));
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label, cur_row, 7);
        
        label = new QLabel();
        label->setText(QString::fromStdString(format::format_momentum(exercise.momentum / exercise.weighed_median)));
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label, cur_row, 8);
        
        this->related_widgets[exercise.name] = { performance_label };
        
        ++cur_row;
    }

    layout->addItem(new QSpacerItem(0,10, QSizePolicy::Expanding, QSizePolicy::Expanding), cur_row, 9);

    this->perform_build_performance();
}


void ui_tab_workout::perform_build_exercise_info()
{
    auto const &state = this->extron_data->get_derived_exercise_state();

    float sum_weight_count	= 0.f;
    float sum_weight_target	= 0.f;

    for (auto const &counts : this->new_data.workout_counts) {
        sum_weight_count	+= state.get_effort_for_exercise_count_balanced(counts.first, counts.second.count);
        sum_weight_target += state.get_effort_for_exercise_count_balanced(counts.first, counts.second.target);
    }

    std::stringstream ss;
    ss << std::setprecision(1) << std::fixed;
    ss << this->new_data.workout_counts.size() << " exercises";
    
    if (sum_weight_count > 0.f)
      ss << u8" — Effort: " << sum_weight_count;
    if (sum_weight_target > 0.f)
      ss << u8" — Goal effort: " << sum_weight_target;

    this->ui.exercise_info->setText(QString::fromStdString(ss.str()));
}


void ui_tab_workout::perform_build_performance()
{
    auto const &state = this->extron_data->get_derived_exercise_state();

    for (auto &item : this->related_widgets) {
      auto const& performance = this->new_data.workout_counts[item.first];
      if (performance.count == 0) {
        item.second.performance_label->setText("");
        continue;
      }

      auto const& reg_perform = state.extercises.find(item.first);

      int comp_reg = 0;
      if (performance.count < reg_perform->second.weighed_median_range.first)
        comp_reg = -1;
      else if (performance.count > reg_perform->second.weighed_median_range.second)
        comp_reg = 2;
      else if (performance.count >= reg_perform->second.weighed_median_range.second)
        comp_reg = 1;

      if (performance.count < performance.target) {
        switch (comp_reg) {
          case -1: item.second.performance_label->setText(u8"💦💦"); continue;
          case  0: item.second.performance_label->setText(u8"💦"); continue;
          case  1: item.second.performance_label->setText(u8"⭐"); continue;
          case  2: item.second.performance_label->setText(u8"⭐⭐"); continue;
        };
      }
      if (performance.count == performance.target) {
        switch (comp_reg) {
          case -1: item.second.performance_label->setText(u8"⭐"); continue;
          case  0: item.second.performance_label->setText(u8"⭐⭐"); continue;
          case  1: item.second.performance_label->setText(u8"🎀"); continue;
          case  2: item.second.performance_label->setText(u8"🎀🎀"); continue;
        }; continue;
      }
      if (performance.count > performance.target) {
        switch (comp_reg) {
          case -1: item.second.performance_label->setText(u8"💪"); continue;
          case  0: item.second.performance_label->setText(u8"💪💪"); continue;
          case  1: item.second.performance_label->setText(u8"🎀🎀"); continue;
          case  2: item.second.performance_label->setText(u8"🎀🎀🎀"); continue;
        };
      }
    }
}


void ui_tab_workout::event_must_rebuild_exercise_list()
{
    this->perform_build_exercise_sample_list();
    this->perform_build_exercise_chosen_list();
    this->perform_build_exercise_info();
    this->perform_build_performance();
    this->perform_name_tab();
}


void ui_tab_workout::event_must_rebuild_sample_list()
{
    this->perform_build_exercise_sample_list();
}


void ui_tab_workout::event_updated_type()
{
    this->new_data.type = this->ui.exercise_type->currentText().toStdString();
    this->perform_name_tab();
}


void ui_tab_workout::event_updated_time()
{
    this->new_data.time.time = this->ui.exercise_time->dateTime().toSecsSinceEpoch();
    this->perform_name_tab();
}


void ui_tab_workout::event_enable_exercise(std::string name)
{
    static bool const use_random = true;

    auto const &state = this->extron_data->get_derived_exercise_state();
    auto exercise = state.extercises.find(name);
    if (exercise == state.extercises.end())
      return;

    auto &workout_count = this->new_data.workout_counts[name];
    if (workout_count.target == 0) {
        std::default_random_engine generator;
        generator.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        
        if (exercise->second.weighed_median == 0)
          workout_count.target = 10.f;
        else if (!use_random)
          workout_count.target = exercise->second.weighed_median;
        else {
            std::uniform_real_distribution<float> dist(
              exercise->second.weighed_median_range.first,
              exercise->second.weighed_median_range.second + 0.9999f
            );
            workout_count.target = dist(generator);
        }
        
        if (use_random) {
            std::normal_distribution<float> distribution(0.f, .1f);
            
            workout_count.target = int((float(workout_count.target) + .5f) * std::exp(distribution(generator)));
            workout_count.target &= ~0x1;
        }
    }

    
    this->perform_build_exercise_sample_list();
    this->perform_build_exercise_chosen_list();
    this->perform_build_exercise_info();
    this->perform_name_tab();
}


void ui_tab_workout::event_disable_exercise(std::string name)
{
    this->new_data.workout_counts.erase(name);
    
    this->perform_build_exercise_sample_list();
    this->perform_build_exercise_chosen_list();
    this->perform_build_exercise_info();
    this->perform_name_tab();
}


void ui_tab_workout::event_update_count_done()
{
    auto const *sender = qobject_cast<QLineEdit*>(QObject::sender());
    std::string const name = sender->objectName().toStdString();
    std::string const value = sender->displayText().toStdString();

    try {
        int const int_value = std::stoi(value);
        
        auto &workout_count = this->new_data.workout_counts[name];
        if (int(workout_count.count) == int_value)
          return;
        
        workout_count.count = int_value;
    }
    catch (...) {
        this->perform_build_exercise_chosen_list();
    };
    
    this->perform_build_exercise_info();
    this->perform_build_performance();
}


void ui_tab_workout::event_update_count_target()
{
    auto const *sender = qobject_cast<QLineEdit*>(QObject::sender());
    std::string const name = sender->objectName().toStdString();
    std::string const value = sender->displayText().toStdString();

    try {
        int const int_value = std::stoi(value);
        
        auto &workout_count = this->new_data.workout_counts[name];
        if (int(workout_count.target) == int_value)
          return;
        
        workout_count.target = int_value;
    }
    catch (...) {
        this->perform_build_exercise_chosen_list();
    };
    
    this->perform_build_exercise_info();
    this->perform_build_performance();
}


void ui_tab_workout::event_save()
{
    this->extron_data->save_workout_data(this->original_data.time, this->new_data.time, this->new_data);
    this->original_data.time = this->new_data.time;
    emit signal_history_requires_save();
}


void ui_tab_workout::event_revert()
{
    this->new_data = this->original_data;

    this->perform_build_exercise_sample_list();
    this->perform_build_exercise_chosen_list();
    this->perform_build_exercise_info();
    this->perform_name_tab();
}


void ui_tab_workout::event_close()
{
    emit signal_close_tab(this);
}


void ui_tab_workout::event_toggle_low_progress_only()
{
    this->show_only_low_progress = !this->show_only_low_progress;

    this->ui.toggle_only_low_progress->setText(this->show_only_low_progress ? u8"🌊" : u8"⭐");

    this->perform_build_exercise_sample_list();
}


void ui_tab_workout::perform_name_tab()
{
    std::tm tm;
    localtime_s(&tm, &this->new_data.time.time);

    std::stringstream ss;
    ss << this->new_data.type;
    ss << " ";
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M");
    ss << " (" << this->new_data.workout_counts.size() << ")";

    emit signal_rename_tab(this, ss.str());
}


format::workout_filter ui_tab_workout::get_workout_filter()
{
    format::workout_filter filter;

    filter.name = this->ui.exercise_name_filter->text().toStdString();

    return filter;
}


ui_tab_workout::workout_setup_data::workout_setup_data():
  time({ 0, 0 })
{
}