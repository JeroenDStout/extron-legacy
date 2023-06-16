#include "extron_ui_qt/clickable_label.h"

#include <qevent.h>


using namespace extron::ui_qt;


clickable_label::clickable_label(std::string style):
  style(style)
{    
	this->setMouseTracking(true);
    this->setCursor(Qt::PointingHandCursor);
}


clickable_label::~clickable_label()
{
}


void clickable_label::enterEvent(QEnterEvent *)
{
	std::string style_ = this->style;
	style_ += "; font-style: italic";
	this->setStyleSheet(QString::fromStdString(style_));
}


void clickable_label::leaveEvent(QEvent *)
{
	this->setStyleSheet(QString::fromStdString(style));
}


clickable_label_str::clickable_label_str(std::string style, std::string str):
  clickable_label(style),
  string(str)
{    
}


clickable_label_str::~clickable_label_str()
{
}


void clickable_label_str::mousePressEvent(QMouseEvent *)
{
    emit clicked(string);
}


clickable_label_time::clickable_label_time(std::string style, core::data_history::workout_time time):
  clickable_label(style),
  time(time)
{    
}


clickable_label_time::~clickable_label_time()
{
}


void clickable_label_time::mousePressEvent(QMouseEvent *)
{
    emit clicked(time);
}


clickable_line_edit::clickable_line_edit()
{
}


void clickable_line_edit::mousePressEvent(QMouseEvent *)
{
	this->selectAll();
}