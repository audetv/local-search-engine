#include "results_widget.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollBar>

ResultsWidget::ResultsWidget(QWidget *parent) : QWidget(parent), font_size_(16)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    list_widget_ = new QListWidget(this);
    list_widget_->setWordWrap(true);
    list_widget_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list_widget_->verticalScrollBar()->setSingleStep(10);
    list_widget_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list_widget_->setSelectionMode(QAbstractItemView::NoSelection);
    list_widget_->setUniformItemSizes(false);
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
    // Заменяем \n на <br> для RichText
    QString html_content = content;
    html_content.replace("\n", "<br>");

    QString header_text = QString(
                              "<p style='margin:0; padding:0;'><b>%1</b> "
                              "<span style='color: gray;'>[%2] — %3 | %4: %5</span></p>")
                              .arg(title.toHtmlEscaped(), genre.toHtmlEscaped(),
                                   author.toHtmlEscaped(), source, QString::number(score, 'f', 1));

    auto *widget = new QWidget();
    widget->setStyleSheet(
        QString("background: #fafafa; border-bottom: 1px solid #ddd; padding: 2px 4px;"));

    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 2, 0, 4);
    layout->setSpacing(0);

    auto *header = new QLabel(header_text);
    header->setTextFormat(Qt::RichText);
    header->setWordWrap(true);
    header->setStyleSheet("padding: 0px; margin: 0px; border: none; background: transparent;");

    auto *content_label = new QLabel(html_content);
    content_label->setTextFormat(Qt::RichText);
    content_label->setWordWrap(true);
    content_label->setStyleSheet(
        "padding: 0px; margin: 0px; border: none; background: transparent;");

    layout->addWidget(header);
    layout->addWidget(content_label);

    // Важно: пересчитываем размер после добавления
    widget->adjustSize();

    auto *item = new QListWidgetItem();
    item->setSizeHint(widget->sizeHint());
    list_widget_->addItem(item);
    list_widget_->setItemWidget(item, widget);
}

void ResultsWidget::clear()
{
    list_widget_->clear();
}