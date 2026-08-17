#include "ui/VendorBridge.h"
#include "vendor_updater/VendorManager.h"

#include <QMetaObject>
#include <QVariantMap>
#include <thread>

namespace ui {

VendorBridge::VendorBridge(QObject* parent) : QObject(parent) {}

QVariantList VendorBridge::listVendors() {
    return toVariantList();
}

QVariantList VendorBridge::toVariantList() const {
    QVariantList list;
    for (const auto& info : vendor_updater::VendorManager::instance().listVendors()) {
        QVariantMap m;
        m["name"]          = QString::fromStdString(info.name);
        m["sourceRepoUrl"] = QString::fromStdString(info.sourceRepoUrl);
        m["releaseRepo"]   = QString::fromStdString(info.releaseRepo);
        m["license"]       = QString::fromStdString(info.license);
        m["currentTag"]    = QString::fromStdString(info.currentTag.empty() ? "not installed" : info.currentTag);
        m["vendorDir"]     = QString::fromStdString(info.vendorDir);
        list.append(m);
    }
    return list;
}

void VendorBridge::checkVendor(const QString& name) {
    emit updateStatus(name, "checking");

    // Runs the network+extraction work off the UI thread; marshals the
    // result back via invokeMethod so the emit happens safely on this
    // object's own thread.
    std::thread([this, name]() {
        auto result = vendor_updater::VendorManager::instance().checkAndUpdate(name.toStdString());

        QMetaObject::invokeMethod(this, [this, result]() {
            emit updateFinished(
                QString::fromStdString(result.vendorName),
                result.updated,
                QString::fromStdString(result.newTag),
                QString::fromStdString(result.error)
            );
        }, Qt::QueuedConnection);
    }).detach();
}

void VendorBridge::checkAll() {
    auto vendors = vendor_updater::VendorManager::instance().listVendors();

    std::thread([this, vendors]() {
        for (const auto& info : vendors) {
            QString name = QString::fromStdString(info.name);
            QMetaObject::invokeMethod(this, [this, name]() {
                emit updateStatus(name, "checking");
            }, Qt::QueuedConnection);

            auto result = vendor_updater::VendorManager::instance().checkAndUpdate(info.name);

            QMetaObject::invokeMethod(this, [this, result]() {
                emit updateFinished(
                    QString::fromStdString(result.vendorName),
                    result.updated,
                    QString::fromStdString(result.newTag),
                    QString::fromStdString(result.error)
                );
            }, Qt::QueuedConnection);
        }
    }).detach();
}

} // namespace ui