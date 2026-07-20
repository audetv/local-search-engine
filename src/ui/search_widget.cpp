#include "search_widget.hpp"
#include <QHBoxLayout>

SearchWidget::SearchWidget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    query_edit_ = new QLineEdit(this);
    query_edit_->setPlaceholderText("Введите поисковый запрос...");
    query_edit_->setMinimumHeight(36);

    search_button_ = new QPushButton("Найти", this);
    search_button_->setMinimumHeight(36);

    layout->addWidget(query_edit_, 1);
    layout->addWidget(search_button_);

    connect(search_button_, &QPushButton::clicked, this, &SearchWidget::onSearchClicked);
    connect(query_edit_, &QLineEdit::returnPressed, this, &SearchWidget::onSearchClicked);
}

void SearchWidget::onSearchClicked()
{
    QString query = query_edit_->text().trimmed();
    if (!query.isEmpty())
    {
        emit searchRequested(query);
    }
}