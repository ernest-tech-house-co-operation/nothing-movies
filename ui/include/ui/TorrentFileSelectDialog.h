#pragma once

#include <QDialog>
#include <string>
#include <utility>
#include <vector>

class QListWidget;
class QPushButton;
class QLabel;

class TorrentFileSelectDialog : public QDialog {
    Q_OBJECT
public:
    explicit TorrentFileSelectDialog(QWidget* parent = nullptr);

    void setFiles(const std::vector<std::pair<int, std::string>>& files);
    std::vector<int> selectedIndices() const;

private:
    QListWidget* list_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPushButton* downloadBtn_ = nullptr;
};
