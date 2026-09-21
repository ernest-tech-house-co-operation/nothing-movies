#pragma once
#include <QWidget>
#include <QString>

class QPushButton;
class QLabel;
class QWebEngineView;

class StreamEmbedWidget : public QWidget {
    Q_OBJECT
public:
    explicit StreamEmbedWidget(QWidget* parent = nullptr);

    void loadUrl(const QString& url);
    void stop();

signals:
    void backRequested();

private:
    QWebEngineView* view_ = nullptr;
    QPushButton* backBtn_ = nullptr;
    QLabel* titleLabel_ = nullptr;
};
