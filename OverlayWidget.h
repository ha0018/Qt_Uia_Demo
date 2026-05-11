#pragma once
#include <QWidget>
#include <QRect>
#include <QPainter> 
#include <QPen>     

class OverlayWidget : public QWidget 
{
    Q_OBJECT
public:
    explicit OverlayWidget(QWidget* parent = nullptr) : QWidget(parent) 
    {
        // 设置窗口属性：无边框、顶层显示、工具窗口
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        // 设置背景透明
        setAttribute(Qt::WA_TranslucentBackground);
        // 设置鼠标穿透，否则覆盖层会拦截鼠标点击，导致无法选中下方元素
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    void updateRect(const QRect& r) 
    {
        m_rect = r;
        update(); // 触发重绘
    }

protected:
    void paintEvent(QPaintEvent*) override 
    {
        if (m_rect.isNull()) return;

        QPainter painter(this); // 修复 C2079
        painter.setRenderHint(QPainter::Antialiasing);

        // 使用红色画笔，宽度为 3
        QPen pen(Qt::red, 3);   // 修复 C2027
        painter.setPen(pen);

        // 绘制矩形（稍微缩进一点，防止边框超出窗口边缘被切掉）
        painter.drawRect(this->rect().adjusted(1, 1, -1, -1));
    }

private:
    QRect m_rect;
};