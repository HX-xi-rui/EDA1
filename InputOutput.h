#pragma once
#ifndef INPUTOUTPUT_H
#define INPUTOUTPUT_H

// 输入输出引脚类
class InputOutput : public CircuitElement {
public:
    // 构造函数，接收元件类型、坐标位置和自定义名称
    InputOutput(ElementType type, int x, int y, const wxString& name = "") : CircuitElement(type, x, y), value(false), customName(name) {  // 初始化基类和成员变量
        // 根据类型创建引脚：输入元件在右侧有输出引脚，输出元件在左侧有输入引脚
        if (type == TYPE_INPUT) {  // 如果是输入元件
            pins.push_back(std::make_unique<Pin>(x + 20, y, false, this)); // 创建输出引脚（右侧）
        }
        else {  // 如果是输出元件
            pins.push_back(std::make_unique<Pin>(x - 20, y, true, this)); // 创建输入引脚（左侧）
        }
    }

    // 绘制输入输出元件
    virtual void Draw(wxDC& dc) override {
        dc.SetPen(selected ? wxPen(*wxRED, 2) : wxPen(*wxBLACK, 1));
        dc.SetBrush(value ? *wxGREEN_BRUSH : *wxWHITE_BRUSH);

        // 1. 绘制带方向凹口的主体（逻辑不变）
        const int BODY_SIZE = 30;
        const int NOTCH_DEPTH = 6;
        std::vector<wxPoint> bodyPoints;
        if (type == TYPE_INPUT) {
            bodyPoints = {
                wxPoint(posX - BODY_SIZE / 2, posY - BODY_SIZE / 2),
                wxPoint(posX + BODY_SIZE / 2, posY - BODY_SIZE / 2),
                wxPoint(posX + BODY_SIZE / 2, posY + BODY_SIZE / 2),
                wxPoint(posX - BODY_SIZE / 2, posY + BODY_SIZE / 2),
                wxPoint(posX - BODY_SIZE / 2 + NOTCH_DEPTH, posY + BODY_SIZE / 4),
                wxPoint(posX - BODY_SIZE / 2 + NOTCH_DEPTH, posY - BODY_SIZE / 4),
                wxPoint(posX - BODY_SIZE / 2, posY - BODY_SIZE / 2)
            };
        }
        else {
            bodyPoints = {
                wxPoint(posX - BODY_SIZE / 2, posY - BODY_SIZE / 2),
                wxPoint(posX + BODY_SIZE / 2, posY - BODY_SIZE / 2),
                wxPoint(posX + BODY_SIZE / 2 - NOTCH_DEPTH, posY - BODY_SIZE / 4),
                wxPoint(posX + BODY_SIZE / 2 - NOTCH_DEPTH, posY + BODY_SIZE / 4),
                wxPoint(posX + BODY_SIZE / 2, posY + BODY_SIZE / 2),
                wxPoint(posX - BODY_SIZE / 2, posY + BODY_SIZE / 2),
                wxPoint(posX - BODY_SIZE / 2, posY - BODY_SIZE / 2)
            };
        }
        dc.DrawPolygon(static_cast<int>(bodyPoints.size()), &bodyPoints[0]);

        // 2. 绘制方向箭头（逻辑不变）
        dc.SetPen(*wxBLACK_PEN);
        const int ARROW_LEN = 8;
        const int ARROW_ANGLE = 3;
        if (type == TYPE_INPUT) {
            wxPoint arrowStart(posX - BODY_SIZE / 2 - ARROW_LEN, posY);
            wxPoint arrowEnd(posX - BODY_SIZE / 2 + NOTCH_DEPTH, posY);
            dc.DrawLine(arrowStart, arrowEnd);
            dc.DrawLine(arrowEnd, wxPoint(arrowEnd.x - ARROW_ANGLE, arrowEnd.y - ARROW_ANGLE));
            dc.DrawLine(arrowEnd, wxPoint(arrowEnd.x - ARROW_ANGLE, arrowEnd.y + ARROW_ANGLE));
        }
        else {
            wxPoint arrowStart(posX + BODY_SIZE / 2 - NOTCH_DEPTH, posY);
            wxPoint arrowEnd(posX + BODY_SIZE / 2 + ARROW_LEN, posY);
            dc.DrawLine(arrowStart, arrowEnd);
            dc.DrawLine(arrowEnd, wxPoint(arrowEnd.x - ARROW_ANGLE, arrowEnd.y - ARROW_ANGLE));
            dc.DrawLine(arrowEnd, wxPoint(arrowEnd.x - ARROW_ANGLE, arrowEnd.y + ARROW_ANGLE));
        }

        // 3. 绘制标签（居中逻辑不变）
        wxString label;
        if (!customName.empty()) {
            label = customName;
        }
        else if (type == TYPE_INPUT) {
            label = "IN";
        }
        else {
            label = "OUT";
        }
        wxSize labelSize = dc.GetTextExtent(label);
        int horizontalOffset = type == TYPE_INPUT ? 3 : -3; // 微调偏移
        dc.DrawText(label, posX - labelSize.GetWidth() / 2 + horizontalOffset, posY - BODY_SIZE / 4);


        // -------------------------- 调整后：圆圈和数字整体下移25px --------------------------
        dc.SetTextForeground(*wxRED);
        wxString valueText = value ? "1" : "0";
        wxSize textSize = dc.GetTextExtent(valueText);

        const int CIRCLE_PADDING = 2;
        int circleDiameter = std::max(textSize.GetWidth(), textSize.GetHeight()) + 2 * CIRCLE_PADDING;
        int circleRadius = circleDiameter / 2;

        // 核心修改：在原垂直中心基础上 +25px（整体下移25px）
        wxPoint circleCenter(
            posX - textSize.GetWidth() / 2, // 水平位置不变
            // 原垂直中心：posY + BODY_SIZE/4 - textSize.GetHeight()/2 
            // 新增下移：+25
            posY + BODY_SIZE / 4 - textSize.GetHeight() / 2 - 27
        );

        // 1. 绘制圆形边框
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawCircle(circleCenter, circleRadius);

        // 2. 绘制"0/1"文字（与圆形中心对齐，随圆形同步下移）
        dc.DrawText(
            valueText,
            circleCenter.x - textSize.GetWidth() / 2,
            circleCenter.y - textSize.GetHeight() / 2
        );
        // -------------------------------------------------------------------------------------


        // 4. 绘制引脚（逻辑不变）
        dc.SetPen(*wxBLACK_PEN);
        dc.SetTextForeground(*wxBLACK);
        for (auto& pin : pins) {
            dc.DrawCircle(pin->GetX(), pin->GetY(), 3);
        }
    }

    // 更新元件状态
    virtual void Update() override {
        // 对于输入元件：把自身的value写到它的输出引脚（驱动信号）
        if (type == TYPE_INPUT) {
            if (!pins.empty()) {  // 确保有引脚
                // pins[0]是输出引脚（构造时如此）
                pins[0]->SetValue(value);  // 将输入元件的值设置到输出引脚
            }
        }
        // 对于输出元件：从连接的输入引脚读值到自身value（显示/记录输出）
        else if (type == TYPE_OUTPUT) {
            if (!pins.empty()) {  // 确保有引脚
                // pins[0]是输入引脚（构造时如此）
                value = pins[0]->GetValue();  // 从输入引脚读取值
            }
        }
    }

    // 设置元件值
    void SetValue(bool val) { value = val; }
    // 获取元件值
    bool GetValue() const { return value; }
    // 设置自定义名称
    void SetName(const wxString& name) { customName = name; }
    // 获取自定义名称
    wxString GetCustomName() const { return customName; }

    // 获取所有引脚指针
    virtual std::vector<Pin*> GetPins() override {
        std::vector<Pin*> pinPtrs;  // 创建引脚指针向量
        for (auto& pin : pins) {  // 遍历引脚容器
            pinPtrs.push_back(pin.get());  // 添加原始指针
        }
        return pinPtrs;  // 返回引脚指针列表
    }

    // 获取边界框
    virtual wxRect GetBoundingBox() const override {
        return wxRect(posX - 15, posY - 15, 30, 30);  // 返回元件边界矩形
    }

    // 获取元件类型名称
    virtual wxString GetName() const override {
        return type == TYPE_INPUT ? "INPUT" : "OUTPUT";  // 返回类型字符串
    }

    // 获取显示名称
    virtual wxString GetDisplayName() const override {
        if (!customName.empty()) {  // 如果有自定义名称
            return customName;  // 返回自定义名称
        }
        return type == TYPE_INPUT ? "Input Pin" : "Output Pin";  // 返回默认显示名称
    }

    // 序列化元件数据
virtual void Serialize(wxString& data) const override {
    // 只保存真正的自定义名称，过滤掉默认名称
    wxString nameToSave = customName;
    if (nameToSave == "INPUT" || nameToSave == "OUTPUT" || 
        nameToSave == "IN" || nameToSave == "OUT" ||
        nameToSave == "Input Pin" || nameToSave == "Output Pin") {
        nameToSave = "";
    }
    
    data += wxString::Format("%d,%d,%d,%d,%s,",
        type, posX, posY, value ? 1 : 0, nameToSave);
}

    // 反序列化元件数据
    virtual void Deserialize(const wxString& data) override {
        wxStringTokenizer tokenizer(data, ",");  // 创建字符串分词器
        if (tokenizer.CountTokens() >= 4) {  // 检查是否有足够的数据
            long typeVal, x, y, val;  // 定义解析变量
            tokenizer.GetNextToken().ToLong(&typeVal);  // 解析类型
            tokenizer.GetNextToken().ToLong(&x);  // 解析X坐标
            tokenizer.GetNextToken().ToLong(&y);  // 解析Y坐标
            tokenizer.GetNextToken().ToLong(&val);  // 解析值
            SetPosition(x, y);  // 设置位置
            value = val != 0;  // 设置布尔值

            if (tokenizer.HasMoreTokens()) {  // 如果还有更多数据
                customName = tokenizer.GetNextToken();  // 解析自定义名称
            }
        }
    }

    // 获取属性用于属性网格显示
    virtual void GetProperties(wxPropertyGrid* pg) const override {
        pg->Append(new wxStringProperty("Type", "Type", GetDisplayName()));  // 类型属性
        pg->Append(new wxIntProperty("X Position", "X", posX));  // X坐标属性
        pg->Append(new wxIntProperty("Y Position", "Y", posY));  // Y坐标属性
        pg->Append(new wxStringProperty("Name", "Name", customName));  // 名称属性
        pg->Append(new wxBoolProperty("Value", "Value", value));  // 值属性
    }

    // 从属性网格设置属性
    virtual void SetProperties(wxPropertyGrid* pg) override {
        wxVariant xVar = pg->GetPropertyValue("X");  // 获取X坐标值
        wxVariant yVar = pg->GetPropertyValue("Y");  // 获取Y坐标值
        wxVariant nameVar = pg->GetPropertyValue("Name");  // 获取名称值
        wxVariant valueVar = pg->GetPropertyValue("Value");  // 获取布尔值

        if (xVar.IsType("long") && yVar.IsType("long")) {  // 检查坐标类型
            SetPosition(xVar.GetLong(), yVar.GetLong());  // 设置新位置
        }
        if (nameVar.IsType("string")) {  // 检查名称类型
            customName = nameVar.GetString();  // 设置新名称
        }
        if (valueVar.IsType("bool")) {  // 检查值类型
            value = valueVar.GetBool();  // 设置新值
        }
    }

private:
    std::vector<std::unique_ptr<Pin>> pins; // 引脚列表（智能指针管理）
    bool value;  // 元件当前值
    wxString customName; // 自定义名称
};
#endif 
