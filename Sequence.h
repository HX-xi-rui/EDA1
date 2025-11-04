#pragma once
#ifndef SEQUENCE_H
#define SEQUENCE_H

// 时钟信号类
class ClockElement : public CircuitElement {
public:
    ClockElement(int x, int y, int frequency = 1)
        : CircuitElement(TYPE_CLOCK, x, y), value(false), frequency(frequency),
        counter(0), enabled(true) {
        // 输出引脚
        pins.push_back(std::make_unique<Pin>(x + 20, y, false, this));
    }

    virtual void Draw(wxDC& dc) override {
        dc.SetPen(selected ? wxPen(*wxRED, 2) : wxPen(*wxBLACK, 1));
        dc.SetBrush(value ? *wxGREEN_BRUSH : *wxWHITE_BRUSH);

        // 绘制时钟符号
        dc.DrawRectangle(posX - 15, posY - 15, 30, 30);
        dc.DrawText("CLK", posX - 12, posY - 7);

        // 显示频率
        wxString freqText = wxString::Format("%dHz", frequency);
        dc.SetTextForeground(*wxBLUE);
        wxFont smallFont = dc.GetFont();
        smallFont.SetPointSize(7);
        dc.SetFont(smallFont);
        dc.DrawText(freqText, posX - 10, posY + 10);

        // 绘制引脚
        dc.SetPen(*wxBLACK_PEN);
        for (auto& pin : pins) {
            dc.DrawCircle(pin->GetX(), pin->GetY(), 3);
        }

        // 显示当前值
        dc.SetTextForeground(*wxRED);
        wxString valText = value ? "1" : "0";
        dc.DrawText(valText, posX + 15, posY - 5);
    }

    virtual void Update() override {
        if (!enabled) return;

        counter++;
        if (counter >= frequency) {
            value = !value;
            counter = 0;
        }

        if (!pins.empty()) {
            pins[0]->SetValue(value);
        }
    }

    virtual std::vector<Pin*> GetPins() override {
        std::vector<Pin*> pinPtrs;
        for (auto& pin : pins) {
            pinPtrs.push_back(pin.get());
        }
        return pinPtrs;
    }

    virtual wxRect GetBoundingBox() const override {
        return wxRect(posX - 15, posY - 15, 30, 30);
    }

    virtual wxString GetName() const override {
        return "CLOCK";
    }

    virtual wxString GetDisplayName() const override {
        return wxString::Format("Clock (%dHz)", frequency);
    }

    virtual void Serialize(wxString& data) const override {
        data += wxString::Format("%d,%d,%d,%d,%d,", type, posX, posY, frequency, enabled ? 1 : 0);
    }

    virtual void Deserialize(const wxString& data) override {
        wxStringTokenizer tokenizer(data, ",");
        if (tokenizer.CountTokens() >= 5) {
            long typeVal, x, y, freq, en;
            tokenizer.GetNextToken().ToLong(&typeVal);
            tokenizer.GetNextToken().ToLong(&x);
            tokenizer.GetNextToken().ToLong(&y);
            tokenizer.GetNextToken().ToLong(&freq);
            tokenizer.GetNextToken().ToLong(&en);
            SetPosition(x, y);
            frequency = freq;
            enabled = en != 0;
        }
    }

    virtual void GetProperties(wxPropertyGrid* pg) const override {
        pg->Append(new wxStringProperty("Type", "Type", GetDisplayName()));
        pg->Append(new wxIntProperty("X Position", "X", posX));
        pg->Append(new wxIntProperty("Y Position", "Y", posY));
        pg->Append(new wxIntProperty("Frequency", "Frequency", frequency));
        pg->Append(new wxBoolProperty("Enabled", "Enabled", enabled));
    }

    virtual void SetProperties(wxPropertyGrid* pg) override {
        wxVariant xVar = pg->GetPropertyValue("X");
        wxVariant yVar = pg->GetPropertyValue("Y");
        wxVariant freqVar = pg->GetPropertyValue("Frequency");
        wxVariant enabledVar = pg->GetPropertyValue("Enabled");

        if (xVar.IsType("long") && yVar.IsType("long")) {
            SetPosition(xVar.GetLong(), yVar.GetLong());
        }
        if (freqVar.IsType("long")) {
            frequency = freqVar.GetLong();
        }
        if (enabledVar.IsType("bool")) {
            enabled = enabledVar.GetBool();
        }
    }

    void SetFrequency(int freq) { frequency = freq; }
    int GetFrequency() const { return frequency; }
    void SetEnabled(bool en) { enabled = en; }
    bool IsEnabled() const { return enabled; }

private:
    std::vector<std::unique_ptr<Pin>> pins;
    bool value;
    int frequency;
    int counter;
    bool enabled;
};

// RS触发器类
class RSFlipFlop : public CircuitElement {
public:
    RSFlipFlop(int x, int y)
        : CircuitElement(TYPE_RS_FLIPFLOP, x, y), q(false), qNot(true) {
        // 输入引脚: S, R
        pins.push_back(std::make_unique<Pin>(x - 20, y - 15, true, this));   // S (Set)
        pins.push_back(std::make_unique<Pin>(x - 20, y + 15, true, this));   // R (Reset)
        // 输出引脚: Q, Q'
        pins.push_back(std::make_unique<Pin>(x + 20, y - 10, false, this));  // Q
        pins.push_back(std::make_unique<Pin>(x + 20, y + 10, false, this));  // Q'
    }

    virtual void Draw(wxDC& dc) override {
        dc.SetPen(selected ? wxPen(*wxRED, 2) : wxPen(*wxBLACK, 1));
        dc.SetBrush(*wxWHITE_BRUSH);

        // 绘制RS触发器符号
        dc.DrawRectangle(posX - 15, posY - 20, 30, 40);
        dc.DrawText("RS", posX - 10, posY - 15);
        dc.DrawText("FF", posX - 10, posY - 5);

        // 绘制引脚标签
        dc.SetTextForeground(*wxBLACK);
        wxFont smallFont = dc.GetFont();
        smallFont.SetPointSize(6);
        dc.SetFont(smallFont);

        dc.DrawText("S", posX - 25, posY - 18);
        dc.DrawText("R", posX - 25, posY + 12);
        dc.DrawText("Q", posX + 15, posY - 13);
        dc.DrawText("Q'", posX + 12, posY + 13);

        // 显示当前状态
        dc.SetTextForeground(q ? *wxRED : *wxBLUE);
        wxString stateText = q ? "Q=1" : "Q=0";
        dc.DrawText(stateText, posX - 12, posY + 5);

        // 绘制引脚
        dc.SetPen(*wxBLACK_PEN);
        for (auto& pin : pins) {
            dc.DrawCircle(pin->GetX(), pin->GetY(), 3);
        }
    }

    virtual void Update() override {
        if (pins.size() < 4) return;

        bool s = pins[0]->GetValue();  // Set
        bool r = pins[1]->GetValue();  // Reset

        // RS触发器逻辑
        if (s && !r) {
            q = true;      // 置位
            qNot = false;
        }
        else if (!s && r) {
            q = false;     // 复位
            qNot = true;
        }
        else if (s && r) {
            // 非法状态：两个输入都为1
            q = true;
            qNot = true;
        }
        // 当S=0,R=0时保持状态不变

        // 设置输出
        pins[2]->SetValue(q);      // Q
        pins[3]->SetValue(qNot);   // Q'
    }

    virtual std::vector<Pin*> GetPins() override {
        std::vector<Pin*> pinPtrs;
        for (auto& pin : pins) {
            pinPtrs.push_back(pin.get());
        }
        return pinPtrs;
    }

    virtual wxRect GetBoundingBox() const override {
        return wxRect(posX - 20, posY - 25, 40, 50);
    }

    virtual wxString GetName() const override {
        return "RS_FLIPFLOP";
    }

    virtual wxString GetDisplayName() const override {
        return "RS Flip-Flop";
    }

    virtual void Serialize(wxString& data) const override {
        data += wxString::Format("%d,%d,%d,%d,%d,",
            type, posX, posY, q ? 1 : 0, qNot ? 1 : 0);
    }

    virtual void Deserialize(const wxString& data) override {
        wxStringTokenizer tokenizer(data, ",");
        if (tokenizer.CountTokens() >= 5) {
            long typeVal, x, y, qVal, qNotVal;
            tokenizer.GetNextToken().ToLong(&typeVal);
            tokenizer.GetNextToken().ToLong(&x);
            tokenizer.GetNextToken().ToLong(&y);
            tokenizer.GetNextToken().ToLong(&qVal);
            tokenizer.GetNextToken().ToLong(&qNotVal);
            SetPosition(x, y);
            q = qVal != 0;
            qNot = qNotVal != 0;
        }
    }

    virtual void GetProperties(wxPropertyGrid* pg) const override {
        pg->Append(new wxStringProperty("Type", "Type", GetDisplayName()));
        pg->Append(new wxIntProperty("X Position", "X", posX));
        pg->Append(new wxIntProperty("Y Position", "Y", posY));
        pg->Append(new wxBoolProperty("Q Output", "Q", q));
        pg->Append(new wxBoolProperty("Q' Output", "QNot", qNot));
    }

    virtual void SetProperties(wxPropertyGrid* pg) override {
        wxVariant xVar = pg->GetPropertyValue("X");
        wxVariant yVar = pg->GetPropertyValue("Y");
        wxVariant qVar = pg->GetPropertyValue("Q");
        wxVariant qNotVar = pg->GetPropertyValue("QNot");

        if (xVar.IsType("long") && yVar.IsType("long")) {
            SetPosition(xVar.GetLong(), yVar.GetLong());
        }
        if (qVar.IsType("bool")) {
            q = qVar.GetBool();
        }
        if (qNotVar.IsType("bool")) {
            qNot = qNotVar.GetBool();
        }
    }

private:
    std::vector<std::unique_ptr<Pin>> pins;
    bool q;
    bool qNot;
};

// D触发器类
class DFlipFlop : public CircuitElement {
public:
    DFlipFlop(int x, int y)
        : CircuitElement(TYPE_D_FLIPFLOP, x, y), q(false), lastClock(false) {
        // 输入引脚: D, CLK
        pins.push_back(std::make_unique<Pin>(x - 20, y - 15, true, this));   // D
        pins.push_back(std::make_unique<Pin>(x - 20, y, true, this));        // CLK
        // 输出引脚: Q, Q'
        pins.push_back(std::make_unique<Pin>(x + 20, y - 10, false, this));  // Q
        pins.push_back(std::make_unique<Pin>(x + 20, y + 10, false, this));  // Q'
    }

    virtual void Draw(wxDC& dc) override {
        dc.SetPen(selected ? wxPen(*wxRED, 2) : wxPen(*wxBLACK, 1));
        dc.SetBrush(*wxWHITE_BRUSH);

        // 绘制D触发器符号
        dc.DrawRectangle(posX - 15, posY - 20, 30, 40);
        dc.DrawText("D", posX - 12, posY - 15);
        dc.DrawText("FF", posX - 10, posY - 5);

        // 绘制引脚标签
        dc.SetTextForeground(*wxBLACK);
        wxFont smallFont = dc.GetFont();
        smallFont.SetPointSize(6);
        dc.SetFont(smallFont);

        dc.DrawText("D", posX - 25, posY - 18);
        dc.DrawText("CLK", posX - 30, posY - 3);
        dc.DrawText("Q", posX + 15, posY - 13);
        dc.DrawText("Q'", posX + 12, posY + 13);

        // 显示当前状态
        dc.SetTextForeground(q ? *wxRED : *wxBLUE);
        wxString stateText = q ? "Q=1" : "Q=0";
        dc.DrawText(stateText, posX - 12, posY + 5);

        // 绘制引脚
        dc.SetPen(*wxBLACK_PEN);
        for (auto& pin : pins) {
            dc.DrawCircle(pin->GetX(), pin->GetY(), 3);
        }
    }

    virtual void Update() override {
        if (pins.size() < 4) return;

        bool d = pins[0]->GetValue();
        bool clock = pins[1]->GetValue();

        // 时钟上升沿触发
        if (clock && !lastClock) {
            q = d;
        }
        lastClock = clock;

        // 设置输出
        pins[2]->SetValue(q);      // Q
        pins[3]->SetValue(!q);     // Q'
    }

    virtual std::vector<Pin*> GetPins() override {
        std::vector<Pin*> pinPtrs;
        for (auto& pin : pins) {
            pinPtrs.push_back(pin.get());
        }
        return pinPtrs;
    }

    virtual wxRect GetBoundingBox() const override {
        return wxRect(posX - 20, posY - 25, 40, 50);
    }

    virtual wxString GetName() const override {
        return "D_FLIPFLOP";
    }

    virtual wxString GetDisplayName() const override {
        return "D Flip-Flop";
    }

    virtual void Serialize(wxString& data) const override {
        data += wxString::Format("%d,%d,%d,%d,", type, posX, posY, q ? 1 : 0);
    }

    virtual void Deserialize(const wxString& data) override {
        wxStringTokenizer tokenizer(data, ",");
        if (tokenizer.CountTokens() >= 4) {
            long typeVal, x, y, qVal;
            tokenizer.GetNextToken().ToLong(&typeVal);
            tokenizer.GetNextToken().ToLong(&x);
            tokenizer.GetNextToken().ToLong(&y);
            tokenizer.GetNextToken().ToLong(&qVal);
            SetPosition(x, y);
            q = qVal != 0;
        }
    }

    virtual void GetProperties(wxPropertyGrid* pg) const override {
        pg->Append(new wxStringProperty("Type", "Type", GetDisplayName()));
        pg->Append(new wxIntProperty("X Position", "X", posX));
        pg->Append(new wxIntProperty("Y Position", "Y", posY));
        pg->Append(new wxBoolProperty("Q Output", "Q", q));
    }

    virtual void SetProperties(wxPropertyGrid* pg) override {
        wxVariant xVar = pg->GetPropertyValue("X");
        wxVariant yVar = pg->GetPropertyValue("Y");
        wxVariant qVar = pg->GetPropertyValue("Q");

        if (xVar.IsType("long") && yVar.IsType("long")) {
            SetPosition(xVar.GetLong(), yVar.GetLong());
        }
        if (qVar.IsType("bool")) {
            q = qVar.GetBool();
        }
    }

private:
    std::vector<std::unique_ptr<Pin>> pins;
    bool q;
    bool lastClock;
};

// JK触发器类
class JKFlipFlop : public CircuitElement {
public:
    JKFlipFlop(int x, int y)
        : CircuitElement(TYPE_JK_FLIPFLOP, x, y), q(false), lastClock(false) {
        // 输入引脚: J, K, CLK
        pins.push_back(std::make_unique<Pin>(x - 20, y - 20, true, this));   // J
        pins.push_back(std::make_unique<Pin>(x - 20, y, true, this));        // K
        pins.push_back(std::make_unique<Pin>(x - 20, y + 20, true, this));   // CLK
        // 输出引脚: Q, Q'
        pins.push_back(std::make_unique<Pin>(x + 20, y - 10, false, this));  // Q
        pins.push_back(std::make_unique<Pin>(x + 20, y + 10, false, this));  // Q'
    }

    virtual void Draw(wxDC& dc) override {
        dc.SetPen(selected ? wxPen(*wxRED, 2) : wxPen(*wxBLACK, 1));
        dc.SetBrush(*wxWHITE_BRUSH);

        // 绘制JK触发器符号
        dc.DrawRectangle(posX - 15, posY - 25, 30, 50);
        dc.DrawText("JK", posX - 10, posY - 15);
        dc.DrawText("FF", posX - 10, posY - 5);

        // 绘制引脚标签
        dc.SetTextForeground(*wxBLACK);
        wxFont smallFont = dc.GetFont();
        smallFont.SetPointSize(6);
        dc.SetFont(smallFont);

        dc.DrawText("J", posX - 25, posY - 23);
        dc.DrawText("K", posX - 25, posY - 3);
        dc.DrawText("CLK", posX - 30, posY + 17);
        dc.DrawText("Q", posX + 15, posY - 13);
        dc.DrawText("Q'", posX + 12, posY + 13);

        // 显示当前状态
        dc.SetTextForeground(q ? *wxRED : *wxBLUE);
        wxString stateText = q ? "Q=1" : "Q=0";
        dc.DrawText(stateText, posX - 12, posY + 10);

        // 绘制引脚
        dc.SetPen(*wxBLACK_PEN);
        for (auto& pin : pins) {
            dc.DrawCircle(pin->GetX(), pin->GetY(), 3);
        }
    }

    virtual void Update() override {
        if (pins.size() < 5) return;

        bool j = pins[0]->GetValue();
        bool k = pins[1]->GetValue();
        bool clock = pins[2]->GetValue();

        // 时钟上升沿触发
        if (clock && !lastClock) {
            if (j && !k) {
                q = true;      // 置位
            }
            else if (!j && k) {
                q = false;     // 复位
            }
            else if (j && k) {
                q = !q;        // 翻转
            }
            // J=0,K=0 时保持状态不变
        }
        lastClock = clock;

        // 设置输出
        pins[3]->SetValue(q);      // Q
        pins[4]->SetValue(!q);     // Q'
    }

    virtual std::vector<Pin*> GetPins() override {
        std::vector<Pin*> pinPtrs;
        for (auto& pin : pins) {
            pinPtrs.push_back(pin.get());
        }
        return pinPtrs;
    }

    virtual wxRect GetBoundingBox() const override {
        return wxRect(posX - 20, posY - 30, 40, 60);
    }

    virtual wxString GetName() const override {
        return "JK_FLIPFLOP";
    }

    virtual wxString GetDisplayName() const override {
        return "JK Flip-Flop";
    }

    virtual void Serialize(wxString& data) const override {
        data += wxString::Format("%d,%d,%d,%d,", type, posX, posY, q ? 1 : 0);
    }

    virtual void Deserialize(const wxString& data) override {
        wxStringTokenizer tokenizer(data, ",");
        if (tokenizer.CountTokens() >= 4) {
            long typeVal, x, y, qVal;
            tokenizer.GetNextToken().ToLong(&typeVal);
            tokenizer.GetNextToken().ToLong(&x);
            tokenizer.GetNextToken().ToLong(&y);
            tokenizer.GetNextToken().ToLong(&qVal);
            SetPosition(x, y);
            q = qVal != 0;
        }
    }

    virtual void GetProperties(wxPropertyGrid* pg) const override {
        pg->Append(new wxStringProperty("Type", "Type", GetDisplayName()));
        pg->Append(new wxIntProperty("X Position", "X", posX));
        pg->Append(new wxIntProperty("Y Position", "Y", posY));
        pg->Append(new wxBoolProperty("Q Output", "Q", q));
    }

    virtual void SetProperties(wxPropertyGrid* pg) override {
        wxVariant xVar = pg->GetPropertyValue("X");
        wxVariant yVar = pg->GetPropertyValue("Y");
        wxVariant qVar = pg->GetPropertyValue("Q");

        if (xVar.IsType("long") && yVar.IsType("long")) {
            SetPosition(xVar.GetLong(), yVar.GetLong());
        }
        if (qVar.IsType("bool")) {
            q = qVar.GetBool();
        }
    }

private:
    std::vector<std::unique_ptr<Pin>> pins;
    bool q;
    bool lastClock;
};

// T触发器类
class TFlipFlop : public CircuitElement {
public:
    TFlipFlop(int x, int y)
        : CircuitElement(TYPE_T_FLIPFLOP, x, y), q(false), lastClock(false) {
        // 输入引脚: T, CLK
        pins.push_back(std::make_unique<Pin>(x - 20, y - 10, true, this));   // T
        pins.push_back(std::make_unique<Pin>(x - 20, y + 10, true, this));   // CLK
        // 输出引脚: Q, Q'
        pins.push_back(std::make_unique<Pin>(x + 20, y - 10, false, this));  // Q
        pins.push_back(std::make_unique<Pin>(x + 20, y + 10, false, this));  // Q'
    }

    virtual void Draw(wxDC& dc) override {
        dc.SetPen(selected ? wxPen(*wxRED, 2) : wxPen(*wxBLACK, 1));
        dc.SetBrush(*wxWHITE_BRUSH);

        // 绘制T触发器符号
        dc.DrawRectangle(posX - 15, posY - 20, 30, 40);
        dc.DrawText("T", posX - 5, posY - 15);
        dc.DrawText("FF", posX - 10, posY - 5);

        // 绘制引脚标签
        dc.SetTextForeground(*wxBLACK);
        wxFont smallFont = dc.GetFont();
        smallFont.SetPointSize(6);
        dc.SetFont(smallFont);

        dc.DrawText("T", posX - 25, posY - 13);
        dc.DrawText("CLK", posX - 30, posY + 7);
        dc.DrawText("Q", posX + 15, posY - 13);
        dc.DrawText("Q'", posX + 12, posY + 13);

        // 显示当前状态
        dc.SetTextForeground(q ? *wxRED : *wxBLUE);
        wxString stateText = q ? "Q=1" : "Q=0";
        dc.DrawText(stateText, posX - 12, posY + 5);

        // 绘制引脚
        dc.SetPen(*wxBLACK_PEN);
        for (auto& pin : pins) {
            dc.DrawCircle(pin->GetX(), pin->GetY(), 3);
        }
    }

    virtual void Update() override {
        if (pins.size() < 4) return;

        bool t = pins[0]->GetValue();
        bool clock = pins[1]->GetValue();

        // 时钟上升沿触发
        if (clock && !lastClock) {
            if (t) {
                q = !q;  // 翻转
            }
            // T=0 时保持状态不变
        }
        lastClock = clock;

        // 设置输出
        pins[2]->SetValue(q);      // Q
        pins[3]->SetValue(!q);     // Q'
    }

    virtual std::vector<Pin*> GetPins() override {
        std::vector<Pin*> pinPtrs;
        for (auto& pin : pins) {
            pinPtrs.push_back(pin.get());
        }
        return pinPtrs;
    }

    virtual wxRect GetBoundingBox() const override {
        return wxRect(posX - 20, posY - 25, 40, 50);
    }

    virtual wxString GetName() const override {
        return "T_FLIPFLOP";
    }

    virtual wxString GetDisplayName() const override {
        return "T Flip-Flop";
    }

    virtual void Serialize(wxString& data) const override {
        data += wxString::Format("%d,%d,%d,%d,", type, posX, posY, q ? 1 : 0);
    }

    virtual void Deserialize(const wxString& data) override {
        wxStringTokenizer tokenizer(data, ",");
        if (tokenizer.CountTokens() >= 4) {
            long typeVal, x, y, qVal;
            tokenizer.GetNextToken().ToLong(&typeVal);
            tokenizer.GetNextToken().ToLong(&x);
            tokenizer.GetNextToken().ToLong(&y);
            tokenizer.GetNextToken().ToLong(&qVal);
            SetPosition(x, y);
            q = qVal != 0;
        }
    }

    virtual void GetProperties(wxPropertyGrid* pg) const override {
        pg->Append(new wxStringProperty("Type", "Type", GetDisplayName()));
        pg->Append(new wxIntProperty("X Position", "X", posX));
        pg->Append(new wxIntProperty("Y Position", "Y", posY));
        pg->Append(new wxBoolProperty("Q Output", "Q", q));
    }

    virtual void SetProperties(wxPropertyGrid* pg) override {
        wxVariant xVar = pg->GetPropertyValue("X");
        wxVariant yVar = pg->GetPropertyValue("Y");
        wxVariant qVar = pg->GetPropertyValue("Q");

        if (xVar.IsType("long") && yVar.IsType("long")) {
            SetPosition(xVar.GetLong(), yVar.GetLong());
        }
        if (qVar.IsType("bool")) {
            q = qVar.GetBool();
        }
    }

private:
    std::vector<std::unique_ptr<Pin>> pins;
    bool q;
    bool lastClock;
};

// 寄存器类（4位）
class RegisterElement : public CircuitElement {
public:
    RegisterElement(int x, int y)
        : CircuitElement(TYPE_REGISTER, x, y), lastClock(false) {
        // 初始化寄存器值为0
        for (int i = 0; i < 4; i++) {
            data[i] = false;
        }

        // 输入引脚: D0-D3, CLK, LOAD
        for (int i = 0; i < 4; i++) {
            pins.push_back(std::make_unique<Pin>(x - 30, y - 30 + i * 15, true, this)); // D0-D3
        }
        pins.push_back(std::make_unique<Pin>(x - 30, y + 30, true, this));  // CLK
        pins.push_back(std::make_unique<Pin>(x - 30, y + 45, true, this));  // LOAD

        // 输出引脚: Q0-Q3
        for (int i = 0; i < 4; i++) {
            pins.push_back(std::make_unique<Pin>(x + 30, y - 30 + i * 15, false, this)); // Q0-Q3
        }
    }

    virtual void Draw(wxDC& dc) override {
        dc.SetPen(selected ? wxPen(*wxRED, 2) : wxPen(*wxBLACK, 1));
        dc.SetBrush(*wxWHITE_BRUSH);

        // 绘制寄存器符号
        dc.DrawRectangle(posX - 25, posY - 35, 50, 70);
        dc.DrawText("REG", posX - 15, posY - 25);
        dc.DrawText("4-bit", posX - 18, posY - 10);

        // 绘制引脚标签
        dc.SetTextForeground(*wxBLACK);
        wxFont smallFont = dc.GetFont();
        smallFont.SetPointSize(6);
        dc.SetFont(smallFont);

        // 输入引脚标签
        for (int i = 0; i < 4; i++) {
            dc.DrawText(wxString::Format("D%d", i), posX - 35, y - 33 + i * 15);
        }
        dc.DrawText("CLK", posX - 35, y + 27);
        dc.DrawText("LD", posX - 35, y + 42);

        // 输出引脚标签
        for (int i = 0; i < 4; i++) {
            dc.DrawText(wxString::Format("Q%d", i), posX + 25, y - 33 + i * 15);
        }

        // 显示当前值
        dc.SetTextForeground(*wxBLUE);
        wxString valueText = GetValueString();
        dc.DrawText(valueText, posX - 20, posY + 10);

        // 绘制引脚
        dc.SetPen(*wxBLACK_PEN);
        for (auto& pin : pins) {
            dc.DrawCircle(pin->GetX(), pin->GetY(), 3);
        }
    }

    virtual void Update() override {
        if (pins.size() < 10) return;

        bool clock = pins[4]->GetValue();
        bool load = pins[5]->GetValue();

        // 时钟上升沿触发且LOAD为高时加载数据
        if (clock && !lastClock && load) {
            for (int i = 0; i < 4; i++) {
                data[i] = pins[i]->GetValue();
            }
        }
        lastClock = clock;

        // 设置输出
        for (int i = 0; i < 4; i++) {
            pins[6 + i]->SetValue(data[i]);
        }
    }

    virtual std::vector<Pin*> GetPins() override {
        std::vector<Pin*> pinPtrs;
        for (auto& pin : pins) {
            pinPtrs.push_back(pin.get());
        }
        return pinPtrs;
    }

    virtual wxRect GetBoundingBox() const override {
        return wxRect(posX - 30, posY - 40, 60, 80);
    }

    virtual wxString GetName() const override {
        return "REGISTER";
    }

    virtual wxString GetDisplayName() const override {
        return "4-bit Register";
    }

    virtual void Serialize(wxString& data) const override {
        data += wxString::Format("%d,%d,%d,", type, posX, posY);
        for (int i = 0; i < 4; i++) {
            data += wxString::Format("%d,", this->data[i] ? 1 : 0);
        }
    }

    virtual void Deserialize(const wxString& data) override {
        wxStringTokenizer tokenizer(data, ",");
        if (tokenizer.CountTokens() >= 7) {
            long typeVal, x, y;
            tokenizer.GetNextToken().ToLong(&typeVal);
            tokenizer.GetNextToken().ToLong(&x);
            tokenizer.GetNextToken().ToLong(&y);
            SetPosition(x, y);

            for (int i = 0; i < 4; i++) {
                long val;
                tokenizer.GetNextToken().ToLong(&val);
                this->data[i] = val != 0;
            }
        }
    }

    virtual void GetProperties(wxPropertyGrid* pg) const override {
        pg->Append(new wxStringProperty("Type", "Type", GetDisplayName()));
        pg->Append(new wxIntProperty("X Position", "X", posX));
        pg->Append(new wxIntProperty("Y Position", "Y", posY));
        pg->Append(new wxStringProperty("Value", "Value", GetValueString()));
    }

    virtual void SetProperties(wxPropertyGrid* pg) override {
        wxVariant xVar = pg->GetPropertyValue("X");
        wxVariant yVar = pg->GetPropertyValue("Y");

        if (xVar.IsType("long") && yVar.IsType("long")) {
            SetPosition(xVar.GetLong(), yVar.GetLong());
        }
    }

private:
    std::vector<std::unique_ptr<Pin>> pins;
    bool data[4];
    bool lastClock;
    int y;

    wxString GetValueString() const {
        int value = 0;
        for (int i = 0; i < 4; i++) {
            if (data[i]) {
                value |= (1 << i);
            }
        }
        return wxString::Format("0x%X", value);
    }
};
#endif
