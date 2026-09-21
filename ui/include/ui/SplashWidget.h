#pragma once

#include <QWidget>

class QTimer;
class QLabel;
class QResizeEvent;

class SplashWidget : public QWidget {
    Q_OBJECT
public:
    explicit SplashWidget(QWidget* parent = nullptr);

signals:
    void finished();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QLabel* background_ = nullptr;
    QWidget* overlay_ = nullptr;
    QTimer* timer_;
};