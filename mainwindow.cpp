#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <uiautomation.h>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QScreen>
#include <QGuiApplication>

void addControlToModel(QStandardItem* parentItem, const ControlInfo& info)
{
    QList<QStandardItem*> rowItems;
    rowItems << new QStandardItem(info.name)
        << new QStandardItem(info.type)
        << new QStandardItem(info.automationId)
        << new QStandardItem(info.className)
        << new QStandardItem(info.helpText)
        << new QStandardItem(info.rect)
        << new QStandardItem(info.enabled)
        << new QStandardItem(info.focus);
    parentItem->appendRow(rowItems);
    for (const auto& child : info.children)
    {
        addControlToModel(rowItems[0], child);
    }
}

void addNameToModel(QStandardItem* parentItem, const NameInfo& info)
{
    QList<QStandardItem*> rowItems;
    rowItems << new QStandardItem(QString("\"%1\" %2").arg(info.name, info.localizedControlType));
    parentItem->appendRow(rowItems);
    for (const auto& child : info.children) 
    {
        addNameToModel(rowItems[0], child);
    }
}

QStringList getPathToRoot(QModelIndex index)
{
    QStringList path;
    while (index.isValid())
    {
        QString name = index.data(Qt::DisplayRole).toString();
        int last = name.lastIndexOf("\" ");
        name = name.mid(1, last - 1);
        path.prepend(name);
        index = index.parent();
    }
    return path;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), myUi(new Ui::QtUiAutomationClass)
{
    myUi->setupUi(this);
    // 设置点击信号到槽函数
    connect(myUi->nameTree, &QTreeView::clicked, this, &MainWindow::on_nameTree_clicked);
    myUi->nameTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    myUi->nameTree->header()->hide(); 
    auto allNameInfos = m_helper.getAllNameTree();
    QStandardItemModel* nameModel = new QStandardItemModel(this);
    for (const auto& nameInfo : allNameInfos)
    {
        if (nameInfo.name.isEmpty() && nameInfo.localizedControlType.isEmpty())
        {
            continue;
        }
        addNameToModel(nameModel->invisibleRootItem(), nameInfo);
    }
    myUi->nameTree->setModel(nameModel);
}

MainWindow::~MainWindow() 
{
    delete myUi;
}

void MainWindow::on_searchButton_clicked() 
{
    QString title = myUi->lineEdit->text();
    auto controls = m_helper.getControlsByWindowName(title);
    QStandardItemModel* model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({ "Name", "Type", "Uia ID", "className", "helpText", "Rectangle", "isEnabled", "isFocus"});
    for (const auto& c : controls)
    {
        addControlToModel(model->invisibleRootItem(), c);
    }
    myUi->attrTree->setModel(model);
    myUi->attrTree->expandAll();
}

//指明按键
void MainWindow::on_selectButton_clicked()
{
    if (!m_overlay) 
        m_overlay = new OverlayWidget();
    
    if (!m_trackTimer) 
    {
        m_trackTimer = new QTimer(this);
        connect(m_trackTimer, &QTimer::timeout, this, [this]() 
        {
            int x = 0;
            int y = 0;
            POINT pt;
            GetCursorPos(&pt); // 获取的是物理像素坐标
            IUIAutomationElement* pElement = nullptr;
            IUIAutomation* pAutomation = m_helper.getAutomationObject(); 
            if (pAutomation && SUCCEEDED(pAutomation->ElementFromPoint(pt, &pElement)) && pElement) 
            {
                // 方法1：从当前拿到的 pElement 开始向深层探测
                IUIAutomationElement* pDeepElement = findDeepestElementAtPoint(pElement, pt);
                pElement->Release(); // 释放初次获取的较浅元素
                pElement = pDeepElement; // 使用最深层元素

                // 方法2：使用控制视图（ControlView）的迭代器进行归一化，
                // 强制它向下寻找真正代表控件的子节点，而不是只停留在容器层。
                //IUIAutomationTreeWalker* pWalker = nullptr;
                //pAutomation->get_ControlViewWalker(&pWalker);
                //IUIAutomationElement* pNormalized = nullptr;
                //// NormalizeElement 会返回该点所属的最具体的控制元素
                //pWalker->NormalizeElement(pElement, &pNormalized);
                //if (pNormalized) 
                //{
                //    pElement->Release();
                //    pElement = pNormalized; // 替换为更具体的节点
                //}
                //pWalker->Release();
                RECT winRect;
                if (SUCCEEDED(pElement->get_CurrentBoundingRectangle(&winRect))) 
                {
                    // 1. 获取当前屏幕的缩放比例
                    // 根据鼠标当前点获取所在的屏幕
                    QScreen* screen = QGuiApplication::screenAt(QPoint(pt.x, pt.y));
                    if (!screen) screen = QGuiApplication::primaryScreen();
                    qreal dpr = screen->devicePixelRatio();

                    // 2. 将物理像素转换为 Qt 逻辑像素
                    x = static_cast<int>(winRect.left / dpr);
                    y = static_cast<int>(winRect.top / dpr);
                    int w = static_cast<int>((winRect.right - winRect.left) / dpr);
                    int h = static_cast<int>((winRect.bottom - winRect.top) / dpr);

                    if (w > 0 && h > 0) 
                    {
                        m_overlay->setGeometry(x, y, w, h);
                        m_overlay->updateRect(m_overlay->rect());
                        m_overlay->show();
                    }
                }
                updateAttrTreeWithRealTimeElement(pElement, x, y); 
                pElement->Release();
            }
            // 退出逻辑
            if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) || (GetAsyncKeyState(VK_RBUTTON) & 0x8000)) 
            {
                m_trackTimer->stop();
                m_overlay->hide();
            }
        });
    }
    m_trackTimer->start(100);
}

// 刷新按钮
void MainWindow::on_refreshButton_clicked()
{
    QGuiApplication::setOverrideCursor(Qt::WaitCursor); // 显示转圈光标
    // 1. 获取最新的全量名录树数据
    // 注意：getAllNameTree 内部会执行递归扫描，可能耗时 1-2 秒
    QVector<NameInfo> nameTreeData = m_helper.getAllNameTree();

    // 2. 获取并准备模型
    // 建议在 MainWindow 中维护一个成员变量 m_nameModel，或者从 View 中获取
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(myUi->nameTree->model());

    if (!model) {
        model = new QStandardItemModel(this);
        myUi->nameTree->setModel(model);
    }

    // 3. 清空现有数据
    model->clear();
    model->setHorizontalHeaderLabels({ "UI 元素层级结构" });

    // 4. 重新填充数据
    for (const auto& info : nameTreeData) {
        addNameToModel(model->invisibleRootItem(), info);
    }

    // 可选：刷新后自动展开第一层
    myUi->nameTree->expandToDepth(0);
    QGuiApplication::restoreOverrideCursor(); // 恢复正常光标
}

void MainWindow::on_nameTree_clicked(const QModelIndex& index)
{
    if (!index.isValid()) 
        return;

    QStringList path = getPathToRoot(index);
    if (path.isEmpty()) 
        return;

    // 1. 获取桌面根节点
    IUIAutomationElement* pCurrentParent = nullptr;
    IUIAutomation* pAutomation = m_helper.getAutomationObject(); // 需要在 Helper 中暴露指针 
    pAutomation->GetRootElement(&pCurrentParent); //[cite:2]

    // 2. 按照路径逐层深入查找
    for (const QString& name : path) 
    {
        VARIANT var;
        var.vt = VT_BSTR;
        var.bstrVal = SysAllocString((BSTR)name.utf16());
        IUIAutomationCondition* pCondition = nullptr;
        pAutomation->CreatePropertyCondition(UIA_NamePropertyId, var, &pCondition);// [cite:2]
        IUIAutomationElement* pNextChild = nullptr;
        // 关键：只在当前父节点的直接子节点中找（TreeScope_Children），效率极高
        pCurrentParent->FindFirst(TreeScope_Children, pCondition, &pNextChild);// [cite:2]
        // 释放旧父节点和资源
        pCurrentParent->Release();
        pCondition->Release();
        SysFreeString(var.bstrVal);
        if (!pNextChild)
        {
            pCurrentParent = nullptr;
            break;
        }
        pCurrentParent = pNextChild;
    }
    // 3. 此时 pCurrentParent 就是你实时点击的那个控件
    if (pCurrentParent) 
    {
        updateAttrTreeWithRealTimeElement(pCurrentParent, -1, -1);
        pCurrentParent->Release();
    }
}

IUIAutomationElement* MainWindow::findDeepestElementAtPoint(IUIAutomationElement* pParent, POINT pt)
{
    IUIAutomation* pAutomation = m_helper.getAutomationObject();
    IUIAutomationTreeWalker* pWalker = nullptr;
    // 获取控制视图 Walker（过滤掉非交互的辅助元素）
    pAutomation->get_ControlViewWalker(&pWalker);
    IUIAutomationElement* pCurrent = pParent;
    pCurrent->AddRef(); // 增加引用计数，方便统一释放
    while (true) 
    {
        IUIAutomationElement* pChild = nullptr;
        // 获取第一个子元素
        pWalker->GetFirstChildElement(pCurrent, &pChild);
        bool foundSmallerChild = false;
        while (pChild) 
        {
            RECT rect;
            pChild->get_CurrentBoundingRectangle(&rect);

            // 检查鼠标点是否在子元素的矩形范围内
            if (pt.x >= rect.left && pt.x <= rect.right && pt.y >= rect.top && pt.y <= rect.bottom) 
            {
                // 找到了更小的包含点的子元素
                pCurrent->Release();
                pCurrent = pChild; // 移动到子元素，准备探测更深层
                foundSmallerChild = true;
                break;
            }

            // 如果这个子元素不包含点，寻找它的兄弟节点
            IUIAutomationElement* pNext = nullptr;
            pWalker->GetNextSiblingElement(pChild, &pNext);
            pChild->Release();
            pChild = pNext;
        }

        // 如果所有子元素都不包含这个点，说明 pCurrent 就是我们要找的最深层
        if (!foundSmallerChild) 
        {
            if (pChild) pChild->Release();
            break;
        }
    }

    pWalker->Release();
    return pCurrent; // 返回最深层的元素
}

void MainWindow::updateAttrTreeWithRealTimeElement(IUIAutomationElement* pElement, int x, int y)
{
    if (!pElement) return;

    // 1. 获取或创建右侧树的模型
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(myUi->attrTree->model());
    if (!model) {
        model = new QStandardItemModel(this);
        myUi->attrTree->setModel(model);
    }
    model->clear();
    if (x == -1 && y == -1)
        model->setHorizontalHeaderLabels({"How found:", "Select from tree..."});
    else
        model->setHorizontalHeaderLabels({"How found:", QString("Mouse move (%1, %2)").arg(x).arg(y)});

    // 2. 准备提取实时属性
    BSTR name = nullptr, autoId = nullptr, className = nullptr, typeName = nullptr;
    BOOL bEnabled = FALSE;
    RECT rect;

    // 调用 UIA 接口获取当前时刻的状态 
    pElement->get_CurrentName(&name);
    pElement->get_CurrentAutomationId(&autoId);
    pElement->get_CurrentClassName(&className);
    pElement->get_CurrentLocalizedControlType(&typeName);
    pElement->get_CurrentIsEnabled(&bEnabled);
    pElement->get_CurrentBoundingRectangle(&rect);

    // 3. 定义辅助 Lambda 函数用于添加行
    auto addRow = [&](QString prop, QString val) {
        QList<QStandardItem*> items;
        items << new QStandardItem(prop) << new QStandardItem(val);
        model->appendRow(items);
    };

    // 4. 将提取到的实时数据填入表格
    addRow("Name", QString::fromWCharArray(name ? name : L""));
    addRow("Automation ID", QString::fromWCharArray(autoId ? autoId : L""));
    addRow("Class Name", QString::fromWCharArray(className ? className : L""));
    addRow("Control Type", QString::fromWCharArray(typeName ? typeName : L""));
    addRow("IsEnabled", bEnabled ? "True" : "False");
    addRow("Location", QString("(%1, %2, %3, %4)")
        .arg(rect.left).arg(rect.top)
        .arg(rect.right - rect.left).arg(rect.bottom - rect.top));

    // 5. 释放 COM 字符串资源 
    if (name) SysFreeString(name);
    if (autoId) SysFreeString(autoId);
    if (className) SysFreeString(className);
    if (typeName) SysFreeString(typeName);

    // 调整列宽以适应内容
    myUi->attrTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}