#pragma once
#ifndef PROPERTIESPANEL_H
#define PROPERTIESPANEL_H                  

// 属性面板类
class PropertiesPanel : public wxPanel {
public:
    // 构造函数，接收父窗口和电路画布指针
    PropertiesPanel(wxWindow* parent, CircuitCanvas* canvas)
        : wxPanel(parent), canvas(canvas) {

        // 创建垂直排列的布局管理器
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        // 创建属性网格控件
        pg = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, wxSize(300, 200),
            wxPG_DEFAULT_STYLE | wxPG_BOLD_MODIFIED);
        mainSizer->Add(pg, 1, wxEXPAND);

        // 创建连接信息文本框（只读，用于显示详细连接信息）
        connectionInfoText = new wxTextCtrl(this, wxID_ANY, "",
            wxDefaultPosition, wxDefaultSize,
            wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxTE_WORDWRAP);
        connectionInfoText->SetMinSize(wxSize(300, 150));
        connectionInfoText->SetBackgroundColour(wxColour(240, 240, 240)); // 浅灰色背景
        connectionInfoText->SetFont(wxFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

        mainSizer->Add(new wxStaticText(this, wxID_ANY, "Connection Information:"), 0, wxALL, 5);
        mainSizer->Add(connectionInfoText, 0, wxEXPAND | wxALL, 5);

        SetSizer(mainSizer);

        // 绑定事件
        pg->Bind(wxEVT_PG_CHANGED, &PropertiesPanel::OnPropertyChanged, this);
        canvas->Bind(wxEVT_LEFT_DOWN, &PropertiesPanel::OnCanvasSelectionChange, this);

        // 新增：绑定画布刷新事件，确保连接信息实时更新
        canvas->Bind(wxEVT_PAINT, &PropertiesPanel::OnCanvasRefresh, this);
    }

    // 更新属性面板内容
    void UpdateProperties() {
        pg->Clear();
        UpdateConnectionInfo(); // 更新连接信息

        if (canvas->HasSelectedWire()) {
            // 显示导线信息
            Wire* wire = canvas->GetSelectedWire();
            if (wire) {
                pg->Append(new wxStringProperty("Type", "Type", "Wire Connection"));
                pg->Append(new wxStringProperty("Start Pin", "Start",
                    wxString::Format("(%d, %d)",
                        wire->GetStartPin()->GetX(),
                        wire->GetStartPin()->GetY())));
                pg->Append(new wxStringProperty("End Pin", "End",
                    wxString::Format("(%d, %d)",
                        wire->GetEndPin()->GetX(),
                        wire->GetEndPin()->GetY())));
                pg->Append(new wxBoolProperty("Signal Value", "Value",
                    wire->GetStartPin()->GetValue()));

                // 添加导线详细信息
                Pin* startPin = wire->GetStartPin();
                Pin* endPin = wire->GetEndPin();
                if (startPin && endPin) {
                    pg->Append(new wxStringProperty("From", "FromElement",
                        GetPinElementInfo(startPin)));
                    pg->Append(new wxStringProperty("To", "ToElement",
                        GetPinElementInfo(endPin)));
                }
            }
        }
        else if (canvas->GetSelectedElement()) {
            canvas->GetSelectedElement()->GetProperties(pg);

            // 为选中的元件添加连接信息到属性网格
            AddElementConnectionInfo(canvas->GetSelectedElement());
        }
        else {
            pg->Append(new wxStringProperty("No Selection", "None",
                "Select an element or wire to edit properties"));
        }
    }

private:
    // 属性改变事件处理函数
    void OnPropertyChanged(wxPropertyGridEvent& event) {
        CircuitElement* selected = canvas->GetSelectedElement();
        if (selected) {
            selected->SetProperties(pg);
            canvas->Refresh();
            UpdateConnectionInfo(); // 属性改变后更新连接信息
        }
    }

    // 画布选择改变事件处理函数
    void OnCanvasSelectionChange(wxMouseEvent& event) {
        UpdateProperties();
        event.Skip();
    }

    // 新增：画布刷新事件处理
    void OnCanvasRefresh(wxPaintEvent& event) {
        UpdateConnectionInfo(); // 画布刷新时也更新连接信息
        event.Skip();
    }

    // 更新连接信息文本框内容
    void UpdateConnectionInfo() {
        wxString info;

        if (canvas->HasSelectedWire()) {
            Wire* wire = canvas->GetSelectedWire();
            if (wire) {
                info = GetWireConnectionInfo(wire);
            }
        }
        else if (canvas->GetSelectedElement()) {
            info = GetElementConnectionInfo(canvas->GetSelectedElement());
        }
        else if (canvas->HasSelectedElements()) {
            // 多选情况
            info = GetMultipleSelectionInfo();
        }
        else {
            info = "No element selected.\n\nTip: Select an element or wire to view connection details.";
        }

        connectionInfoText->SetValue(info);
    }

    // 获取导线连接信息
    wxString GetWireConnectionInfo(Wire* wire) {
        wxString info;
        Pin* startPin = wire->GetStartPin();
        Pin* endPin = wire->GetEndPin();

        if (startPin && endPin) {
            info = wxString::Format(" WIRE CONNECTION\n\n"
                "From: %s\n"
                "To: %s\n"
                "Signal: %s\n"
                "Type: %s → %s\n\n"
                "This wire carries the signal from the output pin to the input pin.",
                GetPinDetailedInfo(startPin).c_str(),
                GetPinDetailedInfo(endPin).c_str(),
                startPin->GetValue() ? "HIGH (1)" : "LOW (0)",
                startPin->IsInput() ? "INPUT" : "OUTPUT",
                endPin->IsInput() ? "INPUT" : "OUTPUT");
        }

        return info;
    }

    // 获取元件连接信息
    wxString GetElementConnectionInfo(CircuitElement* element) {
        wxString info;
        info << element->GetDisplayName().Upper() << " CONNECTION INFO\n\n";

        auto pins = element->GetPins();
        std::vector<Pin*> inputPins;
        std::vector<Pin*> outputPins;

        // 分类引脚
        for (auto pin : pins) {
            if (pin->IsInput()) {
                inputPins.push_back(pin);
            }
            else {
                outputPins.push_back(pin);
            }
        }

        // 输入连接信息
        info << "INPUT CONNECTIONS:\n";
        if (inputPins.empty()) {
            info << "  No input pins\n";
        }
        else {
            for (size_t i = 0; i < inputPins.size(); ++i) {
                Pin* pin = inputPins[i];
                Wire* connectedWire = pin->GetConnectedWire();
                if (connectedWire) {
                    Pin* sourcePin = (connectedWire->GetStartPin() == pin) ?
                        connectedWire->GetEndPin() : connectedWire->GetStartPin();
                    info << wxString::Format("  %zu. %s ← %s\n",
                        i + 1, GetPinShortInfo(pin).c_str(), GetPinElementInfo(sourcePin).c_str());
                }
                else {
                    info << wxString::Format("  %zu. %s : NOT CONNECTED\n",
                        i + 1, GetPinShortInfo(pin).c_str());
                }
            }
        }

        info << "\nOUTPUT CONNECTIONS:\n";
        if (outputPins.empty()) {
            info << "  No output pins\n";
        }
        else {
            for (size_t i = 0; i < outputPins.size(); ++i) {
                Pin* pin = outputPins[i];
                Wire* connectedWire = pin->GetConnectedWire();
                if (connectedWire) {
                    Pin* targetPin = (connectedWire->GetStartPin() == pin) ?
                        connectedWire->GetEndPin() : connectedWire->GetStartPin();
                    info << wxString::Format("  %zu. %s → %s\n",
                        i + 1, GetPinShortInfo(pin).c_str(), GetPinElementInfo(targetPin).c_str());
                }
                else {
                    info << wxString::Format("  %zu. %s : NOT CONNECTED\n",
                        i + 1, GetPinShortInfo(pin).c_str());
                }
            }
        }

        // 添加状态信息
        info << "\nCURRENT STATE:\n";
        if (element->GetType() == TYPE_INPUT) {
            InputOutput* io = dynamic_cast<InputOutput*>(element);
            if (io) {
                info << wxString::Format("  Value: %s\n", io->GetValue() ? "HIGH (1)" : "LOW (0)");
            }
        }
        else if (element->GetType() == TYPE_OUTPUT) {
            InputOutput* io = dynamic_cast<InputOutput*>(element);
            if (io) {
                info << wxString::Format("  Value: %s\n", io->GetValue() ? "HIGH (1)" : "LOW (0)");
            }
        }
        else if (element->GetType() == TYPE_CLOCK) {
            ClockElement* clock = dynamic_cast<ClockElement*>(element);
            if (clock) {
                info << wxString::Format("  Frequency: %d Hz\n", clock->GetFrequency());
                //info << wxString::Format("  Current: %s\n", clock->GetValue() ? "HIGH" : "LOW");
            }
        }

        return info;
    }

    // 获取多选信息
    wxString GetMultipleSelectionInfo() {
        int selectedCount = 0;
        for (auto& element : canvas->GetElements()) {
            if (element->IsSelected()) {
                selectedCount++;
            }
        }

        return wxString::Format("MULTIPLE SELECTION\n\n"
            " Selected Elements: %d\n\n"
            "Tip: Select individual elements to view detailed connection information.",
            selectedCount);
    }

    // 为选中的元件添加连接信息到属性网格
    void AddElementConnectionInfo(CircuitElement* element) {
        auto pins = element->GetPins();
        int inputConnections = 0;
        int outputConnections = 0;

        // 统计连接数
        for (auto pin : pins) {
            if (pin->GetConnectedWire()) {
                if (pin->IsInput()) {
                    inputConnections++;
                }
                else {
                    outputConnections++;
                }
            }
        }

        // 添加连接统计到属性网格
        //pg->Append(new wxCategoryProperty("Connection Statistics"));
        pg->Append(new wxIntProperty("Input Connections", "InputConnections", inputConnections));
        pg->Append(new wxIntProperty("Output Connections", "OutputConnections", outputConnections));
        pg->Append(new wxIntProperty("Total Pins", "TotalPins", (int)pins.size()));

        // 设置连接统计属性为只读
        pg->SetPropertyReadOnly("InputConnections");
        pg->SetPropertyReadOnly("OutputConnections");
        pg->SetPropertyReadOnly("TotalPins");
    }

    // 辅助函数：获取引脚的详细信息
    wxString GetPinDetailedInfo(Pin* pin) {
        if (!pin) return "Unknown Pin";
        CircuitElement* parent = pin->GetParent();
        if (!parent) return "Orphan Pin";

        return wxString::Format("%s [%s] at (%d,%d)",
            parent->GetDisplayName().c_str(),
            pin->IsInput() ? "INPUT" : "OUTPUT",
            pin->GetX(), pin->GetY());
    }

    // 辅助函数：获取引脚的简短信息
    wxString GetPinShortInfo(Pin* pin) {
        if (!pin) return "Unknown";
        CircuitElement* parent = pin->GetParent();
        if (!parent) return "Orphan";

        return wxString::Format("%s(%s)",
            parent->GetDisplayName().c_str(),
            pin->IsInput() ? "IN" : "OUT");
    }

    // 辅助函数：获取引脚所属元件信息
    wxString GetPinElementInfo(Pin* pin) {
        if (!pin) return "Unknown";
        CircuitElement* parent = pin->GetParent();
        if (!parent) return "Orphan Pin";

        return parent->GetDisplayName();
    }

    // 私有成员变量
    wxPropertyGrid* pg;           // 属性网格控件指针
    wxTextCtrl* connectionInfoText; // 新增：连接信息文本框
    CircuitCanvas* canvas;        // 电路画布指针
};
#endif
