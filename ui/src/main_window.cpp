#include "ui/MainWindow.h"

#include <QQuickView>
#include <QQmlEngine>
#include <QUrl>
#include <QWidget>

// Static libs strip unreferenced resource initializers -- force it to run.
inline void initUiResources() {
    Q_INIT_RESOURCE(resources);
}

namespace ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    initUiResources();

    setWindowTitle("Nothing Movies");
    resize(1280, 800);

    // The whole app UI is QML (Main.qml -> splash -> shell -> screens).
    // QQuickView + createWindowContainer keeps this a QMainWindow (so
    // native window chrome, menus, etc. stay available later) while the
    // actual UI content is QML underneath.
    auto* quickView = new QQuickView();
    quickView->setResizeMode(QQuickView::SizeRootObjectToView);
    quickView->setSource(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    QWidget* container = QWidget::createWindowContainer(quickView, this);
    container->setMinimumSize(800, 600);
    container->setFocusPolicy(Qt::TabFocus);

    setCentralWidget(container);
}

} // namespace ui