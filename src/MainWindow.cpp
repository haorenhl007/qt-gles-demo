#include "MainWindow.h"

MainWindow::MainWindow()
{
    ui.setupUi( this );
    ui.buttonInstall->click();
    ui.glWidget->setModel(":/chicken.ply");
}

MainWindow::~MainWindow()
{
}

void MainWindow::on_buttonInstall_clicked()
{
    const QString vertexSource = ui.textVertex->toPlainText();
    const QString fragmentSource = ui.textFragment->toPlainText();
    ui.glWidget->installShaders(vertexSource, fragmentSource);
}

void MainWindow::on_glWidget_notify(const QString& text)
{
    ui.textResult->setPlainText(text);
}
