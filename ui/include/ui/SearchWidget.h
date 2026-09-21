#pragma once

#include <QWidget>
#include <QVariantList>
#include <QVariantMap>

class QLineEdit;
class QPushButton;
class QListWidget;
class QListWidgetItem;
class QLabel;
class SearchBridge;
class HomepageBridge;

class SearchWidget : public QWidget {
    Q_OBJECT
public:
    SearchWidget(SearchBridge* search, HomepageBridge* homepage,
                 QWidget* parent = nullptr);

signals:
    void resultSelected(const QVariantMap& result);

private slots:
    void doSearch();
    void onResultsReady(const QVariantList& results);
    void onSearchError(const QString& message);
    void onItemClicked(QListWidgetItem* item);

private:
    SearchBridge* search_ = nullptr;
    HomepageBridge* homepage_ = nullptr;
    QLineEdit* query_ = nullptr;
    QPushButton* goBtn_ = nullptr;
    QPushButton* movieBtn_ = nullptr;
    QPushButton* tvBtn_ = nullptr;
    bool searchingTV_ = false;
    QListWidget* list_ = nullptr;
    QLabel* status_ = nullptr;
};