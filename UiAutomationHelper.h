#pragma once
#include <QVector>
#include "ControlInfo.h"

// 前向声明，防止 Windows.h 污染
struct IUIAutomation;
struct IUIAutomationElement;

class UiAutomationHelper {
public:
    UiAutomationHelper();
    ~UiAutomationHelper();

    // 新增：暴露内部的 pAutomation 指针
    IUIAutomation* getAutomationObject() const { return m_pAutomation; }

    QVector<ControlInfo> getAllWindowsTree();
    QVector<NameInfo> getAllNameTree();
    QVector<ControlInfo> getControlsByWindowName(const QString& windowTitle);
    ControlInfo getTreeFromElement(IUIAutomationElement* element);
    IUIAutomationElement* findElementByIdAndName(const QString& autoId, const QString& name);
private:
    IUIAutomation* m_pAutomation;

    void getChildrenRecursive(IUIAutomationElement * element, ControlInfo & parentInfo);
    void getChildrenName(IUIAutomationElement* element, NameInfo& parentInfo);
};