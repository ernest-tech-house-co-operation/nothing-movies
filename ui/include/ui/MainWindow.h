#pragma once
#include <QMainWindow>

namespace ui {
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
};
}//namespace ui/include/ui/mainwindow