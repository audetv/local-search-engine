#include "main_window.hpp"
#include "search_widget.hpp"
#include "results_widget.hpp"
#include "search/query_builder.hpp"
#include "tokenizer/tokenizer.hpp"
#include <QVBoxLayout>
#include <QSplitter>
#include <QMessageBox>
#include <spdlog/spdlog.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), stemmer_(lse::StemmerLanguage::Russian)
{

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    search_widget_ = new SearchWidget(this);
    results_widget_ = new ResultsWidget(this);

    layout->addWidget(search_widget_);
    layout->addWidget(results_widget_); // Убрали stretch factor

    connect(search_widget_, &SearchWidget::searchRequested,
            this, &MainWindow::onSearch);

    initSearchEngine();
}

MainWindow::~MainWindow() = default;

void MainWindow::initSearchEngine()
{
    // Локальный поиск
    reader_ = std::make_unique<lse::IndexReader>();
    auto open_result = reader_->open("./user_index");
    if (open_result.has_value())
    {
        engine_ = std::make_unique<lse::SearchEngine>(*reader_, stemmer_);
        spdlog::info("Local index opened: {} docs", reader_->docCount());
    }

    // Удалённый поиск
    manticore_config_.load();
    if (manticore_config_.valid())
    {
        manticore_client_ = std::make_unique<lse::ManticoreClient>(manticore_config_);
    }
}

void MainWindow::onSearch(const QString &query)
{
    results_widget_->clear();

    // Локальный поиск
    if (engine_)
    {
        performLocalSearch(query);
    }

    // Удалённый поиск
    if (manticore_client_)
    {
        performRemoteSearch(query);
    }
}

void MainWindow::performLocalSearch(const QString &query)
{
    std::string q = query.toStdString();

    auto query_node = lse::QueryBuilder::queryString(q);
    if (!query_node.has_value())
        return;

    // Термы для подсветки
    lse::Tokenizer tokenizer(q);
    auto tokens = tokenizer.tokenize();
    std::vector<std::string> highlight_terms;
    if (tokens.has_value())
    {
        for (const auto &t : *tokens)
        {
            highlight_terms.push_back(t.text);
        }
    }

    auto results = engine_->execute(*query_node, 10, "", "", "", highlight_terms);
    if (!results.has_value())
        return;

    for (const auto &hit : *results)
    {
        // Генерируем подсвеченный контент
        QString content = QString::fromStdString(hit->content);
        if (!hit->highlights.empty())
        {
            // Заменяем совпадения на <mark>...</mark>
            QString html;
            size_t last = 0;
            for (const auto &hl : hit->highlights)
            {
                html += content.mid(last, hl.offset - last).toHtmlEscaped();
                html += "<span style='background-color: #e1e0e0;'>" + content.mid(hl.offset, hl.length).toHtmlEscaped() + "</span>";
                last = hl.offset + hl.length;
            }
            html += content.mid(last).toHtmlEscaped();
            content = html;
        }
        else
        {
            content = content.toHtmlEscaped();
        }

        results_widget_->addResult(
            QString::fromStdString(hit->title),
            QString::fromStdString(hit->author),
            QString::fromStdString(hit->genre),
            content,
            hit->bm25_score,
            "local");
    }
}

void MainWindow::performRemoteSearch(const QString &query)
{
    std::string q = query.toStdString();

    auto results = manticore_client_->search(q, "query_string", 10);
    if (!results.has_value())
        return;

    for (const auto &hit : results->hits)
    {
        QString content = QString::fromStdString(
            hit.highlight_content.empty() ? hit.content : hit.highlight_content);

        results_widget_->addResult(
            QString::fromStdString(hit.title),
            QString::fromStdString(hit.author),
            QString::fromStdString(hit.genre),
            content,
            hit.score,
            "remote");
    }
}