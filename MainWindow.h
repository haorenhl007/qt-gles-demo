#pragma once

#include <QDialog>

#include "ui_MainWindow.h"

class MainWindow : public QDialog {
    Q_OBJECT;

public:
    MainWindow();
    ~MainWindow() override;

private:
    Ui::MainWindow ui;
};
