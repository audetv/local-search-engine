#include "results_widget.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollBar>

ResultsWidget::ResultsWidget(QWidget *parent)
    : QWidget(parent), font_size_(20)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    list_widget_ = new QListWidget(this);
    list_widget_->setWordWrap(true);
    list_widget_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list_widget_->verticalScrollBar()->setSingleStep(10);
    list_widget_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list_widget_->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(list_widget_);

    applyFontSize();
}

void ResultsWidget::setFontSize(int size)
{
    font_size_ = size;
    applyFontSize();
}

void ResultsWidget::applyFontSize()
{
    list_widget_->setStyleSheet(
        QString("QListWidget { font-size: %1px; }").arg(font_size_));
}

void ResultsWidget::addResult(const QString &title, const QString &author,
                              const QString &genre, const QString &content,
                              float score, const QString &source)
{
    auto *item = new QListWidgetItem();
    auto *widget = new QWidget();
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(4, 2, 4, 4);
    layout->setSpacing(0);

    QString header_text = QString(
                              "<b style='font-size:%1px;'>%2</b> "
                              "<span style='color: gray;'>[%3] — %4 | %5: %6</span>")
                              .arg(font_size_)
                              .arg(title.toHtmlEscaped(), genre.toHtmlEscaped(),
                                   author.toHtmlEscaped(), source, QString::number(score, 'f', 1));

    auto *header = new QLabel(header_text);
    header->setTextFormat(Qt::RichText);
    header->setWordWrap(true);

    auto *content_label = new QLabel(content);
    content_label->setTextFormat(Qt::RichText);
    content_label->setWordWrap(true);

    layout->addWidget(header);
    layout->addWidget(content_label);

    widget->setStyleSheet(
        QString("background: #fafafa; border-bottom: 1px solid #ddd; font-size: %1px;")
            .arg(font_size_));

    item->setSizeHint(widget->sizeHint());
    list_widget_->addItem(item);
    list_widget_->setItemWidget(item, widget);
}

void ResultsWidget::clear()
{
    list_widget_->clear();
}