#pragma once

#include <QWidget>

#include "ui_MainWindow.h"

class MainWindow : public QWidget
{
    Q_OBJECT;

public:
    MainWindow();
    ~MainWindow() override;

private slots:
    void on_buttonInstall_clicked();
    void on_sliderModelAngle_valueChanged(int degrees);
    void on_glWidget_notify(const QString& text);

private:
    Ui::MainWindow ui;
};
