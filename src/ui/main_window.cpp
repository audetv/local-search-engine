#include "main_window.hpp"
#include "search_widget.hpp"
#include "results_widget.hpp"
#include <QVBoxLayout>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QVBoxLayout(central);

    search_widget_ = new SearchWidget(this);
    results_widget_ = new ResultsWidget(this);

    layout->addWidget(search_widget_);
    layout->addWidget(results_widget_, 1);

    connect(search_widget_, &SearchWidget::searchRequested,
            this, &MainWindow::onSearch);
}

void MainWindow::onSearch(const QString &query)
{
    // Заглушка: выводим запрос
    results_widget_->addResult(query, "Title", "Author", "Genre", "Content...");
}