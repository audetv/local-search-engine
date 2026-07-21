#pragma once

#include <QMainWindow>
#include <memory>
#include "index/index_reader.hpp"
#include "search/search_engine.hpp"
#include "tokenizer/stemmer.hpp"
#include "remote/manticore_config.hpp"
#include "remote/manticore_client.hpp"

class SearchWidget;
class ResultsWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSearch(const QString &query);

private:
    SearchWidget *search_widget_;
    ResultsWidget *results_widget_;

    // Поисковые движки
    std::unique_ptr<lse::IndexReader> reader_;
    std::unique_ptr<lse::SearchEngine> engine_;
    lse::Stemmer stemmer_;
    lse::ManticoreConfig manticore_config_;
    std::unique_ptr<lse::ManticoreClient> manticore_client_;

    void initSearchEngine();
    void performLocalSearch(const QString &query);
    void performRemoteSearch(const QString &query);
};