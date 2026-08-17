#pragma once
#include <QMainWindow>
#include <memory>
#include <QQuickView>
#include <QStackedWidget>
namespace queue_manager {
class Queue_managerModule;
}
namespace ui {
class TmdbBridge;
class SearchBridge;
class HomepageBridge;
class AppController;
class QueueBridge;
class VendorBridge;   // NEW
class PlayerPageWidget;
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(TmdbBridge* tmdbBridge,
                        SearchBridge* searchBridge,
                        AppController* appController,
                        QueueBridge* queueBridge,
                        std::shared_ptr<queue_manager::Queue_managerModule> queueManager,
                        HomepageBridge* homepageBridge = nullptr,
                        VendorBridge* vendorBridge = nullptr,   // NEW
                        QWidget* parent = nullptr);
private slots:
    void switchToShell();
    void switchToPlayer();
private:
    QQuickView* quickView_ = nullptr;
    PlayerPageWidget* playerPage_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    HomepageBridge* homepageBridge_ = nullptr;   // (optional, used only for context property)
    VendorBridge* vendorBridge_ = nullptr;        // NEW (optional, used only for context property)
};
} // namespace ui