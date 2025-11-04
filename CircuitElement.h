#pragma once
#ifndef CIRCUITELEMENT_H
#define CIRCUITELEMENT_H

#include <wx/propgrid/propgrid.h> // 属性网格
#include "Enums.h"

// 电路元素基类
class CircuitElement {
public:
    // 构造函数：初始化类型和位置
    CircuitElement(ElementType type, int x, int y)
        : type(type), posX(x), posY(y), selected(false) {
    }

    virtual ~CircuitElement() {}  // 虚析构函数

    // 基本属性访问方法
    ElementType GetType() const { return type; }
    int GetX() const { return posX; }
    int GetY() const { return posY; }
    bool IsSelected() const { return selected; }
    void SetSelected(bool sel) { selected = sel; }

    // 设置位置并更新所有引脚位置
    void SetPosition(int x, int y) {
        int dx = x - posX;
        int dy = y - posY;
        posX = x;
        posY = y;

        // 更新引脚位置 - 保持相对位置
        for (auto pin : GetPins()) {
            pin->SetPosition(pin->GetX() + dx, pin->GetY() + dy);
        }
    }

    // 纯虚函数 - 子类必须实现
    virtual void Draw(wxDC& dc) = 0;              // 绘制元件
    virtual void Update() = 0;                    // 更新逻辑状态
    virtual std::vector<Pin*> GetPins() = 0;      // 获取所有引脚
    virtual wxRect GetBoundingBox() const = 0;    // 获取边界框
    virtual wxString GetName() const = 0;         // 获取名称
    virtual wxString GetDisplayName() const = 0;  // 获取显示名称

    // 序列化接口
    virtual void Serialize(wxString& data) const = 0;  // 序列化到字符串
    virtual void Deserialize(const wxString& data) = 0; // 从字符串反序列化


    // 属性网格接口
    virtual void GetProperties(wxPropertyGrid* pg) const = 0; // 获取属性
    virtual void SetProperties(wxPropertyGrid* pg) = 0;       // 设置属性

protected:
    ElementType type;    // 元件类型
    int posX, posY;      // 位置坐标
    bool selected;       // 是否被选中
};

#endif
