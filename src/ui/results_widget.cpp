#include "results_widget.hpp"
#include <QVBoxLayout>
#include <QLabel>

ResultsWidget::ResultsWidget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    list_widget_ = new QListWidget(this);
    list_widget_->setWordWrap(true);
    layout->addWidget(list_widget_);
}

void ResultsWidget::addResult(const QString &title, const QString &author,
                              const QString &genre, const QString &content,
                              float score, const QString &source)
{
    auto *item = new QListWidgetItem();
    auto *widget = new QWidget();
    auto *layout = new QVBoxLayout(widget);

    auto *title_label = new QLabel(QString("<b>%1</b> [%2]").arg(title, genre));
    title_label->setTextFormat(Qt::RichText);

    auto *author_label = new QLabel(author);

    auto *content_label = new QLabel(content);
    content_label->setWordWrap(true);
    content_label->setTextFormat(Qt::PlainText);

    layout->addWidget(title_label);
    layout->addWidget(author_label);
    layout->addWidget(content_label);

    item->setSizeHint(widget->sizeHint());
    list_widget_->addItem(item);
    list_widget_->setItemWidget(item, widget);
}

void ResultsWidget::clear()
{
    list_widget_->clear();
}