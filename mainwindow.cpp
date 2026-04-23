#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <uiautomation.h>
#include <QStandardItemModel>
#include <QStringListModel>

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
    rowItems << new QStandardItem(info.name);
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
        path.prepend(index.data(Qt::DisplayRole).toString());
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


    myUi->nameTree->header()->hide(); 
    auto allNameInfos = m_helper.getAllNameTree();
    QStandardItemModel* nameModel = new QStandardItemModel(this);
    for (const auto& nameInfo : allNameInfos)
    {
        if (nameInfo.name.isEmpty())
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

void MainWindow::on_selectButton_clicked() 
{
    //暂未实现
}

void MainWindow::on_nameTree_clicked(const QModelIndex& index)
{
    if (!index.isValid()) 
        return;

    QStringList path = getPathToRoot(index);
    if (path.isEmpty()) return;

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
        updateAttrTreeWithRealTimeElement(pCurrentParent);
        pCurrentParent->Release();
    }

}

void MainWindow::updateAttrTreeWithRealTimeElement(IUIAutomationElement* pElement) 
{
    if (!pElement) return;

    // 1. 获取或创建右侧树的模型
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(myUi->attrTree->model());
    if (!model) {
        model = new QStandardItemModel(this);
        myUi->attrTree->setModel(model);
    }
    model->clear();
    model->setHorizontalHeaderLabels({ "属性名", "当前实时值" });

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