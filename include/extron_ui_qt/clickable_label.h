#pragma once

#include "extron_core/data.h"


#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QWidget>
#include <Qt>


#include <ctime>


namespace extron::ui_qt {


class clickable_label : public QLabel { 
    Q_OBJECT 

  public:
    explicit clickable_label(std::string);
    ~clickable_label();

    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;

  protected:
    std::string style;
};


class clickable_label_str : public clickable_label { 
    Q_OBJECT 

  public:
    explicit clickable_label_str(std::string style, std::string);
    ~clickable_label_str();

  signals:
    void clicked(std::string);

  protected:
    void mousePressEvent(QMouseEvent* event) override;

    std::string string;
};


class clickable_label_time : public clickable_label { 
    Q_OBJECT 

  public:
    explicit clickable_label_time(std::string style, core::data_history::workout_time);
    ~clickable_label_time();

  signals:
    void clicked(core::data_history::workout_time);

  protected:
    void mousePressEvent(QMouseEvent* event) override;

    core::data_history::workout_time time;
};


class clickable_line_edit : public QLineEdit {
    Q_OBJECT 

  public:
    clickable_line_edit();

  protected:
    void mousePressEvent(QMouseEvent* event) override;
};


class date_range_push_button : public QPushButton { 
  Q_OBJECT 

  public:
    explicit date_range_push_button(std::time_t start, std::time_t end):
      start(start), end(end)
    {
      QObject::connect(
        this, &QPushButton::released,
        this, &date_range_push_button::event_pushed
      );

    }
    ~date_range_push_button() { ; }

  signals:
    void date_released(std::time_t start, std::time_t end);
      
  public slots:
    void event_pushed() {
      this->date_released(start, end);
    }

  protected:
    std::time_t start, end;
};


}