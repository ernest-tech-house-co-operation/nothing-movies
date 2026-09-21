#pragma once

#include <QWidget>
#include <QVariantMap>

class QStackedWidget;
class QButtonGroup;
class QPushButton;

class HomepageBridge;
class SearchBridge;
class AppController;
class QueueBridge;

class HomeWidget;
class SearchWidget;
class DownloadsWidget;
class SourcesWidget;
class SettingsWidget;
class ThanksWidget;
class InfoWidget;

class ShellWidget : public QWidget {
    Q_OBJECT
public:
    ShellWidget(HomepageBridge* homepage,
                SearchBridge* search,
                AppController* app,
                QueueBridge* queue,
                QWidget* parent = nullptr);

    void showThanks();

private slots:
    void onNavClicked(int index);
    void showInfo(const QVariantMap& info);
    void closeInfo();

private:
    HomepageBridge* homepage_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    QButtonGroup* navGroup_ = nullptr;

    HomeWidget* home_ = nullptr;
    SearchWidget* searchW_ = nullptr;
    DownloadsWidget* downloads_ = nullptr;
    SourcesWidget* sources_ = nullptr;
    ThanksWidget* thanks_ = nullptr;
    SettingsWidget* settings_ = nullptr;
    InfoWidget* info_ = nullptr;

    int infoIndex_ = -1;
    int prevIndex_ = 0;
};