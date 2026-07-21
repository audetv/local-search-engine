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
    list_widget_->setStyleSheet("QListWidget { border: none; }");
    layout->addWidget(list_widget_);
}

void ResultsWidget::setFontSize(int size)
{
    font_size_ = size;
    list_widget_->update(); // перерисовать
}

void ResultsWidget::addResult(const QString &title, const QString &author,
                              const QString &genre, const QString &content,
                              float score, const QString &source)
{
    QString html_content = content;
    html_content.replace("\n", "<br>");

    QString html = QString(
                       "<div style='font-size:%1px;'>"
                       "<p style='margin:0; padding:0 0 2px 0;'><b>%2</b> "
                       "<span style='color: gray; font-size:%3px;'>[%4] — %5 | %6: %7</span></p>"
                       "<p style='margin:0; padding:0;'>%8</p>"
                       "</div>")
                       .arg(font_size_)
                       .arg(title.toHtmlEscaped())
                       .arg(font_size_ - 2) // автор чуть мельче
                       .arg(genre.toHtmlEscaped(), author.toHtmlEscaped(),
                            source, QString::number(score, 'f', 1))
                       .arg(html_content);

    auto *label = new QLabel(html);
    label->setTextFormat(Qt::RichText);
    label->setWordWrap(true);
    label->setStyleSheet(
        QString("background: #fafafa; border-bottom: 1px solid #ddd; "
                "padding: 4px 4px 6px 4px; font-size: %1px;")
            .arg(font_size_));

    auto *item = new QListWidgetItem();
    item->setSizeHint(label->sizeHint());
    list_widget_->addItem(item);
    list_widget_->setItemWidget(item, label);
}

void ResultsWidget::clear()
{
    list_widget_->clear();
}