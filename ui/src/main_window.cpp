#include "ui/MainWindow.h"
#include "ui/SplashWidget.h"
#include "ui/ShellWidget.h"
#include "ui/PlayerPageWidget.h"
#include "ui/StreamEmbedWidget.h"
#include "ui/AppController.h"
#include "ui/SearchBridge.h"
#include "ui/QueueBridge.h"
#include "ui/HomepageBridge.h"

#include <QStackedWidget>

MainWindow::MainWindow(SearchBridge* search,
                       AppController* app,
                       QueueBridge* queue,
                       HomepageBridge* homepage,
                       std::shared_ptr<queue_manager::Queue_managerModule> queueManager,
                       QWidget* parent)
    : QMainWindow(parent), app_(app)
{
    setWindowTitle("Nothing Movies");
    resize(1280, 800);

    stack_ = new QStackedWidget(this);
    setCentralWidget(stack_);

    splash_ = new SplashWidget(stack_);
    shell_  = new ShellWidget(homepage, search, app, queue, stack_);
    player_ = new PlayerPageWidget(queueManager, stack_);
    embed_  = new StreamEmbedWidget(stack_);

    stack_->addWidget(splash_);   // 0
    stack_->addWidget(shell_);    // 1
    stack_->addWidget(player_);   // 2
    stack_->addWidget(embed_);    // 3
    stack_->setCurrentIndex(0);

    connect(splash_, &SplashWidget::finished, this, [this]() {
        stack_->setCurrentIndex(1);
    });

    if (app_) {
        connect(app_, &AppController::streamRequested, this,
                [this](const QString& title, const QString& url, const QString& streamType) {
                    if (streamType == "embed") {
                        embed_->loadUrl(url);
                        stack_->setCurrentIndex(3);
                    } else {
                        stack_->setCurrentIndex(2);
                        player_->startStream(title, url);
                    }
                });
        connect(app_, &AppController::stopRequested, this, [this]() {
            stack_->setCurrentIndex(1);
        });
    }

    connect(embed_, &StreamEmbedWidget::backRequested, this, [this]() {
        stack_->setCurrentIndex(1);
    });

    connect(player_, &PlayerPageWidget::backRequested, this, [this]() {
        stack_->setCurrentIndex(1);
    });
}