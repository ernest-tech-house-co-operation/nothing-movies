#pragma once

#include <QMainWindow>
#include <memory>

class QStackedWidget;
class SplashWidget;
class ShellWidget;
class PlayerPageWidget;
class StreamEmbedWidget;
class SearchBridge;
class AppController;
class QueueBridge;
class HomepageBridge;

namespace queue_manager { class Queue_managerModule; }

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(SearchBridge* search,
               AppController* app,
               QueueBridge* queue,
               HomepageBridge* homepage,
               std::shared_ptr<queue_manager::Queue_managerModule> queueManager,
               QWidget* parent = nullptr);

private:
    QStackedWidget*   stack_  = nullptr;
    SplashWidget*     splash_ = nullptr;
    ShellWidget*      shell_  = nullptr;
    PlayerPageWidget* player_ = nullptr;
    StreamEmbedWidget* embed_ = nullptr;
    AppController*    app_    = nullptr;
};