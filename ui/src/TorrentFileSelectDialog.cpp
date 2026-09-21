#include "ui/TorrentFileSelectDialog.h"
#include "ui/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

TorrentFileSelectDialog::TorrentFileSelectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Select Files to Download");
    setMinimumSize(520, 400);
    setStyleSheet(QString("QDialog { background-color: %1; }").arg(NM_BG));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel("Select files to download:", this);
    title->setStyleSheet("color: white; font-size: 15px; font-weight: bold; background: transparent;");
    root->addWidget(title);

    auto* hint = new QLabel("Check the files you want. Unchecked files will be skipped.", this);
    hint->setStyleSheet(QString("color: %1; font-size: 12px; background: transparent;").arg(NM_SUBTEXT));
    root->addWidget(hint);

    list_ = new QListWidget(this);
    list_->setSelectionMode(QAbstractItemView::NoSelection);
    list_->setStyleSheet(
        "QListWidget { background-color: #13131f; border: 1px solid #1e1e30; border-radius: 8px; }"
        "QListWidget::item { color: #e5e7eb; padding: 10px; border-bottom: 1px solid #1e1e30; }"
        "QListWidget::item:hover { background-color: #211a3d; }");
    root->addWidget(list_, 1);

    statusLabel_ = new QLabel(this);
    statusLabel_->setStyleSheet(QString("color: %1; font-size: 12px; background: transparent;").arg(NM_SUBTEXT));
    root->addWidget(statusLabel_);

    auto* buttonRow = new QHBoxLayout();
    buttonRow->addStretch();

    auto* cancelButton = new QPushButton("Cancel", this);
    cancelButton->setFixedSize(100, 38);
    cancelButton->setStyleSheet(
        "QPushButton { background-color: #1a1a2e; color: white; border: 1px solid #2a2a3e;"
        "  border-radius: 8px; font-size: 13px; }"
        "QPushButton:hover { background-color: #2d2d4e; }");
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonRow->addWidget(cancelButton);

    downloadBtn_ = new QPushButton("Download Selected", this);
    downloadBtn_->setFixedSize(160, 38);
    downloadBtn_->setStyleSheet(
        "QPushButton { background-color: #7c3aed; color: white; font-weight: bold;"
        "  border: none; border-radius: 8px; font-size: 13px; }"
        "QPushButton:hover { background-color: #6d28d9; }");
    connect(downloadBtn_, &QPushButton::clicked, this, &QDialog::accept);
    buttonRow->addWidget(downloadBtn_);

    root->addLayout(buttonRow);
}

void TorrentFileSelectDialog::setFiles(
    const std::vector<std::pair<int, std::string>>& files) {
    list_->clear();
    for (const auto& [index, path] : files) {
        auto* item = new QListWidgetItem(QString::fromStdString(path), list_);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        item->setData(Qt::UserRole, index);
    }
    statusLabel_->setText(QString("%1 file(s) found").arg(files.size()));
}

std::vector<int> TorrentFileSelectDialog::selectedIndices() const {
    std::vector<int> result;
    for (int i = 0; i < list_->count(); ++i) {
        auto* item = list_->item(i);
        if (item->checkState() == Qt::Checked)
            result.push_back(item->data(Qt::UserRole).toInt());
    }
    return result;
}
