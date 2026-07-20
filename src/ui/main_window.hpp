#pragma once

#include <QMainWindow>
#include <memory>

class SearchWidget;
class ResultsWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onSearch(const QString &query);

private:
    SearchWidget *search_widget_;
    ResultsWidget *results_widget_;
};