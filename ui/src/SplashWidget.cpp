#include "ui/SplashWidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>
#include <QTimer>

SplashWidget::SplashWidget(QWidget* parent) : QWidget(parent) {
    setObjectName("SplashWidget");
    setStyleSheet("QWidget#SplashWidget { background-color: #0d0d0d; }");

    background_ = new QLabel(this);
    background_->setPixmap(QPixmap(":/resources/splash_background.jpeg"));
    background_->setScaledContents(true);
    background_->lower();

    overlay_ = new QWidget(this);
    overlay_->setStyleSheet("background-color: rgba(8, 6, 24, 150);");
    overlay_->lower();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);

    auto* title = new QLabel("Nothing Movies", this);
    title->setPixmap(QPixmap(":/resources/logo.png").scaled(
        280, 280, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "color: white; font-size: 32px; font-weight: bold; background: transparent;");
    layout->addWidget(title);

    timer_ = new QTimer(this);
    timer_->setSingleShot(true);
    timer_->setInterval(1200);
    connect(timer_, &QTimer::timeout, this, &SplashWidget::finished);
    timer_->start();
}

void SplashWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (background_) background_->setGeometry(rect());
    if (overlay_) overlay_->setGeometry(rect());
}