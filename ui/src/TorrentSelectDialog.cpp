#include "ui/TorrentSelectDialog.h"
#include "ui/Theme.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

TorrentSelectDialog::TorrentSelectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Select Torrent");
    setMinimumSize(620, 420);
    setStyleSheet(QString("QDialog { background-color: %1; }").arg(NM_BG));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(12);

    auto* title = new QLabel("Choose a torrent:", this);
    title->setStyleSheet("color: white; font-size: 15px; font-weight: bold; background: transparent;");
    root->addWidget(title);

    list_ = new QListWidget(this);
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->setStyleSheet(
        "QListWidget { background-color: #13131f; border: 1px solid #1e1e30; border-radius: 8px; }"
        "QListWidget::item { color: #e5e7eb; padding: 12px; border-bottom: 1px solid #1e1e30; }"
        "QListWidget::item:hover { background-color: #211a3d; }"
        "QListWidget::item:selected { background-color: #7c3aed; }");
    root->addWidget(list_, 1);

    auto* buttons = new QHBoxLayout();
    buttons->addStretch();

    auto* cancel = new QPushButton("Cancel", this);
    cancel->setFixedSize(100, 38);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    buttons->addWidget(cancel);

    selectButton_ = new QPushButton("Select", this);
    selectButton_->setFixedSize(100, 38);
    selectButton_->setEnabled(false);
    connect(selectButton_, &QPushButton::clicked, this, [this]() {
        if (list_->currentItem()) accept();
    });
    buttons->addWidget(selectButton_);
    root->addLayout(buttons);

    connect(list_, &QListWidget::currentRowChanged, this, [this](int row) {
        selectButton_->setEnabled(row >= 0);
    });
}

void TorrentSelectDialog::setOptions(const QVariantList& options) {
    list_->clear();
    for (const QVariant& value : options) {
        const QVariantMap option = value.toMap();
        const QString title = option.value("title").toString();
        const QString quality = option.value("quality").toString();
        const QString size = option.value("size").toString();
        const int seeds = option.value("seeds").toInt();
        const QString label = QString("%1    %2    %3    Seeds: %4")
            .arg(title.isEmpty() ? "Torrent" : title)
            .arg(quality.isEmpty() ? "Unknown quality" : quality)
            .arg(size.isEmpty() ? "Unknown size" : size)
            .arg(seeds);
        auto* item = new QListWidgetItem(label, list_);
        item->setData(Qt::UserRole, option.value("magnetUrl"));
    }
    if (list_->count() > 0) list_->setCurrentRow(0);
}

QString TorrentSelectDialog::selectedMagnetUrl() const {
    if (!list_->currentItem()) return {};
    return list_->currentItem()->data(Qt::UserRole).toString();
}
