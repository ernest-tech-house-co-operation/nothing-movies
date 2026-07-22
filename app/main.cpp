#include <QApplication>
#include <iostream>
#include "ui/MainWindow.h"
#include "vendor_updater/VendorUpdater.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    std::cout << "Nothing Movies - We have what? Everything.\n";

#if defined(_WIN32)
    static const std::string kPlatformTag = "windows";
#else
    static const std::string kPlatformTag = "linux";
#endif

    static vendor_updater::VendorUpdater updater(
        "BunElysiaReact/nothing-browser",
        "vendor/nothing-browser",
        kPlatformTag
    );

    updater.startBackgroundWatch(6 * 3600, [](vendor_updater::UpdateResult r) {
        if (!r.error.empty()) {
            std::cerr << "[vendor_updater] error: " << r.error << "\n";
        } else if (r.updated) {
            std::cout << "[vendor_updater] updated " << r.oldTag << " -> " << r.newTag << "\n";
        }
    });

    ui::MainWindow window;
    window.show();
    return app.exec();
}
