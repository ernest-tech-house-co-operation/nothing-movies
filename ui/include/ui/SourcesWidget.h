#pragma once

#include <QWidget>

class QTableWidget;
class QVBoxLayout;
class HomepageBridge;

class SourcesWidget : public QWidget {
    Q_OBJECT
public:
    explicit SourcesWidget(HomepageBridge* bridge, QWidget* parent = nullptr);

private:
    void reload();

    HomepageBridge* bridge_ = nullptr;
    QVBoxLayout* cardsLayout_ = nullptr;
};