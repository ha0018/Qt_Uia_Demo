#include "UiAutomationHelper.h"
#include <windows.h>
#include <uiautomation.h>
#include <comdef.h>

UiAutomationHelper::UiAutomationHelper() : m_pAutomation(nullptr) 
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
        IID_IUIAutomation, (void**)&m_pAutomation);
}

UiAutomationHelper::~UiAutomationHelper() 
{
    if (m_pAutomation) m_pAutomation->Release();
    CoUninitialize();
}

QVector<ControlInfo> UiAutomationHelper::getAllWindowsTree() {
    QVector<ControlInfo> result;
    if (!m_pAutomation) return result;

    IUIAutomationElement* pRoot = nullptr;
    m_pAutomation->GetRootElement(&pRoot); // 获取桌面根节点

    if (pRoot) {
        // 使用之前写的 getChildrenRecursive，
        // 但我们要把 root 作为一个虚拟父节点，抓取它的所有子节点（即所有窗口）
        ControlInfo desktopRoot;
        desktopRoot.name = "Desktop";

        // 这里的 getChildrenRecursive 会填充 desktopRoot.children
        // 也就是所有的顶级窗口
        getChildrenRecursive(pRoot, desktopRoot);

        result = desktopRoot.children;
        pRoot->Release();
    }

    return result;
}

QVector<ControlInfo> UiAutomationHelper::getControlsByWindowName(const QString& windowTitle) {
    QVector<ControlInfo> result;
    if (!m_pAutomation) return result;

    IUIAutomationElement* pRoot = nullptr;
    m_pAutomation->GetRootElement(&pRoot);

    VARIANT varProp;
    varProp.vt = VT_BSTR;
    varProp.bstrVal = SysAllocString((BSTR)windowTitle.utf16());

    IUIAutomationCondition* pCondition = nullptr;
    m_pAutomation->CreatePropertyCondition(UIA_NamePropertyId, varProp, &pCondition);

    IUIAutomationElement* pFound = nullptr;
    // 在根节点的子节点中找窗口
    pRoot->FindFirst(TreeScope_Children, pCondition, &pFound);

    if (pFound) {
        // 1. 创建窗口自身的节点信息
        ControlInfo windowRoot;
        BSTR name, autoId, className, helpText, typeName;
        BOOL bEnabled, bFocusable;
        RECT rect;

        // --- 获取常用属性 ---
        pFound->get_CurrentName(&name);
        pFound->get_CurrentAutomationId(&autoId);
        pFound->get_CurrentClassName(&className); // 获取类名 (如 "Qt662QWindowIcon")
        pFound->get_CurrentLocalizedControlType(&typeName);   // 获取类型 ID
        pFound->get_CurrentHelpText(&helpText);   // 获取帮助文本
        pFound->get_CurrentIsEnabled(&bEnabled);  // 是否可用
        pFound->get_CurrentIsKeyboardFocusable(&bFocusable); // 是否可聚焦
        pFound->get_CurrentBoundingRectangle(&rect); // 获取坐标

        // --- 赋值给结构体 ---
        windowRoot.name = QString::fromWCharArray(name ? name : L"");
        windowRoot.automationId = QString::fromWCharArray(autoId ? autoId : L"");
        windowRoot.className = QString::fromWCharArray(className ? className : L"");
        windowRoot.helpText = QString::fromWCharArray(helpText ? helpText : L"");
        windowRoot.type = QString::fromWCharArray(typeName ? typeName : L"");
        windowRoot.enabled = QString::fromWCharArray(bEnabled ? L"True" : L"False"); 
        windowRoot.focus = QString::fromWCharArray(bFocusable ? L"True" : L"False"); 
        windowRoot.rect = QString("(%1, %2, %3, %4)").arg(rect.left).arg(rect.top).arg(rect.right - rect.left).arg(rect.bottom - rect.top);
        // --- 释放内存 (COM 要求必须手动释放 BSTR) ---
        if (name) SysFreeString(name);
        if (autoId) SysFreeString(autoId);
        if (className) SysFreeString(className);
        if (helpText) SysFreeString(helpText);
        if (typeName) SysFreeString(typeName);

        // 2. 递归获取该窗口下的所有子孙节点，存入 windowRoot.children 中
        // 注意：这里的第二个参数现在是 windowRoot 对象，而不是 result 向量
        getChildrenRecursive(pFound, windowRoot);

        // 3. 将构建好的整棵“树”加入结果向量
        result.append(windowRoot);

        pFound->Release();
    }

    if (pCondition) pCondition->Release();
    SysFreeString(varProp.bstrVal);
    if (pRoot) pRoot->Release();

    return result;
}

// UiAutomationHelper.cpp 里的相关部分
void UiAutomationHelper::getChildrenRecursive(IUIAutomationElement* element, ControlInfo& parentInfo) {

    IUIAutomationElementArray* children = nullptr;
    IUIAutomationCondition* pTrueCondition = nullptr;
    m_pAutomation->CreateTrueCondition(&pTrueCondition); // 创建一个总是为 True 的条件

    // 使用 pTrueCondition 代替 nullptr
    element->FindAll(TreeScope_Children, pTrueCondition, &children);

    if (pTrueCondition) pTrueCondition->Release();

    if (children) {
        int length = 0;
        children->get_Length(&length);
        for (int i = 0; i < length; ++i) {
            IUIAutomationElement* child = nullptr;
            children->GetElement(i, &child);
            if (child) {
                BSTR name, autoId, className, helpText, typeName;
                BOOL bEnabled, bFocusable;
                RECT rect;

                // --- 获取常用属性 ---
                child->get_CurrentName(&name);
                child->get_CurrentAutomationId(&autoId);
                child->get_CurrentClassName(&className); // 获取类名 (如 "Qt662QWindowIcon")
                child->get_CurrentLocalizedControlType(&typeName);   // 获取类型 ID
                child->get_CurrentHelpText(&helpText);   // 获取帮助文本
                child->get_CurrentIsEnabled(&bEnabled);  // 是否可用
                child->get_CurrentIsKeyboardFocusable(&bFocusable); // 是否可聚焦
                child->get_CurrentBoundingRectangle(&rect); // 获取坐标

                ControlInfo childInfo;
                // --- 赋值给结构体 ---
                childInfo.name = QString::fromWCharArray(name ? name : L"");
                childInfo.automationId = QString::fromWCharArray(autoId ? autoId : L"");
                childInfo.className = QString::fromWCharArray(className ? className : L"");
                childInfo.helpText = QString::fromWCharArray(helpText ? helpText : L"");
                childInfo.type = QString::fromWCharArray(typeName ? typeName : L"");
                childInfo.enabled = QString::fromWCharArray(bEnabled ? L"True" : L"False");
                childInfo.focus = QString::fromWCharArray(bFocusable ? L"True" : L"False"); 
                childInfo.rect = QString("(%1, %2, %3, %4)").arg(rect.left).arg(rect.top).arg(rect.right - rect.left).arg(rect.bottom - rect.top);
                
                // 递归：继续往下找当前节点的子节点
                getChildrenRecursive(child, childInfo);

                // 将填满子节点的 childInfo 加入父节点
                parentInfo.children.append(childInfo);

                // --- 释放内存 (COM 要求必须手动释放 BSTR) ---
                if (name) SysFreeString(name);
                if (autoId) SysFreeString(autoId);
                if (className) SysFreeString(className);
                if (helpText) SysFreeString(helpText);
                if (typeName) SysFreeString(typeName);
                child->Release();
            }
        }
        children->Release();
    }
}

ControlInfo UiAutomationHelper::getTreeFromElement(IUIAutomationElement* element) 
{
    ControlInfo rootInfo;
    if (!element) return rootInfo;

    // 获取当前传入元素的基本信息
    BSTR name, autoId, typeName;
    element->get_CurrentName(&name);
    element->get_CurrentAutomationId(&autoId);
    element->get_CurrentLocalizedControlType(&typeName);

    rootInfo.name = QString::fromWCharArray(name ? name : L"");
    rootInfo.automationId = QString::fromWCharArray(autoId ? autoId : L"");
    rootInfo.type = QString::fromWCharArray(typeName ? typeName : L"");

    if (name) SysFreeString(name);
    if (autoId) SysFreeString(autoId);
    if (typeName) SysFreeString(typeName);

    // 开启递归抓取子节点
    getChildrenRecursive(element, rootInfo);

    return rootInfo;
}

IUIAutomationElement* UiAutomationHelper::findElementByIdAndName(const QString& autoId, const QString& name) {
    if (!m_pAutomation) return nullptr;

    IUIAutomationElement* pRoot = nullptr;
    m_pAutomation->GetRootElement(&pRoot);

    // 创建复合搜索条件：ID 或 Name
    IUIAutomationCondition* pIdCond = nullptr, * pNameCond = nullptr, * pOrCond = nullptr;

    VARIANT vId, vName;
    vId.vt = VT_BSTR; vId.bstrVal = SysAllocString((BSTR)autoId.utf16());
    vName.vt = VT_BSTR; vName.bstrVal = SysAllocString((BSTR)name.utf16());

    m_pAutomation->CreatePropertyCondition(UIA_AutomationIdPropertyId, vId, &pIdCond);
    m_pAutomation->CreatePropertyCondition(UIA_NamePropertyId, vName, &pNameCond);
    m_pAutomation->CreateOrCondition(pIdCond, pNameCond, &pOrCond);

    IUIAutomationElement* pFound = nullptr;
    // 在全系统范围内找（TreeScope_Descendants），或者在你记录的窗口句柄下找
    pRoot->FindFirst(TreeScope_Descendants, pOrCond, &pFound);

    // 释放资源
    SysFreeString(vId.bstrVal); SysFreeString(vName.bstrVal);
    if (pIdCond) pIdCond->Release();
    if (pNameCond) pNameCond->Release();
    if (pOrCond) pOrCond->Release();
    if (pRoot) pRoot->Release();

    return pFound;
}

