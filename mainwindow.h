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
    void on_nameTree_clicked(const QModelIndex& index);
    void updateAttrTreeWithRealTimeElement(IUIAutomationElement* pElement);
private:
    Ui::QtUiAutomationClass* myUi;
    UiAutomationHelper m_helper;
};