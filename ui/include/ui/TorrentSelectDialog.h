#pragma once

#include <QDialog>
#include <QVariantList>
#include <QString>

class QListWidget;
class QPushButton;

class TorrentSelectDialog : public QDialog {
    Q_OBJECT
public:
    explicit TorrentSelectDialog(QWidget* parent = nullptr);

    void setOptions(const QVariantList& options);
    QString selectedMagnetUrl() const;

private:
    QListWidget* list_ = nullptr;
    QPushButton* selectButton_ = nullptr;
};
