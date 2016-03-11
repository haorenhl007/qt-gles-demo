#pragma once

#include <QWidget>

#include "ui_MainWindow.h"

class MainWindow : public QWidget {
    Q_OBJECT;

public:
    MainWindow();
    ~MainWindow() override;

private:
    Ui::MainWindow ui;
};
