#pragma once
#ifndef PIN_H
#define PIN_H

// 前向声明
class CircuitElement;
class Wire;

// 引脚类
class Pin {
public:
    // 构造函数：初始化引脚位置、类型和父元素
    Pin(int x, int y, bool isInput, CircuitElement* parent, bool isVirtual = false)
        : posX(x), posY(y), input(isInput), value(false),
        connectedWire(nullptr), parentElement(parent), virtualPin(isVirtual) {
    }

    // Getter方法 - 获取引脚属性
    int GetX() const { return posX; }
    int GetY() const { return posY; }
    bool IsInput() const { return input; }
    bool GetValue() const { return value; }
    bool IsVirtual() const { return virtualPin; }

    // Setter方法 - 设置引脚属性  
    void SetValue(bool val) { value = val; }
    void SetConnectedWire(Wire* wire) { connectedWire = wire; }
    void SetPosition(int x, int y) { posX = x; posY = y; }

    // 获取连接的导线
    Wire* GetConnectedWire() const { return connectedWire; }

    // 获取父元素
    CircuitElement* GetParent() const { return parentElement; }

    // 生成唯一ID - 用于序列化
    wxString GetId() const {
        return wxString::Format("%p_%d", parentElement, input ? 1 : 0);
    }

private:
    int posX, posY;           // 引脚坐标
    bool input;               // 是否为输入引脚
    bool value;               // 逻辑值(true=1, false=0)
    Wire* connectedWire;      // 连接的导线
    CircuitElement* parentElement; // 所属的电路元件
    bool virtualPin;          // 新增：是否为虚拟引脚
};
#endif
