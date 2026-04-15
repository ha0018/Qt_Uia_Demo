#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <uiautomation.h>
#include <QStandardItemModel>

// 辅助函数：递归将 ControlInfo 节点及其子节点加入模型
void addControlToModel(QStandardItem* parentItem, const ControlInfo& info) {
    // 创建当前行的三个列单元格
    QList<QStandardItem*> rowItems;
    rowItems << new QStandardItem(info.name)
        << new QStandardItem(info.type)
        << new QStandardItem(info.automationId)
        << new QStandardItem(info.className)
        << new QStandardItem(info.helpText)
        << new QStandardItem(info.rect)
        << new QStandardItem(info.enabled)
        << new QStandardItem(info.focus);

    // 将这一行作为子节点加入父节点
    parentItem->appendRow(rowItems);

    // 递归处理子节点：使用该行第一列的 Item 作为下一层的容器[cite: 5]
    for (const auto& child : info.children) {
        addControlToModel(rowItems[0], child);
    }
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), myUi(new Ui::QtUiAutomationClass)
{
    BOOL isScreenReaderActive = FALSE;
    SystemParametersInfo(SPI_GETSCREENREADER, 0, &isScreenReaderActive, 0);

    if (isScreenReaderActive) {
        // 系统已进入“屏幕阅读器模式”，大多数框架（如 Chrome/Qt）此时会自动暴露 UIA
        MessageBox(
            NULL,
            L"确定要执行此操作吗？",
            L"系统提示",
            MB_YESNO | MB_ICONQUESTION
        );
    }
    myUi->setupUi(this);
    // 1. 调用 Helper 获取全系统窗口树
    // 注意：这可能需要几秒钟，因为系统窗口非常多
    auto allControls = m_helper.getAllWindowsTree();

    // 2. 更新模型
    QStandardItemModel* model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({"Name", "Type", "Uia ID", "className", "helpText", "Rectangle", "isEnabled", "isFocus" });

    for (const auto& control : allControls) {
        // 过滤掉一些没有名字的背景进程窗口（可选）
        if (control.name.isEmpty() && control.automationId.isEmpty()) continue;

        addControlToModel(model->invisibleRootItem(), control);
    }

    myUi->treeView->setModel(model);
}

MainWindow::~MainWindow() {
    delete myUi;
}

void MainWindow::on_searchButton_clicked() {
    QString title = myUi->lineEdit->text();

    // 从 Helper 获取带有层级关系的控件树
    auto controls = m_helper.getControlsByWindowName(title);

    // 创建标准项模型[cite: 5]
    QStandardItemModel* model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({ "Name", "Type", "Uia ID", "className", "helpText", "Rectangle", "isEnabled", "isFocus"});

    for (const auto& c : controls) {
        // 从模型的根部开始递归构建树[cite: 5]
        addControlToModel(model->invisibleRootItem(), c);
    }

    // 设置模型给 TreeView[cite: 5]
    myUi->treeView->setModel(model);

    // 展开所有节点以达到图中的展示效果[cite: 5]
    myUi->treeView->expandAll();
}

void MainWindow::updateTreeViewWithSubTree(const ControlInfo& subTree) {
    // 1. 获取当前 TreeView 使用的模型
    // 注意：如果是第一次加载，需要新建模型并设置表头
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(myUi->treeView->model());

    if (!model) {
        model = new QStandardItemModel(this);
        myUi->treeView->setModel(model);
    }

    // 2. 清空现有内容
    model->clear();

    // 3. 设置表头（clear 会把表头也删掉，所以要重新设）
    model->setHorizontalHeaderLabels({ "Name", "Type", "Automation ID", "Rect" });

    // 4. 使用我们之前写好的递归填充函数 addControlToModel
    // 将这棵新抓取的子树挂载到模型的根节点上
    addControlToModel(model->invisibleRootItem(), subTree);

    // 5. 自动展开前几层，方便查看
    myUi->treeView->expandAll();
}

void MainWindow::on_selectButton_clicked() {
    // 1. 获取 TreeView 当前选中的索引
    QModelIndex index = myUi->treeView->currentIndex();
    if (!index.isValid()) return;

    // 2. 获取该节点的 Automation ID (假设它在第三列)
    QString autoId = index.siblingAtColumn(2).data().toString();
    QString name = index.siblingAtColumn(0).data().toString();

    if (autoId.isEmpty() && name.isEmpty()) return;

    // 3. 寻找该元素 (需要 Helper 提供一个 FindElement 方法)
    // 这里简化逻辑：在当前窗口下通过 ID 找这个元素
    IUIAutomationElement* targetElement = m_helper.findElementByIdAndName(autoId, name);

    if (targetElement) {
        // 4. 加载子树
        ControlInfo subTree = m_helper.getTreeFromElement(targetElement);

        // 5. 更新模型 (可以选择清空模型只显示这棵子树，或者挂载到当前节点下)
        updateTreeViewWithSubTree(subTree);

        targetElement->Release();
    }
}