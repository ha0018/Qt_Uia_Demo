#pragma once
#include <QtWidgets/QMainWindow>
#include <windows.h>
#include <QTimer>
#include "UiAutomationHelper.h"
#include "OverlayWidget.h"

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
    void on_refreshButton_clicked();
    void on_nameTree_clicked(const QModelIndex& index);
    void updateAttrTreeWithRealTimeElement(IUIAutomationElement* pElement, int x, int y);
    IUIAutomationElement* findDeepestElementAtPoint(IUIAutomationElement* pParent, POINT pt);
private:
    OverlayWidget* m_overlay = nullptr;
    QTimer* m_trackTimer = nullptr;
    bool m_isSelecting = false;
    Ui::QtUiAutomationClass* myUi;
    UiAutomationHelper m_helper;
};