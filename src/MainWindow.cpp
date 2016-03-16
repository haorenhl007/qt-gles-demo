#include "MainWindow.h"

//=============================================================================
MainWindow::MainWindow()
{
    ui.setupUi( this );
    ui.buttonInstall->click();
    ui.glWidget->setModel(":/chicken.ply");

    connect(ui.checkFaceCulling, &QCheckBox::toggled,
            ui.glWidget, &GlWidget::enableFaceCulling);
    connect(ui.checkDepthTesting, &QCheckBox::toggled,
            ui.glWidget, &GlWidget::enableDepthTesting);
    connect(ui.checkFaceNormals, &QCheckBox::toggled,
            ui.glWidget, &GlWidget::enableFacetedRender);

    ui.glWidget->enableFaceCulling(ui.checkFaceCulling->isChecked());
    ui.glWidget->enableDepthTesting(ui.checkDepthTesting->isChecked());
    ui.glWidget->enableFacetedRender(ui.checkFaceNormals->isChecked());
}

//=============================================================================
MainWindow::~MainWindow()
{
}

//=============================================================================
void MainWindow::on_buttonInstall_clicked()
{
    const QString vertexSource = ui.textVertex->toPlainText();
    const QString fragmentSource = ui.textFragment->toPlainText();
    ui.glWidget->installShaders(vertexSource, fragmentSource);
}

//=============================================================================
void MainWindow::on_sliderModelAngle_valueChanged(int degrees)
{
    ui.glWidget->setModelAngle(degrees);
}

//=============================================================================
void MainWindow::on_glWidget_notify(const QString& text)
{
    ui.textResult->append(text);
}
