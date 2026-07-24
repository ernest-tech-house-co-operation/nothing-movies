#include "ui/MainWindow.h"
#include "ui/TmdbBridge.h"
#include "ui/SearchBridge.h"
#include "ui/AppController.h"
#include "ui/PlayerPageWidget.h"
#include "ui/QueueBridge.h"

#include <QQuickView>
#include <QQmlEngine>
#include <QWindow>
#include <QQmlContext>
#include <QUrl>
#include <QWidget>
#include <QStackedWidget>

// Static libs strip unreferenced resource initializers -- force it to run.
inline void initUiResources() {
    Q_INIT_RESOURCE(resources);
}

namespace ui {

MainWindow::MainWindow(TmdbBridge* tmdbBridge, SearchBridge* searchBridge, AppController* appController,
                        QueueBridge* queueBridge,
                        std::shared_ptr<queue_manager::Queue_managerModule> queueManager, QWidget* parent)
    : QMainWindow(parent) {
    initUiResources();

    setWindowTitle("Nothing Movies");
    resize(1280, 800);

    // Page 0: the existing QML app, completely unchanged.
    quickView_ = new QQuickView();
    quickView_->setResizeMode(QQuickView::SizeRootObjectToView);
    quickView_->rootContext()->setContextProperty("tmdbBridge", tmdbBridge);
    quickView_->rootContext()->setContextProperty("searchBridge", searchBridge);
    quickView_->rootContext()->setContextProperty("appController", appController);
    quickView_->rootContext()->setContextProperty("queueBridge", queueBridge);
    quickView_->setSource(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    QWidget* qmlContainer = QWidget::createWindowContainer(quickView_, this);
    qmlContainer->setMinimumSize(800, 600);
    qmlContainer->setFocusPolicy(Qt::TabFocus);

    // Page 1: plain QWidget holding a raw native QWindow that mpv renders
    // into -- a separate native surface from the QQuickView above, not a
    // hole punched into it.
    playerPage_ = new PlayerPageWidget(std::move(queueManager), this);

    stack_ = new QStackedWidget(this);
    stack_->addWidget(qmlContainer);   // index 0
    stack_->addWidget(playerPage_);    // index 1
    setCentralWidget(stack_);

    connect(appController, &AppController::streamRequested, this,
            [this](const QString& title, const QString& url) {
                playerPage_->startStream(title, url);
                switchToPlayer();
            });
    connect(appController, &AppController::stopRequested, this, [this]() {
        playerPage_->stopStream();
        switchToShell();
    });
    connect(playerPage_, &PlayerPageWidget::backRequested, this, [this]() {
        switchToShell();
    });
    connect(playerPage_, &PlayerPageWidget::pipActiveChanged, this, [this](bool active) {
        // Entering PiP: drop back to the normal app so the user can browse
        // while the floating mini player keeps playing. Exiting: bring the
        // full player page back into view. The PiP overlay itself lives
        // outside stack_ (see PlayerPageWidget::enterPip), so it stays on
        // screen through either switch.
        if (active) {
            switchToShell();
        } else {
            switchToPlayer();
        }
    });
}

void MainWindow::switchToShell() {
    // videoWidget_ is a normal QWidget now (not a foreign native window),
    // so QStackedWidget's own paging is enough -- no manual raise/lower of
    // a separate native surface required, and nothing to leak through
    // visually between pages.
    stack_->setCurrentIndex(0);
}

void MainWindow::switchToPlayer() {
    stack_->setCurrentIndex(1);
}

}
