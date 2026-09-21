#pragma once

#include <QWidget>
#include <QVariantList>
#include <QVariantMap>

class QGridLayout;
class QLabel;
class QScrollArea;
class QResizeEvent;
class HomepageBridge;

class HomeWidget : public QWidget {
    Q_OBJECT
public:
    explicit HomeWidget(HomepageBridge* bridge, QWidget* parent = nullptr);
    ~HomeWidget() override;

signals:
    void infoRequested(const QVariantMap& info);

private slots:
    void onHomepageReady(const QVariantList& items,
                         const QString& sourceName,
                         bool isFallback);
    void onInfoReady(const QVariantMap& info);
    void onHomepageError(const QString& message);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUi();
    void rebuildGrid();
    int calculateColumns() const;
    void clearGrid();

    HomepageBridge* bridge_ = nullptr;
    QGridLayout*    grid_   = nullptr;
    QScrollArea*    scroll_ = nullptr;
    QWidget*        content_ = nullptr;
    QLabel* sectionTitle_ = nullptr;
    QLabel* statusLabel_  = nullptr;

    QVariantList currentItems_;
    QString      currentSource_;
    bool         isFallback_ = false;
};