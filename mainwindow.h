#pragma once
#include <QtWidgets/QMainWindow>
#include "UiAutomationHelper.h"

QT_BEGIN_NAMESPACE
namespace Ui { class QtUiAutomationClass; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void on_searchButton_clicked();
    void on_selectButton_clicked();

private:
    Ui::QtUiAutomationClass* myUi;
    UiAutomationHelper m_helper; // 这里可以直接用了，因为 Helper 头文件很干净

    void updateTreeViewWithSubTree(const ControlInfo& subTree);
};