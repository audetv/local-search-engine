#include "results_widget.hpp"
#include <QVBoxLayout>
#include <QLabel>

ResultsWidget::ResultsWidget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    list_widget_ = new QListWidget(this);
    list_widget_->setWordWrap(true);
    list_widget_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list_widget_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    list_widget_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(list_widget_);
}

void ResultsWidget::addResult(const QString &title, const QString &author,
                              const QString &genre, const QString &content,
                              float score, const QString &source)
{
    auto *item = new QListWidgetItem();
    auto *widget = new QWidget();
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(1);

    auto *header = new QLabel(
        QString("<b>%1</b> <span style='color: gray;'>[%2]</span> — %3 | score: %4")
            .arg(title.toHtmlEscaped(), genre.toHtmlEscaped(),
                 author.toHtmlEscaped(), QString::number(score, 'f', 1)));
    header->setTextFormat(Qt::RichText);
    header->setWordWrap(true);

    auto *content_label = new QLabel(content);
    content_label->setTextFormat(Qt::RichText);
    content_label->setWordWrap(true);

    layout->addWidget(header);
    layout->addWidget(content_label);

    item->setSizeHint(widget->sizeHint());
    list_widget_->addItem(item);
    list_widget_->setItemWidget(item, widget);
}

void ResultsWidget::clear()
{
    list_widget_->clear();
}