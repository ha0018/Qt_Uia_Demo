#pragma once
#include <QString>
#include <QVector>

struct ControlInfo {
    QString name;               // 窗口类名
    QString type;               // 类型 ID
    QString automationId;       // uia ID
    QString className;          // 窗口类名
    QString helpText;           // 帮助文本
    QString rect;               // 位置
    QString enabled;            // 是否启用
    QString focus;           // 是否可获焦
    QVector<ControlInfo> children; // 新增：存储子节点
};