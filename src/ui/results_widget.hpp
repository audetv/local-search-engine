#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>

class ResultsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ResultsWidget(QWidget *parent = nullptr);

    void addResult(const QString &title, const QString &author,
                   const QString &genre, const QString &content,
                   float score, const QString &source);
    void clear();
    void setFontSize(int size);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QScrollArea *scroll_area_;
    QWidget *content_widget_;
    QVBoxLayout *layout_;
    int font_size_ = 16;

    QWidget *createResultItem(const QString &title, const QString &author,
                              const QString &genre, const QString &content,
                              float score, const QString &source);
    void updateAllItems();
};