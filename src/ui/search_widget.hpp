#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SearchWidget(QWidget *parent = nullptr);

signals:
    void searchRequested(const QString &query);

private slots:
    void onSearchClicked();

private:
    QLineEdit *query_edit_;
    QPushButton *search_button_;
};