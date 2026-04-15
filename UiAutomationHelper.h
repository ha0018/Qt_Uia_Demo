#pragma once
#include <QVector>
#include "ControlInfo.h"

// «∞œÚ…˘√˜£¨∑¿÷π Windows.h Œ€»æ
struct IUIAutomation;
struct IUIAutomationElement;

class UiAutomationHelper {
public:
    UiAutomationHelper();
    ~UiAutomationHelper();

    QVector<ControlInfo> getAllWindowsTree();
    QVector<ControlInfo> getControlsByWindowName(const QString& windowTitle);
    ControlInfo getTreeFromElement(IUIAutomationElement* element);
    IUIAutomationElement* findElementByIdAndName(const QString& autoId, const QString& name);
private:
    IUIAutomation* m_pAutomation;

    void getChildrenRecursive(IUIAutomationElement * element, ControlInfo & parentInfo);
};