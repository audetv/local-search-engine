#pragma once

#include <QWidget>
#include <QListWidget>
#include <QListWidgetItem>

class ResultsWidget : public QWidget
{
    // Q_OBJECT убран — нет сигналов/слотов

public:
    explicit ResultsWidget(QWidget *parent = nullptr);

    void addResult(const QString &title, const QString &author,
                   const QString &genre, const QString &content,
                   float score, const QString &source);
    void clear();
    void setFontSize(int size);

private:
    QListWidget *list_widget_ = nullptr;
    int font_size_ = 20;

    void applyFontSize();
};