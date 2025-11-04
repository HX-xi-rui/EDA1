#pragma once
#ifndef CIRCUITCANVAS_H
#define CIRCUITCANVAS_H
      
#include <random>  
#include <wx/dcbuffer.h>           // 双缓冲绘图            
#include "CircuitElement.h"
#include "Wire.h"
#include "Gate.h"
#include "InputOutput.h"
#include "Sequence.h"

// 前向声明
class TruthTableDialog;
class ElementTreeCtrl;

// 画布类 
class CircuitCanvas : public wxScrolledWindow {
public:
    // 构造函数
    CircuitCanvas(wxWindow* parent) : wxScrolledWindow(parent, wxID_ANY),
        currentTool(TYPE_SELECT), wiringMode(false), selectedElement(nullptr),
        startPin(nullptr), simulating(false), showGrid(true), zoomLevel(1.0),
        autoPlaceMode(false), autoPlaceType(TYPE_SELECT),
        virtualSize(2000, 2000), isRestoringState(false),
        pasteOffset(0, 0), pasteCount(0), selectedWire(nullptr),
        connectionScrollPos(0), maxConnectionWidth(0) {  // 新增滚动相关变量

        // 设置滚动条
        SetScrollRate(10, 10);  // 设置滚动步长
        SetVirtualSize(virtualSize);  // 设置虚拟大小

        // 设置双缓冲避免闪烁
        SetBackgroundStyle(wxBG_STYLE_PAINT);

        // 绑定各种事件处理函数
        Bind(wxEVT_PAINT, &CircuitCanvas::OnPaint, this);
        Bind(wxEVT_LEFT_DOWN, &CircuitCanvas::OnLeftDown, this);
        Bind(wxEVT_LEFT_UP, &CircuitCanvas::OnLeftUp, this);
        Bind(wxEVT_MOTION, &CircuitCanvas::OnMouseMove, this);
        Bind(wxEVT_RIGHT_DOWN, &CircuitCanvas::OnRightDown, this);
        Bind(wxEVT_MOUSEWHEEL, &CircuitCanvas::OnMouseWheel, this);
        Bind(wxEVT_KEY_DOWN, &CircuitCanvas::OnKeyDown, this);
        Bind(wxEVT_SIZE, &CircuitCanvas::OnSize, this);
        Bind(wxEVT_MENU, &CircuitCanvas::OnContextMenu, this);

        // 绑定滚动事件
        Bind(wxEVT_SCROLLWIN_THUMBTRACK, &CircuitCanvas::OnScroll, this);
        Bind(wxEVT_SCROLLWIN_THUMBRELEASE, &CircuitCanvas::OnScroll, this);

        SetFocus();  // 设置焦点以接收键盘事件
    }

    // 更新状态栏显示
    void UpdateStatusBar() {
        wxWindow* topWindow = wxGetTopLevelParent(this);
        if (topWindow && topWindow->IsKindOf(CLASSINFO(wxFrame))) {
            wxFrame* frame = static_cast<wxFrame*>(topWindow);
            wxStatusBar* statusBar = frame->GetStatusBar();
            if (statusBar) {
                wxString connectionInfo;
                if (selectedElement) {
                    connectionInfo = GetSelectedElementConnectionInfo();
                }
                else if (selectedWire) {
                    connectionInfo = "Wire selected - connects " +
                        GetPinInfo(selectedWire->GetStartPin()) + " to " +
                        GetPinInfo(selectedWire->GetEndPin());
                }
                else {
                    connectionInfo = "Ready";
                }

                // 计算连接信息的宽度
                wxClientDC dc(statusBar);
                dc.SetFont(statusBar->GetFont());
                wxSize textSize = dc.GetTextExtent(connectionInfo);
                maxConnectionWidth = textSize.GetWidth();

                // 应用滚动位置
                if (maxConnectionWidth > 400) { // 如果文本宽度超过状态栏宽度
                    if (connectionScrollPos > maxConnectionWidth - 400) {
                        connectionScrollPos = maxConnectionWidth - 400;
                    }
                    if (connectionScrollPos < 0) {
                        connectionScrollPos = 0;
                    }

                    // 显示滚动后的文本
                    statusBar->SetStatusText(connectionInfo.Mid(connectionScrollPos / 8), 0);

                    // 在状态栏显示滚动提示
                    statusBar->SetStatusText("← Scroll →", 1);
                }
                else {
                    statusBar->SetStatusText(connectionInfo, 0);
                    statusBar->SetStatusText("", 1); // 清除滚动提示
                    connectionScrollPos = 0; // 重置滚动位置
                }
            }
        }
    }

    // 处理连接信息滚动
    void ScrollConnectionInfo(int direction) {
        if (maxConnectionWidth <= 400) return; // 不需要滚动

        connectionScrollPos += direction * 20; // 每次滚动20像素

        // 限制滚动范围
        if (connectionScrollPos < 0) {
            connectionScrollPos = 0;
        }
        if (connectionScrollPos > maxConnectionWidth - 400) {
            connectionScrollPos = maxConnectionWidth - 400;
        }

        UpdateStatusBar();
    }

    // 辅助函数：获取引脚信息
    wxString GetPinInfo(Pin* pin) const {
        if (!pin) return "Unknown";
        CircuitElement* parent = pin->GetParent();
        if (!parent) return "Orphan pin";

        return parent->GetDisplayName() + " (" + (pin->IsInput() ? "input" : "output") + ")";
    }

    void NotifyStateChange() {
        wxWindow* topWindow = wxGetTopLevelParent(this);
        if (topWindow && topWindow->IsKindOf(CLASSINFO(wxFrame))) {
            wxFrame* frame = static_cast<wxFrame*>(topWindow);
            if (wxToolBar* toolBar = frame->GetToolBar()) {
                toolBar->EnableTool(wxID_UNDO, CanUndo());
                toolBar->EnableTool(wxID_REDO, CanRedo());
            }
        }
    }

    // 设置选中元件
    void SetSelectedElement(CircuitElement* element) {
        if (selectedElement) {
            selectedElement->SetSelected(false);
        }
        selectedElement = element;
        if (selectedElement) {
            selectedElement->SetSelected(true);
        }
        connectionScrollPos = 0; // 重置滚动位置
        UpdateStatusBar();
    }

    // 删除选中的导线
    void DeleteSelectedWire() {
        if (!selectedWire || isRestoringState) return;

        // 显示确认对话框
        wxMessageDialog dialog(GetParent(),
            "Are you sure you want to delete the selected wire?",
            "Confirm Delete",
            wxYES_NO | wxICON_QUESTION);

        if (dialog.ShowModal() == wxID_YES) {
            // 序列化导线数据用于撤销
            wxString serializedData = SerializeWire(selectedWire);

            // 找到要删除的导线
            auto it = std::find_if(wires.begin(), wires.end(),
                [this](const std::unique_ptr<Wire>& w) {
                    return w.get() == selectedWire;
                });

            if (it != wires.end()) {
                // 保存要删除的导线
                std::unique_ptr<Wire> wireToDelete = std::move(*it);

                // 记录删除操作到撤销栈
                auto operation = std::make_unique<DeleteWireOperation>(
                    std::move(wireToDelete), serializedData);

                // 限制历史记录数量
                if (undoStack.size() >= MAX_HISTORY) {
                    undoStack.erase(undoStack.begin());
                }
                undoStack.push_back(std::move(operation));
                redoStack.clear();
                UpdateUndoRedoStatus();

                // 从导线列表中移除
                wires.erase(it);

                // 清除选中状态
                selectedWire = nullptr;

                // 更新电路状态
                UpdateCircuit();
                Refresh();

                // 更新状态栏
                UpdateStatusBar();
            }
        }
    }

    wxString GetSelectedElementConnectionInfo() const {
        if (!selectedElement) return "No element selected";

        wxString info;
        info << "Selected: " << selectedElement->GetDisplayName() << " | ";

        // 获取所有引脚
        auto pins = selectedElement->GetPins();

        // 分析输入连接（上一个门）
        info << "Inputs from: ";
        bool hasPrevious = false;
        for (auto pin : pins) {
            if (pin->IsInput()) {
                Wire* connectedWire = pin->GetConnectedWire();
                if (connectedWire) {
                    Pin* startPin = connectedWire->GetStartPin();
                    if (startPin && startPin->GetParent() != selectedElement) {
                        if (hasPrevious) info << ", ";
                        info << startPin->GetParent()->GetDisplayName();
                        hasPrevious = true;
                    }
                }
            }
        }
        if (!hasPrevious) info << "None";

        // 分析输出连接（下一个门）
        info << " | Outputs to: ";
        bool hasNext = false;
        for (auto pin : pins) {
            if (!pin->IsInput()) {
                Wire* connectedWire = pin->GetConnectedWire();
                if (connectedWire) {
                    Pin* endPin = connectedWire->GetEndPin();
                    if (endPin && endPin->GetParent() != selectedElement) {
                        if (hasNext) info << ", ";
                        info << endPin->GetParent()->GetDisplayName();
                        hasNext = true;
                    }
                }
            }
        }
        if (!hasNext) info << "None";

        return info;
    }

    // 检查是否有选中的导线
    bool HasSelectedWire() const { return selectedWire != nullptr; }

    // 获取选中的导线
    Wire* GetSelectedWire() const { return selectedWire; }

    // 删除所有选中的元件
    void DeleteAllSelectedElements() {
        if (isRestoringState) return;

        // 收集要删除的元件（使用指针，避免迭代器失效）
        std::vector<CircuitElement*> elementsToDelete;
        for (auto& element : elements) {
            if (element->IsSelected()) {
                elementsToDelete.push_back(element.get());
            }
        }

        if (elementsToDelete.empty()) return;

        // 显示确认对话框
        wxMessageDialog dialog(GetParent(),
            wxString::Format("Are you sure you want to delete %zu selected elements?", elementsToDelete.size()),
            "Confirm Delete",
            wxYES_NO | wxICON_QUESTION);

        if (dialog.ShowModal() != wxID_YES) {
            return;
        }

        // 为撤销操作收集数据
        std::vector<std::pair<std::unique_ptr<CircuitElement>, wxString>> deletedElements;

        // 删除收集到的元件
        for (auto element : elementsToDelete) {
            // 序列化元件数据用于撤销
            wxString serializedData = SerializeElement(element);

            // 找到要删除的元件
            auto it = std::find_if(elements.begin(), elements.end(),
                [element](const std::unique_ptr<CircuitElement>& elem) {
                    return elem.get() == element;
                });

            if (it != elements.end()) {
                // 先断开所有引脚连接
                auto pins = element->GetPins();
                for (auto pin : pins) {
                    // 找到并删除连接到该引脚的所有导线
                    for (auto wireIt = wires.begin(); wireIt != wires.end(); ) {
                        if ((*wireIt)->GetStartPin() == pin || (*wireIt)->GetEndPin() == pin) {
                            wireIt = wires.erase(wireIt);
                        }
                        else {
                            ++wireIt;
                        }
                    }
                    // 清除引脚的连接
                    pin->SetConnectedWire(nullptr);
                }

                // 保存要删除的元件和序列化数据
                deletedElements.push_back(std::make_pair(std::move(*it), serializedData));

                // 从元素列表中移除
                elements.erase(it);
            }
        }

        // 记录批量删除操作到撤销栈
        if (!deletedElements.empty() && !isRestoringState) {
            auto operation = std::make_unique<BatchDeleteOperation>(std::move(deletedElements));

            // 限制历史记录数量
            if (undoStack.size() >= MAX_HISTORY) {
                undoStack.erase(undoStack.begin());
            }
            undoStack.push_back(std::move(operation));
            redoStack.clear();
            UpdateUndoRedoStatus();
        }

        // 清除选中状态
        selectedElement = nullptr;
        connectionScrollPos = 0; // 重置滚动位置

        // 更新电路状态
        UpdateCircuit();
        Refresh();

        // 更新状态栏
        UpdateStatusBar();
    }

    // 在指定位置创建元件，支持时序元件
    void CreateElementAtPosition(ElementType type, const wxPoint& pos) {
        std::unique_ptr<CircuitElement> newElement;

        // 根据元件类型创建相应的元件对象
        if (type >= TYPE_AND && type <= TYPE_NOR) {  // 逻辑门元件
            newElement = std::make_unique<Gate>(type, pos.x, pos.y);
        }
        else if (type == TYPE_INPUT || type == TYPE_OUTPUT) {  // 输入输出元件
            newElement = std::make_unique<InputOutput>(type, pos.x, pos.y);
        }
        else if (type == TYPE_CLOCK) {  // 时钟元件
            newElement = std::make_unique<ClockElement>(pos.x, pos.y);
        }
        else if (type == TYPE_RS_FLIPFLOP) {  // RS触发器 - 新增
            newElement = std::make_unique<RSFlipFlop>(pos.x, pos.y);
        }
        else if (type == TYPE_D_FLIPFLOP) {  // D触发器
            newElement = std::make_unique<DFlipFlop>(pos.x, pos.y);
        }
        else if (type == TYPE_JK_FLIPFLOP) {  // JK触发器
            newElement = std::make_unique<JKFlipFlop>(pos.x, pos.y);
        }
        else if (type == TYPE_T_FLIPFLOP) {  // T触发器
            newElement = std::make_unique<TFlipFlop>(pos.x, pos.y);
        }
        else if (type == TYPE_REGISTER) {  // 寄存器
            newElement = std::make_unique<RegisterElement>(pos.x, pos.y);
        }

        // 如果成功创建元件，将其添加到元件列表
        if (newElement) {
            CircuitElement* elementPtr = newElement.get();
            elements.push_back(std::move(newElement));

            // 记录添加元件操作（用于撤销/重做）
            if (!isRestoringState) {
                wxString serializedData = SerializeElement(elementPtr);
                auto operation = std::make_unique<AddElementOperation>(serializedData);
                undoStack.push_back(std::move(operation));

                // 限制历史记录数量
                if (undoStack.size() > MAX_HISTORY) {
                    undoStack.erase(undoStack.begin());
                }

                redoStack.clear();
                UpdateUndoRedoStatus();
            }

            Refresh();  // 刷新显示
        }
    }

    // 设置自动放置模式
    void SetAutoPlaceMode(ElementType type) {
        autoPlaceMode = (type != TYPE_SELECT && type != TYPE_WIRE);  // 非选择和连线工具时启用
        autoPlaceType = type;  // 设置要自动放置的类型
        currentTool = type;    // 设置当前工具
        wiringMode = (type == TYPE_WIRE);  // 如果是连线工具则设置连线模式

        if (autoPlaceMode) {
            // 设置鼠标光标为十字准星
            SetCursor(wxCursor(wxCURSOR_CROSS));
            // 显示提示信息
            wxWindow* topWindow = wxGetTopLevelParent(this);
            if (topWindow) {
                wxStatusBar* statusBar = static_cast<wxFrame*>(topWindow)->GetStatusBar();
                if (statusBar) {
                    statusBar->SetStatusText("Click on canvas to place " + GetToolName(type));
                }
            }
        }
        else {
            SetCursor(wxCursor(wxCURSOR_ARROW));  // 恢复正常光标
        }

        Refresh();  // 刷新显示
    }

    // 上下文菜单事件处理
    void OnContextMenu(wxCommandEvent& event) {
        switch (event.GetId()) {
        case wxID_DELETE:
            DeleteSelectedElement();  // 删除选中元件
            break;
        case wxID_PROPERTIES:
            // 属性菜单（暂未实现）
            break;
        default:
            break;
        }
    }

    // 设置当前工具
    void SetCurrentTool(ElementType tool) {
        currentTool = tool;
        wiringMode = (tool == TYPE_WIRE);  // 设置连线模式
        autoPlaceMode = false; // 重置自动放置模式

        if (!wiringMode) {
            startPin = nullptr;  // 清除连线起始引脚
        }

        // 设置合适的鼠标光标
        if (tool == TYPE_TOGGLE_VALUE) {
            SetCursor(wxCursor(wxCURSOR_HAND));  // 切换值工具使用手型光标
        }
        else {
            SetCursor(wxCursor(wxCURSOR_ARROW));  // 其他工具使用箭头光标
        }
        Refresh();  // 刷新显示
    }

    // 开始仿真
    void StartSimulation() {
        simulating = true;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 1);

        // 为所有输入元件随机设置初始值
        for (auto& element : elements) {
            if (element->GetType() == TYPE_INPUT) {
                InputOutput* io = dynamic_cast<InputOutput*>(element.get());
                if (io) {
                    io->SetValue(dis(gen) == 0);  // 随机设置0或1
                }
            }
        }
        UpdateCircuit();  // 更新电路状态
        Refresh();  // 刷新显示
    }

    // 获取元件列表
    const std::vector<std::unique_ptr<CircuitElement>>& GetElements() const {
        return elements;
    }

    // 获取导线列表
    const std::vector<std::unique_ptr<Wire>>& GetWires() const {
        return wires;
    }

    // 停止仿真
    void StopSimulation() {
        simulating = false;

        // 将所有输入元件的值设为0（false）
        for (auto& element : elements) {
            if (element->GetType() == TYPE_INPUT) {
                InputOutput* input = dynamic_cast<InputOutput*>(element.get());
                if (input) {
                    input->SetValue(false);  // 重置为0
                }
            }
        }

        // 更新电路状态以反映所有输入为0的情况
        UpdateCircuit();
        Refresh();  // 刷新显示
    }

    // 更新整个电路状态
    void UpdateCircuit() {
        // 多次迭代确保信号稳定传播
        for (int i = 0; i < 5; ++i) {
            // 先更新输入元件
            for (auto& element : elements) {
                if (element->GetType() == TYPE_INPUT) {
                    element->Update();
                }
            }

            // 更新导线传递信号
            for (auto& wire : wires) {
                wire->Update();
            }

            // 更新逻辑门元件
            for (auto& element : elements) {
                if (element->GetType() >= TYPE_AND && element->GetType() <= TYPE_NOR) {
                    element->Update();
                }
            }

            // 再次更新导线
            for (auto& wire : wires) {
                wire->Update();
            }

            // 更新输出元件
            for (auto& element : elements) {
                if (element->GetType() == TYPE_OUTPUT) {
                    element->Update();
                }
            }
        }
    }

    // 清空画布
    void Clear() {
        elements.clear();  // 清空元件
        wires.clear();     // 清空导线
        virtualPins.clear(); // 新增：清空虚拟引脚
        selectedElement = nullptr;  // 清除选中
        startPin = nullptr;         // 清除连线起始引脚
        autoPlaceMode = false;      // 关闭自动放置模式
        undoStack.clear();          // 清空撤销栈
        redoStack.clear();          // 清空重做栈
        connectionScrollPos = 0;    // 重置滚动位置
        maxConnectionWidth = 0;     // 重置最大宽度
        Refresh();  // 刷新显示
    }

    // 保存电路图
    bool SaveCircuit(const wxString& filename) {
        wxFile file;
        if (file.Create(filename, true)) {
            wxString data;

            // 保存所有元件
            for (auto& element : elements) {
                element->Serialize(data);
                data += "\n";
            }

            // 保存所有导线 
            for (auto& wire : wires) {
                Pin* startPin = wire->GetStartPin();
                Pin* endPin = wire->GetEndPin();

                if (startPin && endPin) {
                    // 保存起始引脚和结束引脚的坐标
                    data += wxString::Format("WIRE,%d,%d,%d,%d\n",
                        startPin->GetX(), startPin->GetY(),
                        endPin->GetX(), endPin->GetY());
                }
            }

            file.Write(data);
            file.Close();
            return true;
        }
        return false;
    }

    // 加载电路图
    bool LoadCircuit(const wxString& filename) {
        wxFile file;
        if (file.Open(filename)) {
            wxString data;
            file.ReadAll(&data);
            file.Close();

            Clear();  // 清空当前画布

            // 第一遍：加载所有元件
            wxStringTokenizer lines(data, "\n");
            while (lines.HasMoreTokens()) {
                wxString line = lines.GetNextToken().Trim();
                if (line.empty()) continue;

                wxStringTokenizer tokens(line, ",");
                if (tokens.HasMoreTokens()) {
                    wxString firstToken = tokens.GetNextToken();

                    if (firstToken == "WIRE") {
                        // 导线在第二遍处理
                        continue;
                    }
                    else {
                        long typeVal;
                        if (firstToken.ToLong(&typeVal)) {
                            ElementType type = static_cast<ElementType>(typeVal);

                            // 根据类型创建相应的元件
                            if (type >= TYPE_AND && type <= TYPE_NOR) {
                                auto gate = std::make_unique<Gate>(type, 0, 0);
                                gate->Deserialize(line);
                                elements.push_back(std::move(gate));
                            }
                            else if (type == TYPE_INPUT || type == TYPE_OUTPUT) {
                                auto io = std::make_unique<InputOutput>(type, 0, 0);
                                io->Deserialize(line);
                                elements.push_back(std::move(io));
                            }
                        }
                    }
                }
            }

            // 第二遍：重建导线连接
            lines = wxStringTokenizer(data, "\n");
            while (lines.HasMoreTokens()) {
                wxString line = lines.GetNextToken().Trim();
                if (line.empty()) continue;

                wxStringTokenizer tokens(line, ",");
                if (tokens.HasMoreTokens()) {
                    wxString firstToken = tokens.GetNextToken();

                    if (firstToken == "WIRE") {
                        if (tokens.CountTokens() >= 4) {
                            long startX, startY, endX, endY;
                            tokens.GetNextToken().ToLong(&startX);
                            tokens.GetNextToken().ToLong(&startY);
                            tokens.GetNextToken().ToLong(&endX);
                            tokens.GetNextToken().ToLong(&endY);

                            // 通过坐标查找对应的引脚
                            Pin* startPin = FindPinByPosition(startX, startY);
                            Pin* endPin = FindPinByPosition(endX, endY);

                            if (startPin && endPin && startPin->IsInput() != endPin->IsInput()) {
                                // 确保连接方向正确：输出引脚 -> 输入引脚
                                if (!startPin->IsInput() && endPin->IsInput()) {
                                    wires.push_back(std::make_unique<Wire>(startPin, endPin));
                                }
                                else if (startPin->IsInput() && !endPin->IsInput()) {
                                    wires.push_back(std::make_unique<Wire>(endPin, startPin));
                                }
                            }
                        }
                    }
                }
            }

            UpdateCircuit();
            Refresh();
            return true;
        }
        return false;
    }

    // 切换网格显示
    void ToggleGrid() {
        showGrid = !showGrid;
        Refresh();  // 刷新显示
    }

    // 删除所有元件
    void DeleteAll() {
        if (wxMessageBox("Are you sure you want to delete all elements?", "Confirm Delete All",
            wxYES_NO | wxICON_QUESTION, GetParent()) == wxYES) {
            Clear();
        }
    }

    // 放大
    void ZoomIn() {
        wxPoint oldCenter = GetViewCenter();
        zoomLevel *= 1.2;  // 增加缩放级别
        UpdateScrollbars();
        CenterView(oldCenter);
        Refresh();  // 刷新显示
    }

    // 缩小
    void ZoomOut() {
        wxPoint oldCenter = GetViewCenter();
        zoomLevel /= 1.2;  // 减小缩放级别
        UpdateScrollbars();
        CenterView(oldCenter);
        Refresh();  // 刷新显示
    }

    // 重置缩放
    void ResetZoom() {
        wxPoint oldCenter = GetViewCenter();
        zoomLevel = 1.0;  // 恢复原始缩放
        UpdateScrollbars();
        CenterView(oldCenter);
        Refresh();  // 刷新显示
    }

    // 获取缩放级别
    double GetZoomLevel() const { return zoomLevel; }

    // 获取选中元件
    CircuitElement* GetSelectedElement() const { return selectedElement; }

    // 重命名选中元件
    void RenameSelectedElement(const wxString& newName) {
        if (selectedElement) {
            InputOutput* io = dynamic_cast<InputOutput*>(selectedElement);
            if (io) {
                io->SetName(newName);  // 设置新名称
                Refresh();  // 刷新显示
            }
        }
    }

    // 显示真值表
    void ShowTruthTable();

    // 获取所有输入引脚
    std::vector<InputOutput*> GetInputPins() const {
        std::vector<InputOutput*> inputs;
        for (auto& element : elements) {
            if (element->GetType() == TYPE_INPUT) {
                inputs.push_back(dynamic_cast<InputOutput*>(element.get()));
            }
        }
        return inputs;
    }

    // 获取所有输出引脚
    std::vector<InputOutput*> GetOutputPins() const {
        std::vector<InputOutput*> outputs;
        for (auto& element : elements) {
            if (element->GetType() == TYPE_OUTPUT) {
                outputs.push_back(dynamic_cast<InputOutput*>(element.get()));
            }
        }
        return outputs;
    }

    // 公共撤销/重做接口
    bool CanUndo() const { return !undoStack.empty(); }
    bool CanRedo() const { return !redoStack.empty(); }

    void Undo() {
        if (!CanUndo() || isRestoringState) return;

        isRestoringState = true;

        // 执行撤销操作
        if (!undoStack.empty()) {
            auto& operation = undoStack.back();
            operation->Undo(this);

            // 移动到重做栈
            redoStack.push_back(std::move(operation));
            undoStack.pop_back();
        }

        isRestoringState = false;
        UpdateUndoRedoStatus();
        Refresh();
        NotifyStateChange(); // 新增：通知状态改变

        // 更新状态栏
        UpdateStatusBar();
    }

    void Redo() {
        if (!CanRedo() || isRestoringState) return;

        isRestoringState = true;

        // 执行重做操作
        if (!redoStack.empty()) {
            auto& operation = redoStack.back();
            operation->Execute(this);

            // 移动回撤销栈
            undoStack.push_back(std::move(operation));
            redoStack.pop_back();
        }

        isRestoringState = false;
        UpdateUndoRedoStatus();
        Refresh();
        NotifyStateChange(); // 新增：通知状态改变

        // 更新状态栏
        UpdateStatusBar();
    }

    void ClearHistory() {
        undoStack.clear();
        redoStack.clear();
        UpdateUndoRedoStatus();
    }

    // 复制粘贴相关公共方法
    void CopySelectedElements() {
        clipboard.clear();
        pasteCount = 0;

        for (auto& element : elements) {
            if (element->IsSelected()) {
                // 序列化然后反序列化来创建深拷贝
                wxString data;
                element->Serialize(data);

                // 根据类型创建新元件
                std::unique_ptr<CircuitElement> newElement = CreateElementFromData(data);
                if (newElement) {
                    clipboard.push_back(std::move(newElement));
                }
            }
        }

        if (!clipboard.empty()) {
            // 更新状态栏
            UpdateStatusBar();
        }
    }

    void PasteElements() {
        if (clipboard.empty()) return;

        pasteCount++;
        pasteOffset = wxPoint(20 * pasteCount, 20 * pasteCount);  // 每次粘贴偏移20像素

        // 清除当前选中状态
        for (auto& element : elements) {
            element->SetSelected(false);
        }

        // 添加剪贴板中的元件到画布
        for (auto& element : clipboard) {
            // 创建新位置
            int newX = element->GetX() + pasteOffset.x;
            int newY = element->GetY() + pasteOffset.y;

            // 序列化然后创建新元件
            wxString data;
            element->Serialize(data);
            std::unique_ptr<CircuitElement> newElement = CreateElementFromData(data);

            if (newElement) {
                newElement->SetPosition(newX, newY);
                newElement->SetSelected(true);  // 选中粘贴的元件

                CircuitElement* elementPtr = newElement.get();
                elements.push_back(std::move(newElement));

                // 记录添加操作（用于撤销）
                if (!isRestoringState) {
                    wxString serializedData = SerializeElement(elementPtr);
                    auto operation = std::make_unique<AddElementOperation>(serializedData);
                    undoStack.push_back(std::move(operation));

                    if (undoStack.size() > MAX_HISTORY) {
                        undoStack.erase(undoStack.begin());
                    }
                    redoStack.clear();
                    UpdateUndoRedoStatus();
                }
            }
        }

        UpdateCircuit();
        Refresh();

        // 更新状态栏
        UpdateStatusBar();
    }

    void SelectAllElements() {
        for (auto& element : elements) {
            if (element->IsSelected())
                element->SetSelected(false);
            else
                element->SetSelected(true);
        }
        selectedElement = nullptr;  // 清除单个选中
        Refresh();

        // 更新状态栏
        UpdateStatusBar();
    }

    // 检查是否有选中的元件
    bool HasSelectedElements() const {
        for (auto& element : elements) {
            if (element->IsSelected()) {
                return true;
            }
        }
        return false;
    }

    bool HasOneSelectedElement() const {
        return selectedElement != nullptr;
    }

    // 删除选中元件
    void DeleteSelectedElement() {
        if (!selectedElement || isRestoringState) return;

        // 显示确认对话框
        wxMessageDialog dialog(GetParent(),
            "Are you sure you want to delete the selected element?",
            "Confirm Delete",
            wxYES_NO | wxICON_QUESTION);

        if (dialog.ShowModal() == wxID_YES) {
            // 序列化元件数据用于撤销
            wxString serializedData = SerializeElement(selectedElement);

            // 找到要删除的元件
            std::unique_ptr<CircuitElement> elementToDelete;
            auto it = std::find_if(elements.begin(), elements.end(),
                [this](const std::unique_ptr<CircuitElement>& elem) {
                    return elem.get() == selectedElement;
                });

            if (it != elements.end()) {
                // 先断开所有引脚连接
                auto pins = selectedElement->GetPins();
                for (auto pin : pins) {
                    // 找到并删除连接到该引脚的所有导线
                    for (auto wireIt = wires.begin(); wireIt != wires.end(); ) {
                        if ((*wireIt)->GetStartPin() == pin || (*wireIt)->GetEndPin() == pin) {
                            // 记录导线删除操作（如果需要撤销）
                            wireIt = wires.erase(wireIt);
                        }
                        else {
                            ++wireIt;
                        }
                    }
                    // 清除引脚的连接
                    pin->SetConnectedWire(nullptr);
                }

                // 保存要删除的元件
                elementToDelete = std::move(*it);

                // 记录删除操作到撤销栈
                auto operation = std::make_unique<DeleteElementOperation>(
                    std::move(elementToDelete), serializedData);

                // 限制历史记录数量
                if (undoStack.size() >= MAX_HISTORY) {
                    undoStack.erase(undoStack.begin());
                }
                undoStack.push_back(std::move(operation));
                redoStack.clear();
                UpdateUndoRedoStatus();

                // 从元素列表中移除
                elements.erase(it);

                // 清除选中状态
                selectedElement = nullptr;
                connectionScrollPos = 0; // 重置滚动位置

                // 新增：更新状态栏清除显示
                UpdateStatusBar();

                // 更新电路状态
                UpdateCircuit();
                Refresh();
            }
        }
    }

    // 检查是否在自动放置模式
    bool IsInAutoPlaceMode() const { return autoPlaceMode; }
    // 获取自动放置类型
    ElementType GetAutoPlaceType() const { return autoPlaceType; }

    // 新增：在导线上查找最近的点
    wxPoint FindNearestPointOnWire(const wxPoint& pos) const {
        double minDistance = std::numeric_limits<double>::max();
        wxPoint nearestPoint = pos;

        for (auto& wire : wires) {
            Pin* startPin = wire->GetStartPin();
            Pin* endPin = wire->GetEndPin();
            if (!startPin || !endPin) continue;

            int x1 = startPin->GetX(), y1 = startPin->GetY();
            int x2 = endPin->GetX(), y2 = endPin->GetY();

            // 计算点到线段的最近点
            double A = pos.x - x1;
            double B = pos.y - y1;
            double C = x2 - x1;
            double D = y2 - y1;

            double dot = A * C + B * D;
            double len_sq = C * C + D * D;
            double param = (len_sq != 0) ? dot / len_sq : -1;

            double xx, yy;

            if (param < 0) {
                xx = x1;
                yy = y1;
            }
            else if (param > 1) {
                xx = x2;
                yy = y2;
            }
            else {
                xx = x1 + param * C;
                yy = y1 + param * D;
            }

            double dx = pos.x - xx;
            double dy = pos.y - yy;
            double distance = std::sqrt(dx * dx + dy * dy);

            if (distance < minDistance && distance < 10) { // 10像素范围内
                minDistance = distance;
                nearestPoint = wxPoint(static_cast<int>(xx), static_cast<int>(yy));
            }
        }

        return nearestPoint;
    }

    // 新增：检查点是否在导线上
    Wire* FindWireAtPosition(const wxPoint& pos) const {
        for (auto& wire : wires) {
            if (wire->ContainsPoint(pos)) {
                return wire.get();
            }
        }
        return nullptr;
    }

    // 新增：创建连接到导线的连接点
    void CreateWireToWireConnection(Pin* startPin, const wxPoint& wirePoint, Wire* targetWire) {
        if (!startPin || !targetWire) return;

        // 创建虚拟引脚（输入引脚）
        auto virtualPin = std::make_unique<Pin>(wirePoint.x, wirePoint.y, true, nullptr, true);
        Pin* virtualPinPtr = virtualPin.get();
        virtualPins.push_back(std::move(virtualPin));

        // 创建从引脚到虚拟引脚的连接
        wires.push_back(std::make_unique<Wire>(startPin, virtualPinPtr));

        // 记录操作
        if (!isRestoringState) {
            wxString serializedData = SerializeWire(wires.back().get());
            auto operation = std::make_unique<AddWireOperation>(serializedData);
            undoStack.push_back(std::move(operation));

            if (undoStack.size() > MAX_HISTORY) {
                undoStack.erase(undoStack.begin());
            }
            redoStack.clear();
            UpdateUndoRedoStatus();
        }

        UpdateCircuit();
        Refresh();
    }

private:
    // 复制粘贴相关成员变量
    std::vector<std::unique_ptr<CircuitElement>> clipboard;  // 剪贴板
    wxPoint pasteOffset;                                     // 粘贴偏移量
    int pasteCount;                                          // 粘贴计数

    // 虚拟引脚管理
    std::vector<std::unique_ptr<Pin>> virtualPins;           // 虚拟引脚容器

    // 连接信息滚动相关变量
    int connectionScrollPos;     // 连接信息滚动位置
    int maxConnectionWidth;      // 连接信息最大宽度

    // 从序列化数据创建元件
    std::unique_ptr<CircuitElement> CreateElementFromData(const wxString& data) {
        wxStringTokenizer tokens(data, ",");
        if (tokens.CountTokens() >= 3) {
            long typeVal, x, y;
            tokens.GetNextToken().ToLong(&typeVal);
            tokens.GetNextToken().ToLong(&x);
            tokens.GetNextToken().ToLong(&y);

            ElementType type = static_cast<ElementType>(typeVal);
            std::unique_ptr<CircuitElement> newElement;

            // 根据类型创建相应的元件
            if (type >= TYPE_AND && type <= TYPE_NOR) {
                newElement = std::make_unique<Gate>(type, x, y);
            }
            else if (type == TYPE_INPUT || type == TYPE_OUTPUT) {
                newElement = std::make_unique<InputOutput>(type, x, y);

                // 处理输入输出的额外属性
                if (tokens.HasMoreTokens()) {
                    long value;
                    tokens.GetNextToken().ToLong(&value);
                    if (InputOutput* io = dynamic_cast<InputOutput*>(newElement.get())) {
                        io->SetValue(value != 0);
                    }
                    if (tokens.HasMoreTokens()) {
                        wxString name = tokens.GetNextToken();
                        if (InputOutput* io = dynamic_cast<InputOutput*>(newElement.get())) {
                            io->SetName(name);
                        }
                    }
                }
            }
            else if (type == TYPE_CLOCK) {
                newElement = std::make_unique<ClockElement>(x, y);
                if (tokens.HasMoreTokens()) {
                    long freq;
                    tokens.GetNextToken().ToLong(&freq);
                    if (ClockElement* clock = dynamic_cast<ClockElement*>(newElement.get())) {
                        clock->SetFrequency(freq);
                    }
                }
            }
            else if (type == TYPE_RS_FLIPFLOP) {
                newElement = std::make_unique<RSFlipFlop>(x, y);
            }
            else if (type == TYPE_D_FLIPFLOP) {
                newElement = std::make_unique<DFlipFlop>(x, y);
            }
            else if (type == TYPE_JK_FLIPFLOP) {
                newElement = std::make_unique<JKFlipFlop>(x, y);
            }
            else if (type == TYPE_T_FLIPFLOP) {
                newElement = std::make_unique<TFlipFlop>(x, y);
            }
            else if (type == TYPE_REGISTER) {
                newElement = std::make_unique<RegisterElement>(x, y);
            }

            if (newElement) {
                newElement->Deserialize(data);
                return newElement;
            }
        }
        return nullptr;
    }

    // === 操作系统 ===
    enum OperationType {
        OP_ADD_ELEMENT,
        OP_DELETE_ELEMENT,
        OP_ADD_WIRE,
        OP_DELETE_WIRE,
        OP_MOVE_ELEMENT,
        OP_CHANGE_VALUE
    };

    // 操作基类，定义撤销/重做接口
    class Operation {
    public:
        Operation(OperationType type) : type(type) {}
        virtual ~Operation() {}
        virtual void Execute(CircuitCanvas* canvas) = 0;  // 执行操作
        virtual void Undo(CircuitCanvas* canvas) = 0;     // 撤销操作
        OperationType GetType() const { return type; }
    private:
        OperationType type;
    };

    // 添加元件操作
    class AddElementOperation : public Operation {
    public:
        AddElementOperation(const wxString& serializedData)
            : Operation(OP_ADD_ELEMENT), serializedData(serializedData) {
        }

        virtual void Execute(CircuitCanvas* canvas) override {
            canvas->RestoreElementFromSerializedData(serializedData);
        }

        virtual void Undo(CircuitCanvas* canvas) override {
            // 找到并删除最后添加的元件（就是我们刚刚重做添加的那个）
            if (!canvas->elements.empty()) {
                CircuitElement* elementToRemove = canvas->elements.back().get();
                canvas->RemoveElementWithoutHistory(elementToRemove);
            }
        }

    private:
        wxString serializedData;
    };

    // 添加导线操作
    class AddWireOperation : public Operation {
    public:
        AddWireOperation(const wxString& serializedData)
            : Operation(OP_ADD_WIRE), serializedData(serializedData) {
        }

        virtual void Execute(CircuitCanvas* canvas) override {
            canvas->RestoreWireFromSerializedData(serializedData);
        }

        virtual void Undo(CircuitCanvas* canvas) override {
            // 找到并删除最后添加的导线
            if (!canvas->wires.empty()) {
                Wire* wireToRemove = canvas->wires.back().get();
                canvas->RemoveWireWithoutHistory(wireToRemove);
            }
        }

    private:
        wxString serializedData;
    };

    // 批量删除操作
    class BatchDeleteOperation : public Operation {
    public:
        BatchDeleteOperation(std::vector<std::pair<std::unique_ptr<CircuitElement>, wxString>> elements)
            : Operation(OP_DELETE_ELEMENT), elements(std::move(elements)) {
        }

        virtual void Execute(CircuitCanvas* canvas) override {
            // 重做删除操作：再次删除
            for (auto& element : elements) {
                if (element.first) {
                    canvas->RemoveElementWithoutHistory(element.first.get());
                }
            }
        }

        virtual void Undo(CircuitCanvas* canvas) override {
            // 撤销删除操作：恢复所有元件
            for (auto& element : elements) {
                canvas->RestoreElementFromSerializedData(element.second);
            }
        }

    private:
        std::vector<std::pair<std::unique_ptr<CircuitElement>, wxString>> elements;
    };

    // 删除元件操作
    class DeleteElementOperation : public Operation {
    public:
        DeleteElementOperation(std::unique_ptr<CircuitElement> element, const wxString& serializedData)
            : Operation(OP_DELETE_ELEMENT), element(std::move(element)), serializedData(serializedData) {
        }

        virtual void Execute(CircuitCanvas* canvas) override {
            // 重做删除操作：再次删除
            if (element) {
                canvas->RemoveElementWithoutHistory(element.get());
            }
        }

        virtual void Undo(CircuitCanvas* canvas) override {
            // 撤销删除操作：恢复元件
            canvas->RestoreElementFromSerializedData(serializedData);
        }

    private:
        std::unique_ptr<CircuitElement> element;
        wxString serializedData;
    };

    // 删除导线操作
    class DeleteWireOperation : public Operation {
    public:
        DeleteWireOperation(std::unique_ptr<Wire> wire, const wxString& serializedData)
            : Operation(OP_DELETE_WIRE), wire(std::move(wire)), serializedData(serializedData) {
        }

        virtual void Execute(CircuitCanvas* canvas) override {
            // 重做删除操作：再次删除
            if (wire) {
                canvas->RemoveWireWithoutHistory(wire.get());
            }
        }

        virtual void Undo(CircuitCanvas* canvas) override {
            // 撤销删除操作：恢复导线
            canvas->RestoreWireFromSerializedData(serializedData);
        }

    private:
        std::unique_ptr<Wire> wire;
        wxString serializedData;
    };

    // === 撤销/重做系统 ===
    std::vector<std::unique_ptr<Operation>> undoStack;  // 撤销栈
    std::vector<std::unique_ptr<Operation>> redoStack;  // 重做栈
    const int MAX_HISTORY = 50;  // 最大历史记录数量
    bool isRestoringState;       // 是否正在恢复状态（防止递归记录操作）

    // === 序列化和反序列化方法 ===

    // 序列化元件
    wxString SerializeElement(CircuitElement* element) {
        wxString data;
        element->Serialize(data);
        return data;
    }

    // 序列化导线
    wxString SerializeWire(Wire* wire) {
        wxString data;
        wire->Serialize(data);
        return data;
    }

    // 从序列化数据恢复元件
    void RestoreElementFromSerializedData(const wxString& data) {
        CreateElementFromSerializedData(data);
        UpdateCircuit();
        Refresh();
    }

    // 从序列化数据恢复导线
    void RestoreWireFromSerializedData(const wxString& data) {
        CreateWireFromSerializedData(data);
        UpdateCircuit();
        Refresh();
    }

    // 无历史记录地删除元件（用于撤销/重做系统内部使用）
    void RemoveElementWithoutHistory(CircuitElement* element) {
        if (!element) return;

        isRestoringState = true;

        // 安全地删除与元件引脚相连的所有导线
        auto pins = element->GetPins();
        for (auto pin : pins) {
            // 使用临时向量收集要删除的导线，避免迭代器失效
            std::vector<Wire*> wiresToRemove;
            for (auto& wire : wires) {
                if (wire->GetStartPin() == pin || wire->GetEndPin() == pin) {
                    wiresToRemove.push_back(wire.get());
                }
            }

            // 删除收集到的导线
            for (auto wireToRemove : wiresToRemove) {
                auto it = std::find_if(wires.begin(), wires.end(),
                    [wireToRemove](const std::unique_ptr<Wire>& w) {
                        return w.get() == wireToRemove;
                    });
                if (it != wires.end()) {
                    wires.erase(it);
                }
            }

            // 清除引脚连接
            pin->SetConnectedWire(nullptr);
        }

        // 安全地删除元件
        auto it = std::find_if(elements.begin(), elements.end(),
            [element](const std::unique_ptr<CircuitElement>& elem) {
                return elem.get() == element;
            });

        if (it != elements.end()) {
            elements.erase(it);
        }

        // 清除选中状态
        if (selectedElement == element) {
            selectedElement = nullptr;
        }

        isRestoringState = false;
        UpdateCircuit();
        Refresh();
    }

    // 无历史记录地删除导线
    void RemoveWireWithoutHistory(Wire* wire) {
        isRestoringState = true;

        auto it = std::find_if(wires.begin(), wires.end(),
            [wire](const std::unique_ptr<Wire>& w) {
                return w.get() == wire;
            });

        if (it != wires.end()) {
            // 如果导线包含虚拟引脚，也需要清理虚拟引脚
            if (wire->HasVirtualPin()) {
                Pin* startPin = wire->GetStartPin();
                Pin* endPin = wire->GetEndPin();

                // 清理虚拟引脚
                if (startPin && startPin->IsVirtual()) {
                    auto pinIt = std::find_if(virtualPins.begin(), virtualPins.end(),
                        [startPin](const std::unique_ptr<Pin>& p) {
                            return p.get() == startPin;
                        });
                    if (pinIt != virtualPins.end()) {
                        virtualPins.erase(pinIt);
                    }
                }
                if (endPin && endPin->IsVirtual()) {
                    auto pinIt = std::find_if(virtualPins.begin(), virtualPins.end(),
                        [endPin](const std::unique_ptr<Pin>& p) {
                            return p.get() == endPin;
                        });
                    if (pinIt != virtualPins.end()) {
                        virtualPins.erase(pinIt);
                    }
                }
            }

            wires.erase(it);
        }

        isRestoringState = false;
        UpdateCircuit();
        Refresh();
    }

    // 完成导线连接
    void CompleteWireConnection(Pin* startPin, Pin* endPin) {
        if (startPin && endPin && startPin->IsInput() != endPin->IsInput()) {
            Pin* outputPin = startPin->IsInput() ? endPin : startPin;
            Pin* inputPin = startPin->IsInput() ? startPin : endPin;

            // 创建导线
            wires.push_back(std::make_unique<Wire>(outputPin, inputPin));
            Wire* wirePtr = wires.back().get();

            // 记录添加导线操作
            if (!isRestoringState) {
                wxString serializedData = SerializeWire(wirePtr);
                auto operation = std::make_unique<AddWireOperation>(serializedData);
                undoStack.push_back(std::move(operation));

                // 限制历史记录数量
                if (undoStack.size() > MAX_HISTORY) {
                    undoStack.erase(undoStack.begin());
                }

                redoStack.clear();
                UpdateUndoRedoStatus();
            }

            UpdateCircuit();
        }
        this->startPin = nullptr;
    }

    // 从序列化数据创建元件
    void CreateElementFromSerializedData(const wxString& data) {
        wxStringTokenizer tokens(data, ",");
        if (tokens.CountTokens() >= 3) {
            long typeVal, x, y;
            tokens.GetNextToken().ToLong(&typeVal);
            tokens.GetNextToken().ToLong(&x);
            tokens.GetNextToken().ToLong(&y);

            ElementType type = static_cast<ElementType>(typeVal);
            std::unique_ptr<CircuitElement> newElement;

            // 根据类型创建相应的元件
            if (type >= TYPE_AND && type <= TYPE_NOR) {
                newElement = std::make_unique<Gate>(type, x, y);
            }
            else if (type == TYPE_INPUT || type == TYPE_OUTPUT) {
                newElement = std::make_unique<InputOutput>(type, x, y);

                // 处理输入输出的额外属性
                if (tokens.HasMoreTokens()) {
                    long value;
                    tokens.GetNextToken().ToLong(&value);
                    if (InputOutput* io = dynamic_cast<InputOutput*>(newElement.get())) {
                        io->SetValue(value != 0);
                    }
                    if (tokens.HasMoreTokens()) {
                        wxString name = tokens.GetNextToken();
                        if (InputOutput* io = dynamic_cast<InputOutput*>(newElement.get())) {
                            io->SetName(name);
                        }
                    }
                }
            }
            else if (type == TYPE_CLOCK) {
                newElement = std::make_unique<ClockElement>(x, y);
                if (tokens.HasMoreTokens()) {
                    long freq;
                    tokens.GetNextToken().ToLong(&freq);
                    if (ClockElement* clock = dynamic_cast<ClockElement*>(newElement.get())) {
                        clock->SetFrequency(freq);
                    }
                }
            }
            else if (type == TYPE_RS_FLIPFLOP) {  // 新增RS触发器
                newElement = std::make_unique<RSFlipFlop>(x, y);
            }
            else if (type == TYPE_D_FLIPFLOP) {
                newElement = std::make_unique<DFlipFlop>(x, y);
            }
            else if (type == TYPE_JK_FLIPFLOP) {
                newElement = std::make_unique<JKFlipFlop>(x, y);
            }
            else if (type == TYPE_T_FLIPFLOP) {
                newElement = std::make_unique<TFlipFlop>(x, y);
            }
            else if (type == TYPE_REGISTER) {
                newElement = std::make_unique<RegisterElement>(x, y);
            }

            if (newElement) {
                elements.push_back(std::move(newElement));
            }
        }
    }

    // 从序列化数据创建导线
    void CreateWireFromSerializedData(const wxString& data) {
        wxStringTokenizer tokens(data, ",");
        tokens.GetNextToken(); // 跳过 "WIRE"

        if (tokens.CountTokens() >= 6) {
            long startX, startY, startIsInput, endX, endY, endIsInput;
            tokens.GetNextToken().ToLong(&startX);
            tokens.GetNextToken().ToLong(&startY);
            tokens.GetNextToken().ToLong(&startIsInput);
            tokens.GetNextToken().ToLong(&endX);
            tokens.GetNextToken().ToLong(&endY);
            tokens.GetNextToken().ToLong(&endIsInput);

            // 通过坐标查找对应的引脚
            Pin* startPin = FindPinByPosition(startX, startY);
            Pin* endPin = FindPinByPosition(endX, endY);

            // 验证引脚类型匹配
            if (startPin && endPin &&
                startPin->IsInput() == (startIsInput == 1) &&
                endPin->IsInput() == (endIsInput == 1)) {

                // 确保连接方向正确：输出引脚 -> 输入引脚
                if (!startPin->IsInput() && endPin->IsInput()) {
                    wires.push_back(std::make_unique<Wire>(startPin, endPin));
                }
                else if (startPin->IsInput() && !endPin->IsInput()) {
                    wires.push_back(std::make_unique<Wire>(endPin, startPin));
                }
            }
        }
    }

    // 通过坐标查找引脚
    Pin* FindPinByPosition(int x, int y) {
        const int tolerance = 5;  // 容差范围
        for (auto& element : elements) {
            for (auto pin : element->GetPins()) {
                int pinX = pin->GetX();
                int pinY = pin->GetY();
                if (abs(pinX - x) <= tolerance && abs(pinY - y) <= tolerance) {
                    return pin;
                }
            }
        }
        return nullptr;
    }

    // 更新撤销/重做按钮状态
    void UpdateUndoRedoStatus() {
        wxWindow* topWindow = wxGetTopLevelParent(this);
        if (topWindow && topWindow->IsKindOf(CLASSINFO(wxFrame))) {
            wxFrame* frame = static_cast<wxFrame*>(topWindow);
            if (wxMenuBar* menuBar = frame->GetMenuBar()) {
                menuBar->Enable(wxID_UNDO, CanUndo());
                menuBar->Enable(wxID_REDO, CanRedo());
            }
            if (wxToolBar* toolBar = frame->GetToolBar()) {
                toolBar->EnableTool(wxID_UNDO, CanUndo());
                toolBar->EnableTool(wxID_REDO, CanRedo());
            }
        }
    }

    // 原有的私有成员变量
    ElementType currentTool;      // 当前工具类型
    bool wiringMode;              // 是否处于连线模式
    bool simulating;              // 是否正在仿真
    bool showGrid;                // 是否显示网格
    double zoomLevel;             // 缩放级别
    CircuitElement* selectedElement;  // 当前选中的元件
    Pin* startPin;                // 连线起始引脚
    wxPoint lastMousePos;         // 最后鼠标位置
    wxPoint dragStartPos;         // 拖动起始位置
    wxPoint elementStartPos;      // 元件起始位置
    bool autoPlaceMode;           // 自动放置模式
    ElementType autoPlaceType;    // 自动放置类型
    wxSize virtualSize;           // 虚拟画布大小
    std::vector<std::unique_ptr<CircuitElement>> elements;  // 元件列表
    std::vector<std::unique_ptr<Wire>> wires;               // 导线列表
    Wire* selectedWire;           // 当前选中的导线

    // 绘制事件处理
    void OnPaint(wxPaintEvent& event) {
        wxAutoBufferedPaintDC dc(this);  // 创建双缓冲绘图设备上下文
        DoPrepareDC(dc);  // 准备设备上下文，处理滚动和缩放

        dc.Clear();  // 清空画布

        dc.SetUserScale(zoomLevel, zoomLevel);  // 应用缩放

        // 获取可见区域（虚拟坐标）
        wxPoint viewStart = GetViewStart();
        wxSize clientSize = GetClientSize();

        // 正确的可见区域计算
        int startX = viewStart.x * 10;  // 转换为虚拟坐标
        int startY = viewStart.y * 10;
        int endX = startX + clientSize.x / zoomLevel;
        int endY = startY + clientSize.y / zoomLevel;

        // 绘制网格（只在可见区域绘制以提高性能）
        if (showGrid) {
            dc.SetPen(wxPen(wxColour(220, 220, 220), 1));

            // 计算网格起始位置（对齐到网格）
            int gridStartX = (startX / 20) * 20;
            int gridStartY = (startY / 20) * 20;

            // 绘制垂直线
            for (int x = gridStartX; x <= endX; x += 20) {
                dc.DrawLine(x, startY, x, endY);
            }
            // 绘制水平线
            for (int y = gridStartY; y <= endY; y += 20) {
                dc.DrawLine(startX, y, endX, y);
            }
        }

        // 绘制所有导线
        for (auto& wire : wires) {
            // 如果是选中的导线，用不同颜色绘制
            if (wire.get() == selectedWire) {
                wxDC& dcRef = dc; // 创建引用以便在lambda中使用
                bool value = wire->GetStartPin()->GetValue();

                // 选中的导线用更粗的蓝色线绘制
                dc.SetPen(value ? wxPen(*wxBLUE, 4) : wxPen(*wxBLUE, 4));
                wire->Draw(dc);

                // 恢复原来的颜色继续绘制
                dc.SetPen(value ? wxPen(*wxGREEN, 2) : wxPen(*wxRED, 2));
            }
            else {
                wire->Draw(dc);
            }
        }

        // 绘制所有元件（只绘制在可见区域内的元件以提高性能）
        for (auto& element : elements) {
            wxRect bbox = element->GetBoundingBox();
            // 简单的可见性检查
            if (bbox.GetRight() >= startX && bbox.GetLeft() <= endX &&
                bbox.GetBottom() >= startY && bbox.GetTop() <= endY) {
                element->Draw(dc);
            }
        }

        // 为输入元件添加点击提示
        if (!simulating) {
            dc.SetTextForeground(*wxBLUE);
            wxFont smallFont = dc.GetFont();
            smallFont.SetPointSize(7);
            dc.SetFont(smallFont);

            for (auto& element : elements) {
                if (element->GetType() == TYPE_INPUT) {
                    InputOutput* input = dynamic_cast<InputOutput*>(element.get());
                    if (input) {
                        wxRect bbox = element->GetBoundingBox();
                        // 可见性检查
                        if (bbox.GetRight() >= startX && bbox.GetLeft() <= endX &&
                            bbox.GetBottom() >= startY && bbox.GetTop() <= endY) {
                            wxString hint = "(Click to toggle)";
                            wxSize textSize = dc.GetTextExtent(hint);
                            dc.DrawText(hint,
                                bbox.GetLeft() + (bbox.GetWidth() - textSize.GetWidth()) / 2,
                                bbox.GetBottom() + 5);
                        }
                    }
                }
            }
        }

        // 绘制连线过程中的临时线
        if (wiringMode && startPin) {
            dc.SetPen(wxPen(*wxBLUE, 2, wxPENSTYLE_DOT));
            dc.DrawLine(startPin->GetX(), startPin->GetY(), lastMousePos.x, lastMousePos.y);
        }

        // 绘制自动放置预览
        if (autoPlaceMode && !simulating) {
            DrawAutoPlacePreview(dc, lastMousePos);
        }

        // 可选：绘制虚拟引脚作为视觉指示
        dc.SetPen(wxPen(*wxBLUE, 1, wxPENSTYLE_DOT));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        for (auto& vpin : virtualPins) {
            dc.DrawCircle(vpin->GetX(), vpin->GetY(), 4);
        }

        dc.SetUserScale(1.0, 1.0);  // 恢复原始缩放

        // 绘制状态信息
        if (!simulating) {
            dc.SetTextForeground(*wxBLUE);
            wxString toolText = GetToolName(currentTool);
            if (autoPlaceMode) {
                toolText += " - Click to place";
            }
            dc.DrawText(toolText, 10, 10);

            // 显示当前视图位置和缩放信息
            wxString viewInfo = wxString::Format("View: (%d,%d) Zoom: %.0f%%", startX, startY, zoomLevel * 100);
            dc.DrawText(viewInfo, 10, 30);

            // 显示连接信息滚动提示
            if (maxConnectionWidth > 400) {
                dc.DrawText("Use ← → to scroll connection info", 10, 50);
            }
        }
        else {
            dc.SetTextForeground(*wxRED);
            dc.DrawText("SIMULATION RUNNING", 10, 10);
        }
    }

    // 绘制自动放置预览
    void DrawAutoPlacePreview(wxDC& dc, const wxPoint& pos) {
        dc.SetPen(wxPen(*wxLIGHT_GREY, 1, wxPENSTYLE_DOT));  // 浅灰色虚线
        dc.SetBrush(*wxTRANSPARENT_BRUSH);  // 透明填充

        // 根据元件类型绘制预览轮廓
        switch (autoPlaceType) {
        case TYPE_AND:
        case TYPE_OR:
        case TYPE_NOT:
        case TYPE_XOR:
        case TYPE_NAND:
        case TYPE_NOR:
            dc.DrawRectangle(pos.x - 15, pos.y - 15, 30, 30);  // 矩形轮廓
            break;
        case TYPE_INPUT:
        case TYPE_OUTPUT:
            dc.DrawRectangle(pos.x - 15, pos.y - 15, 30, 30);  // 矩形轮廓
            break;
        default:
            break;
        }
    }

    // 鼠标左键按下事件
    void OnLeftDown(wxMouseEvent& event) {
        wxPoint pos = event.GetPosition();
        CalcUnscrolledPosition(pos.x, pos.y, &pos.x, &pos.y);  // 转换为虚拟坐标

        pos.x /= zoomLevel;  // 调整到缩放后坐标
        pos.y /= zoomLevel;
        lastMousePos = pos;  // 记录鼠标位置

        // 清除之前的选中状态
        if (selectedElement) {
            selectedElement->SetSelected(false);
            selectedElement = nullptr;
        }
        if (selectedWire) {
            selectedWire = nullptr;
        }

        // 如果是显示真值工具模式，优先处理输入元件点击
        if (currentTool == TYPE_TOGGLE_VALUE) {
            if (TryToggleInputElement(pos)) {
                Refresh();
                return;
            }
        }

        if (autoPlaceMode) {
            // 自动放置模式：直接创建元件
            CreateElementAtPosition(autoPlaceType, pos);
            return;
        }

        if (currentTool == TYPE_SELECT) {
            // 首先检查是否点击了导线
            bool wireClicked = false;
            for (auto& wire : wires) {
                if (wire->ContainsPoint(pos)) {
                    selectedWire = wire.get();
                    wireClicked = true;
                    break;
                }
            }

            // 如果没有点击导线，再检查元件
            if (!wireClicked) {
                for (auto it = elements.rbegin(); it != elements.rend(); ++it) {
                    if ((*it)->GetBoundingBox().Contains(pos)) {
                        selectedElement = it->get();
                        (*it)->SetSelected(true);

                        if (currentTool == TYPE_SELECT) {
                            dragStartPos = pos;
                            elementStartPos = wxPoint((*it)->GetX(), (*it)->GetY());
                        }
                        break;
                    }
                }
            }

            // 重置滚动位置
            connectionScrollPos = 0;
            // 新增：更新状态栏显示连接信息
            UpdateStatusBar();
        }

        if (currentTool == TYPE_SELECT || currentTool == TYPE_TOGGLE_VALUE) {
            selectedElement = nullptr;
            // 从后向前遍历（处理重叠元件）
            for (auto it = elements.rbegin(); it != elements.rend(); ++it) {
                if ((*it)->GetBoundingBox().Contains(pos)) {
                    selectedElement = it->get();
                    (*it)->SetSelected(true);

                    // 只有在选择工具模式下才能拖动
                    if (currentTool == TYPE_SELECT) {
                        dragStartPos = pos;  // 记录拖动起始位置
                        elementStartPos = wxPoint((*it)->GetX(), (*it)->GetY());  // 记录元件起始位置
                    }
                    break;
                }
            }

            // 如果没有选中元件，清除所有选中状态
            if (selectedElement == nullptr) {
                for (auto& element : elements) {
                    element->SetSelected(false);
                }
            }

            // 重置滚动位置
            connectionScrollPos = 0;
            // 新增：更新状态栏显示连接信息
            UpdateStatusBar();
        }
        else if (currentTool == TYPE_TOGGLE_VALUE) {
            if (TryToggleInputElement(pos)) {
                Refresh();
                return;
            }
        }
        // 修改连线模式的处理
        else if (wiringMode) {
            // 连线模式：处理引脚连接
            Pin* pin = FindPinAt(pos);
            if (pin) {
                if (startPin == nullptr) {
                    startPin = pin;  // 设置起始引脚
                }
                else if (pin != startPin) {
                    // 确保连接的是输入和输出引脚
                    if (startPin->IsInput() != pin->IsInput()) {
                        // 使用新的方法完成连线并记录操作
                        CompleteWireConnection(startPin, pin);
                    }
                    startPin = nullptr;  // 重置起始引脚
                }
            }
            else if (startPin != nullptr) {
                // 检查是否点击在导线上
                Wire* wire = FindWireAtPosition(pos);
                if (wire) {
                    // 在导线上创建连接点
                    wxPoint wirePoint = FindNearestPointOnWire(pos);
                    CreateWireToWireConnection(startPin, wirePoint, wire);
                    startPin = nullptr;
                }
                else {
                    startPin = nullptr;  // 点击空白处重置起始引脚
                }
            }
        }
        else {
            // 传统模式：点击创建元件
            CreateElementAtPosition(currentTool, pos);
        }

        Refresh();  // 刷新显示
    }

    // 尝试切换输入元件的值
    bool TryToggleInputElement(const wxPoint& pos) {
        for (auto& element : elements) {
            if (element->GetType() == TYPE_INPUT && element->GetBoundingBox().Contains(pos)) {
                InputOutput* inputElement = dynamic_cast<InputOutput*>(element.get());
                if (inputElement) {
                    // 切换输入值（0变1，1变0）
                    inputElement->SetValue(!inputElement->GetValue());

                    // 更新电路状态
                    UpdateCircuit();

                    // 更新状态栏
                    wxWindow* topWindow = wxGetTopLevelParent(this);
                    if (topWindow && topWindow->IsKindOf(CLASSINFO(wxFrame))) {
                        wxFrame* frame = static_cast<wxFrame*>(topWindow);
                        wxStatusBar* statusBar = frame->GetStatusBar();
                        if (statusBar) {
                            wxString state = inputElement->GetValue() ? "1" : "0";
                            wxString name = inputElement->GetDisplayName();
                            statusBar->SetStatusText(wxString::Format("%s toggled to %s", name, state));
                        }
                    }
                    return true;
                }
            }
        }
        return false;
    }

    // 鼠标左键释放事件
    void OnLeftUp(wxMouseEvent& event) {
        if (selectedElement) {
            selectedElement->SetSelected(false);  // 取消选中状态
        }
    }

    // 鼠标移动事件
    void OnMouseMove(wxMouseEvent& event) {
        wxPoint pos = event.GetPosition();
        CalcUnscrolledPosition(pos.x, pos.y, &pos.x, &pos.y);  // 转换为虚拟坐标

        pos.x /= zoomLevel;
        pos.y /= zoomLevel;
        lastMousePos = pos;  // 记录鼠标位置

        // 只有在选择工具模式下才能拖动元件
        if (event.Dragging() && selectedElement && currentTool == TYPE_SELECT) {
            int gridSize = 20;  // 网格大小
            // 计算新位置
            int x = elementStartPos.x + (pos.x - dragStartPos.x);
            int y = elementStartPos.y + (pos.y - dragStartPos.y);

            // 对齐到网格
            x = (x / gridSize) * gridSize;
            y = (y / gridSize) * gridSize;

            selectedElement->SetPosition(x, y);  // 设置新位置
            Refresh();  // 刷新显示
        }

        // 连线模式下刷新临时线显示
        if (wiringMode && startPin) {
            Refresh();
        }

        // 在自动放置模式下实时刷新预览
        if (autoPlaceMode) {
            Refresh();
        }

        // 更新状态栏显示位置信息
        wxWindow* topWindow = wxGetTopLevelParent(this);
        if (topWindow && topWindow->IsKindOf(CLASSINFO(wxFrame))) {
            wxFrame* frame = static_cast<wxFrame*>(topWindow);
            wxStatusBar* statusBar = frame->GetStatusBar();
            if (statusBar) {
                int vx, vy;
                GetViewStart(&vx, &vy);
                wxString posInfo = wxString::Format("Position: (%d,%d)  View: (%d,%d)",
                    lastMousePos.x, lastMousePos.y, vx * 10, vy * 10);
                statusBar->SetStatusText(posInfo, 1);  // 在第二个字段显示

                // 新增：第三个字段显示当前工具信息
                wxString toolInfo = GetToolName(currentTool);
                if (autoPlaceMode) toolInfo += " - Auto Place";
                statusBar->SetStatusText(toolInfo, 2);
            }
        }
    }

    // 鼠标右键按下事件
    void OnRightDown(wxMouseEvent& event) {
        wxPoint pos = event.GetPosition();
        CalcUnscrolledPosition(pos.x, pos.y, &pos.x, &pos.y);
        pos.x /= zoomLevel;
        pos.y /= zoomLevel;

        // 清除之前的选中状态
        if (selectedElement) {
            selectedElement->SetSelected(false);
            selectedElement = nullptr;
        }
        if (selectedWire) {
            selectedWire = nullptr;
        }

        // 首先检查是否点击了导线
        bool wireClicked = false;
        for (auto& wire : wires) {
            if (wire->ContainsPoint(pos)) {
                selectedWire = wire.get();
                wireClicked = true;
                break;
            }
        }

        // 如果没有点击导线，检查元件
        CircuitElement* clickedElement = nullptr;
        if (!wireClicked) {
            for (auto& element : elements) {
                if (element->GetBoundingBox().Contains(pos)) {
                    clickedElement = element.get();
                    selectedElement = clickedElement;
                    selectedElement->SetSelected(true);
                    break;
                }
            }

            // 重置滚动位置
            connectionScrollPos = 0;
            // 新增：更新状态栏显示连接信息
            UpdateStatusBar();
        }

        // 创建上下文菜单
        wxMenu contextMenu;

        if (selectedWire) {
            contextMenu.Append(wxID_DELETE, "Delete Wire");
        }
        else if (selectedElement) {
            contextMenu.Append(wxID_DELETE, "Delete Element");
            contextMenu.Append(wxID_PROPERTIES, "Properties");
        }

        // 绑定上下文菜单事件
        contextMenu.Bind(wxEVT_MENU, [this](wxCommandEvent&) {
            if (HasSelectedWire()) {
                DeleteSelectedWire();
            }
            else if (HasSelectedElements()) {
                DeleteAllSelectedElements();
            }
            }, wxID_DELETE);

        if (!contextMenu.GetMenuItems().empty()) {
            PopupMenu(&contextMenu, event.GetPosition());
        }

        Refresh();
    }

    // 鼠标滚轮事件
    void OnMouseWheel(wxMouseEvent& event) {
        if (event.ControlDown()) {
            // Ctrl+滚轮：缩放
            if (event.GetWheelRotation() > 0) {
                ZoomIn();
            }
            else {
                ZoomOut();
            }
        }
        else if (event.ShiftDown()) {
            // Shift+滚轮：水平滚动连接信息
            if (maxConnectionWidth > 400) {
                int direction = event.GetWheelRotation() > 0 ? -1 : 1;
                ScrollConnectionInfo(direction);
            }
        }
        else {
            // 普通滚轮：滚动
            event.Skip();  // 让父类处理滚动
        }
    }

    // 键盘按下事件
    void OnKeyDown(wxKeyEvent& event) {
        int vx, vy;
        GetViewStart(&vx, &vy);
        wxSize clientSize = GetClientSize();

        switch (event.GetKeyCode()) {
        case WXK_LEFT:
            if (event.ShiftDown() && maxConnectionWidth > 400) {
                // Shift+左箭头：向左滚动连接信息
                ScrollConnectionInfo(-1);
            }
            else {
                // 普通左箭头：向左滚动视图
                Scroll(vx - 1, vy);
                Refresh();
            }
            break;
        case WXK_RIGHT:
            if (event.ShiftDown() && maxConnectionWidth > 400) {
                // Shift+右箭头：向右滚动连接信息
                ScrollConnectionInfo(1);
            }
            else {
                // 普通右箭头：向右滚动视图
                Scroll(vx + 1, vy);
                Refresh();
            }
            break;
        case WXK_DELETE:
            // 优先删除选中的导线
            if (HasSelectedWire()) {
                DeleteSelectedWire();
            }
            // 然后删除选中的元件
            else if (HasSelectedElements()) {
                DeleteAllSelectedElements();
            }
            break;
        case WXK_ESCAPE:
            if (autoPlaceMode) {
                // 退出自动放置模式
                autoPlaceMode = false;
                SetCursor(wxCursor(wxCURSOR_ARROW));
                Refresh();
            }
            else if (wiringMode && startPin) {
                startPin = nullptr;  // 取消连线
                Refresh();
            }
            break;
        case 'G':
            ToggleGrid();  // 切换网格显示
            break;
        case 'Z':
            if (event.ControlDown()) {
                if (event.ShiftDown()) {
                    Redo(); // Ctrl+Shift+Z 重做
                }
                else {
                    Undo(); // Ctrl+Z 撤销
                }
            }
            break;
        case 'Y':
            if (event.ControlDown()) {
                Redo(); // Ctrl+Y 重做
            }
            break;
        case 'A':
            if (event.ControlDown()) {
                SelectAllElements(); // Ctrl+A 全选
            }
            break;
        case 'C':
            if (event.ControlDown()) {
                CopySelectedElements(); // Ctrl+C 复制
            }
            break;
        case 'V':
            if (event.ControlDown()) {
                PasteElements(); // Ctrl+V 粘贴
            }
            break;
        case WXK_UP:
            Scroll(vx, vy - 1);
            Refresh();
            break;
        case WXK_DOWN:
            Scroll(vx, vy + 1);
            Refresh();
            break;
        case WXK_PAGEUP:
            Scroll(vx, vy - (clientSize.y / 20));
            Refresh();
            break;
        case WXK_PAGEDOWN:
            Scroll(vx, vy + (clientSize.y / 20));
            Refresh();
            break;
        case WXK_HOME:
            Scroll(0, 0);
            Refresh();
            break;
        case WXK_END:
            // 滚动到右下角（简化实现）
            Scroll(100, 100);
            Refresh();
            break;

        default:
            event.Skip();
            break;
        }
    }

    // 窗口大小改变事件
    void OnSize(wxSizeEvent& event) {
        Refresh();  // 刷新显示
        event.Skip();  // 传递事件
    }

    // 滚动事件处理
    void OnScroll(wxScrollWinEvent& event) {
        Refresh();  // 滚动时刷新显示
        event.Skip();  // 继续处理事件
    }

    // 在指定位置查找引脚
    Pin* FindPinAt(const wxPoint& pos) {
        for (auto& element : elements) {
            for (auto pin : element->GetPins()) {
                int dx = pin->GetX() - pos.x;
                int dy = pin->GetY() - pos.y;
                if (std::sqrt(dx * dx + dy * dy) < 5) {  // 5像素范围内
                    return pin;
                }
            }
        }
        return nullptr;  // 未找到引脚
    }

    // 获取工具名称
    wxString GetToolName(ElementType tool) {
        switch (tool) {
        case TYPE_SELECT: return "Selection Tool";
        case TYPE_TOGGLE_VALUE: return "Toggle Value Tool";
        case TYPE_AND: return "AND Gate";
        case TYPE_OR: return "OR Gate";
        case TYPE_NOT: return "NOT Gate";
        case TYPE_XOR: return "XOR Gate";
        case TYPE_NAND: return "NAND Gate";
        case TYPE_NOR: return "NOR Gate";
        case TYPE_INPUT: return "Input Pin";
        case TYPE_OUTPUT: return "Output Pin";
        case TYPE_WIRE: return "Wire Tool";
        default: return "Unknown Tool";
        }
    }

    // 获取当前视图中心
    wxPoint GetViewCenter() {
        int vx, vy;
        GetViewStart(&vx, &vy);
        wxSize clientSize = GetClientSize();
        return wxPoint(
            vx * 10 + clientSize.x / (2 * zoomLevel),
            vy * 10 + clientSize.y / (2 * zoomLevel)
        );
    }

    // 将视图中心设置为指定位置
    void CenterView(const wxPoint& center) {
        wxSize clientSize = GetClientSize();
        int newVX = (center.x - clientSize.x / (2 * zoomLevel)) / 10;
        int newVY = (center.y - clientSize.y / (2 * zoomLevel)) / 10;
        Scroll(newVX, newVY);
    }

    // 更新滚动条
    void UpdateScrollbars() {
        SetVirtualSize(
            virtualSize.x * zoomLevel,
            virtualSize.y * zoomLevel
        );
        SetScrollRate(20 * zoomLevel, 20 * zoomLevel);
    }
};

#endif
