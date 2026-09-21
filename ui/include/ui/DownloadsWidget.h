#pragma once

#include <QWidget>
#include <QVariantList>
#include <vector>

class QListWidget;
class QLabel;
class QueueBridge;
class AppController;

class DownloadsWidget : public QWidget {
    Q_OBJECT
public:
    DownloadsWidget(QueueBridge* queue, AppController* app,
                    QWidget* parent = nullptr);

private slots:
    void onItemsReady(const QVariantList& items);
    void onMetadataReady(const QString& id, const QVariantList& files);

private:
    QWidget* buildRow(const QVariantMap& item);

    QueueBridge* queue_ = nullptr;
    AppController* app_ = nullptr;
    QListWidget* list_ = nullptr;
    QLabel* emptyLabel_ = nullptr;
    QLabel* folderLabel_ = nullptr;
};