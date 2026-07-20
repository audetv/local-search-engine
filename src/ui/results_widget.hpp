#pragma once

#include <QWidget>
#include <QListWidget>
#include <QListWidgetItem>

class ResultsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ResultsWidget(QWidget *parent = nullptr);

    void addResult(const QString &query, const QString &title,
                   const QString &author, const QString &genre,
                   const QString &content);
    void clear();

private:
    QListWidget *list_widget_;
};