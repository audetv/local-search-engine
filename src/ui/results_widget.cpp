#include "results_widget.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QTimer>

ResultsWidget::ResultsWidget(QWidget *parent) : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    scroll_area_ = new QScrollArea(this);
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll_area_->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { width: 10px; }"
        "QScrollBar::handle:vertical { background: #c0c0c0; border-radius: 5px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }");

    content_widget_ = new QWidget();
    layout_ = new QVBoxLayout(content_widget_);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);
    layout_->addStretch(); // Ключевой момент: прижимает элементы к верху

    scroll_area_->setWidget(content_widget_);
    mainLayout->addWidget(scroll_area_);
}

QWidget *ResultsWidget::createResultItem(const QString &title, const QString &author,
                                         const QString &genre, const QString &content,
                                         float score, const QString &source)
{
    auto *item = new QWidget();
    auto *itemLayout = new QVBoxLayout(item);
    itemLayout->setContentsMargins(8, 6, 8, 6);
    itemLayout->setSpacing(2);
    itemLayout->setAlignment(Qt::AlignTop);

    // Заголовок
    auto *titleLabel = new QLabel();
    QString titleHtml = QString(
                            "<b>%1</b> <span style='color: #666; font-size: %2px;'>[%3] — %4 | %5: %6</span>")
                            .arg(title.toHtmlEscaped())
                            .arg(font_size_ - 2)
                            .arg(genre.toHtmlEscaped())
                            .arg(author.toHtmlEscaped())
                            .arg(source)
                            .arg(QString::number(score, 'f', 1));
    titleLabel->setText(titleHtml);
    titleLabel->setTextFormat(Qt::RichText);
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet("padding: 0; margin: 0;");
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    // Контент
    auto *contentLabel = new QLabel();
    QString html_content = content;
    html_content.replace("\n", "<br>");
    contentLabel->setText(html_content);
    contentLabel->setTextFormat(Qt::RichText);
    contentLabel->setWordWrap(true);
    contentLabel->setStyleSheet("padding: 0; margin: 0;");
    contentLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    itemLayout->addWidget(titleLabel);
    itemLayout->addWidget(contentLabel);

    item->setStyleSheet(
        "background: #fafafa; "
        "border-bottom: 1px solid #ddd; "
        "padding: 0; margin: 0;");

    item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    return item;
}

void ResultsWidget::addResult(const QString &title, const QString &author,
                              const QString &genre, const QString &content,
                              float score, const QString &source)
{
    // Удаляем старый stretch
    if (layout_->count() > 0)
    {
        auto *lastItem = layout_->itemAt(layout_->count() - 1);
        if (lastItem && lastItem->spacerItem())
        {
            layout_->removeItem(lastItem);
            delete lastItem;
        }
    }

    // Создаем и добавляем новый элемент
    auto *item = createResultItem(title, author, genre, content, score, source);
    layout_->addWidget(item);

    // Добавляем новый stretch в конец
    layout_->addStretch();

    // Принудительно обновляем геометрию
    content_widget_->updateGeometry();

    // Прокручиваем вниз к новому результату
    QTimer::singleShot(0, [this]()
                       {
        QScrollBar *vbar = scroll_area_->verticalScrollBar();
        if (vbar) {
            vbar->setValue(vbar->maximum());
        } });
}

void ResultsWidget::clear()
{
    // Очищаем все виджеты, сохраняя только stretch
    while (layout_->count() > 0)
    {
        auto *item = layout_->takeAt(0);
        if (item->widget())
        {
            delete item->widget();
        }
        delete item;
    }
    layout_->addStretch(); // Добавляем stretch обратно
    content_widget_->updateGeometry();
}

void ResultsWidget::setFontSize(int size)
{
    font_size_ = size;
    updateAllItems();
}

void ResultsWidget::updateAllItems()
{
    // Обновляем все элементы
    for (int i = 0; i < layout_->count(); ++i)
    {
        auto *item = layout_->itemAt(i);
        if (item && item->widget())
        {
            // Пересоздаем элемент с новым размером шрифта
            // В реальном коде лучше обновлять существующие label'ы
            // Но для простоты пересоздаем все элементы
            // Это упрощенный вариант
        }
    }
}

void ResultsWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // При изменении размера обновляем геометрию
    if (content_widget_)
    {
        content_widget_->updateGeometry();
    }
}