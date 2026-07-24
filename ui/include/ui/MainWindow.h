#pragma once
#include <QMainWindow>
#include <memory>
#include "queue_manager/queue_manager.h"

class QStackedWidget;
class QQuickView;

namespace ui {

class TmdbBridge;
class SearchBridge;
class AppController;
class PlayerPageWidget;
class QueueBridge;

class MainWindow : public QMainWindow {
Q_OBJECT
public:
    explicit MainWindow(TmdbBridge* tmdbBridge, SearchBridge* searchBridge, AppController* appController,
                         QueueBridge* queueBridge,
                         std::shared_ptr<queue_manager::Queue_managerModule> queueManager,
                         QWidget* parent = nullptr);

private:
    // Both pages are backed by a genuine native/foreign window (the QML
    // page's QQuickView, the player page's mpv QWindow) rather than plain
    // Qt-painted content. QStackedWidget's hide()/show() alone doesn't
    // reliably reorder two foreign native windows at the OS compositor
    // level, which is what let both pages be visible at once for a moment.
    // switchToShell()/switchToPlayer() explicitly raise()/lower() the
    // native windows themselves alongside the QStackedWidget switch.
    void switchToShell();
    void switchToPlayer();

    QStackedWidget* stack_ = nullptr;
    PlayerPageWidget* playerPage_ = nullptr;
    QQuickView* quickView_ = nullptr;
};

}
