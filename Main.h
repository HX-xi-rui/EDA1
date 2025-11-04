#pragma once
#ifndef MAIN_H
#define MAIN_H
              
#include <wx/splitter.h>      // 分割窗口      
#include "Pin.h"
#include "CircuitCanvas.h"
#include "Gate.h"
#include "InputOutput.h"
#include "Sequence.h"
#include "Wire.h"
#include "CircuitElement.h"
#include "ElementTreeCtrl.h"
#include "TruthTableDialog.h"
#include "PropertiesPanel.h"
#include "MainToolbar.h"
#include "MainMenu.h"

// 主窗口类
class MainFrame : public wxFrame {
public:
    // 构造函数
    MainFrame() : wxFrame(nullptr, wxID_ANY, "Logisim-like Circuit Simulator",
        wxDefaultPosition, wxSize(1200, 800)) {  // 设置窗口标题和初始大小

        // 创建主分割窗口
        mainSplitter = new wxSplitterWindow(this, wxID_ANY);

        // 创建左侧分割窗口（树控件和画布）
        leftSplitter = new wxSplitterWindow(mainSplitter, wxID_ANY);

        // 创建画布
        canvas = new CircuitCanvas(leftSplitter);

        // 创建元件树控件（现在传递正确的 canvas 引用）
        elementTree = new ElementTreeCtrl(leftSplitter, canvas);

        // 设置左侧分割窗口：垂直分割，树控件宽度200
        leftSplitter->SplitVertically(elementTree, canvas, 200);
        leftSplitter->SetMinimumPaneSize(100);  // 设置最小窗格大小

        // 创建属性面板
        propertiesPanel = new PropertiesPanel(mainSplitter, canvas);

        // 设置主分割窗口：垂直分割，左侧宽度800
        mainSplitter->SplitVertically(leftSplitter, propertiesPanel, 750);
        mainSplitter->SetMinimumPaneSize(200);  // 设置最小窗格大小

        // 创建菜单栏和工具栏
        menuBar = new MainMenu(canvas);
        SetMenuBar(menuBar);  // 设置菜单栏

        toolBar = new MainToolbar(this, canvas);
        SetToolBar(toolBar);  // 设置工具栏

        // 创建状态栏 - 修改为3个字段
        CreateStatusBar(3);
        GetStatusBar()->SetStatusText("Ready", 0);  // 第一个字段显示连接信息
        GetStatusBar()->SetStatusText("", 1);       // 第二个字段用于显示位置信息
        GetStatusBar()->SetStatusText("", 2);       // 第三个字段用于显示工具信息

        // 设置状态栏字段宽度
        int widths[] = { 400, 200, -1 };  // 第一个字段显示连接信息，第二个字段显示位置，第三个字段自动扩展
        GetStatusBar()->SetStatusWidths(3, widths);

        // 绑定事件
        Bind(wxEVT_MENU, &MainFrame::OnMenuEvent, this);        // 菜单事件
        Bind(wxEVT_TOOL, &MainFrame::OnToolEvent, this);        // 工具栏事件
        Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);    // 关闭事件

        // 设置最小大小
        SetMinClientSize(wxSize(800, 600));

        // 初始化属性面板
        propertiesPanel->UpdateProperties();

        // 初始化元件树
        elementTree->UpdateTree();
    }

private:
    // 私有成员变量
    MainMenu* menuBar;                    // 菜单栏指针
    MainToolbar* toolBar;                 // 工具栏指针
    CircuitCanvas* canvas;                // 电路画布指针
    PropertiesPanel* propertiesPanel;     // 属性面板指针
    wxSplitterWindow* mainSplitter;       // 主分割窗口指针
    wxSplitterWindow* leftSplitter;       // 左侧分割窗口指针
    ElementTreeCtrl* elementTree;         // 元件树控件指针
    wxString currentFilename;             // 当前文件名
    bool treeVisible = true;              // 树控件可见性标志

    // 菜单事件处理函数
    void OnMenuEvent(wxCommandEvent& event) {
        int id = event.GetId();  // 获取事件ID

        switch (id) {
        case wxID_CUT:
            if (canvas->HasSelectedElements()) {
                canvas->CopySelectedElements(); // 先复制
                canvas->DeleteAllSelectedElements(); // 然后删除所有选中的元件
                GetStatusBar()->SetStatusText("Cut selected elements");
            }
            break;
        case wxID_UNDO:
            if (canvas->CanUndo()) {
                canvas->Undo();
                propertiesPanel->UpdateProperties();
                GetStatusBar()->SetStatusText("Undo performed");
            }
            break;

        case wxID_REDO:
            if (canvas->CanRedo()) {
                canvas->Redo();
                propertiesPanel->UpdateProperties();
                GetStatusBar()->SetStatusText("Redo performed");
            }
            break;
            // 切换值工具
        case MainMenu::ID_TOOL_TOGGLE_VALUE:
            canvas->SetCurrentTool(TYPE_TOGGLE_VALUE);
            GetStatusBar()->SetStatusText("Toggle Value tool selected - click on input pins to change values");
            break;

            // 切换树控件显示
        case ID_TOGGLE_TREE:
            ToggleElementTree();
            break;

            // 新建电路
        case wxID_NEW:
            if (ConfirmSave()) {
                canvas->Clear();  // 清空画布
                currentFilename = "";  // 重置文件名
                SetTitle("Logisim-like Circuit Simulator - New Circuit");  // 更新标题
                GetStatusBar()->SetStatusText("New circuit created");  // 更新状态栏
                propertiesPanel->UpdateProperties();  // 更新属性面板
            }
            break;

            // 打开电路文件
        case wxID_OPEN: {
            if (ConfirmSave()) {
                wxFileDialog openFileDialog(this, "Open Circuit File", "", "",
                    "Circuit files (*.circ)|*.circ", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

                if (openFileDialog.ShowModal() == wxID_CANCEL)
                    return;

                currentFilename = openFileDialog.GetPath();  // 获取文件路径
                if (canvas->LoadCircuit(currentFilename)) {
                    wxFileName fn(currentFilename);
                    SetTitle(wxString::Format("Logisim-like Circuit Simulator - %s", fn.GetFullName()));  // 更新标题
                    GetStatusBar()->SetStatusText("Circuit loaded successfully");  // 更新状态栏
                    propertiesPanel->UpdateProperties();  // 更新属性面板
                }
                else {
                    wxMessageBox("Failed to load circuit file", "Error", wxOK | wxICON_ERROR, this);
                }
            }
            break;
        }

                      // 保存文件
        case wxID_SAVE:
            if (currentFilename.empty()) {
                OnSaveAs(event);  // 如果无文件名，调用另存为
            }
            else {
                if (canvas->SaveCircuit(currentFilename)) {
                    GetStatusBar()->SetStatusText("Circuit saved successfully");
                }
                else {
                    wxMessageBox("Failed to save circuit file", "Error", wxOK | wxICON_ERROR, this);
                }
            }
            break;

            // 另存为
        case wxID_SAVEAS:
            OnSaveAs(event);
            break;

            // 退出程序
        case wxID_EXIT:
            Close(true);
            break;

            // 开始仿真
        case MainMenu::ID_START_SIM:
            canvas->StartSimulation();
            GetStatusBar()->SetStatusText("Simulation running");
            break;

            // 停止仿真
        case MainMenu::ID_STOP_SIM:
            canvas->StopSimulation();
            GetStatusBar()->SetStatusText("Simulation stopped");
            break;

            // 单步仿真
        case MainMenu::ID_STEP:
            canvas->UpdateCircuit();
            canvas->Refresh();
            GetStatusBar()->SetStatusText("Simulation step executed");
            break;

            // 放大
        case MainMenu::ID_ZOOM_IN:
            canvas->ZoomIn();
            GetStatusBar()->SetStatusText(wxString::Format("Zoom: %.0f%%", canvas->GetZoomLevel() * 100));
            break;

            // 缩小
        case MainMenu::ID_ZOOM_OUT:
            canvas->ZoomOut();
            GetStatusBar()->SetStatusText(wxString::Format("Zoom: %.0f%%", canvas->GetZoomLevel() * 100));
            break;

            // 重置缩放
        case MainMenu::ID_ZOOM_RESET:
            canvas->ResetZoom();
            GetStatusBar()->SetStatusText("Zoom reset to 100%");
            break;

            // 切换网格显示
        case MainMenu::ID_TOGGLE_GRID:
            canvas->ToggleGrid();
            GetStatusBar()->SetStatusText("Grid toggled");
            break;

            // 居中视图
        case MainMenu::ID_CENTER_VIEW:
            canvas->Scroll(50, 50);
            GetStatusBar()->SetStatusText("View centered");
            break;

            // 适应窗口
        case MainMenu::ID_FIT_TO_WINDOW:
            canvas->ResetZoom();
            canvas->Scroll(0, 0);
            GetStatusBar()->SetStatusText("Circuit fitted to window");
            break;

            // 选择工具
        case MainMenu::ID_TOOL_SELECT:
            canvas->SetCurrentTool(TYPE_SELECT);
            GetStatusBar()->SetStatusText("Selection tool activated");
            break;

            // 连线工具
        case MainMenu::ID_TOOL_WIRE:
            canvas->SetCurrentTool(TYPE_WIRE);
            GetStatusBar()->SetStatusText("Wire tool selected - click on pins to connect");
            break;

            // AND门工具
        case MainMenu::ID_TOOL_AND:
            canvas->SetCurrentTool(TYPE_AND);
            GetStatusBar()->SetStatusText("AND Gate tool selected");
            break;

            // OR门工具
        case MainMenu::ID_TOOL_OR:
            canvas->SetCurrentTool(TYPE_OR);
            GetStatusBar()->SetStatusText("OR Gate tool selected");
            break;

            // NOT门工具
        case MainMenu::ID_TOOL_NOT:
            canvas->SetCurrentTool(TYPE_NOT);
            GetStatusBar()->SetStatusText("NOT Gate tool selected");
            break;

            // XOR门工具
        case MainMenu::ID_TOOL_XOR:
            canvas->SetCurrentTool(TYPE_XOR);
            GetStatusBar()->SetStatusText("XOR Gate tool selected");
            break;

            // NAND门工具
        case MainMenu::ID_TOOL_NAND:
            canvas->SetCurrentTool(TYPE_NAND);
            GetStatusBar()->SetStatusText("NAND Gate tool selected");
            break;

            // NOR门工具
        case MainMenu::ID_TOOL_NOR:
            canvas->SetCurrentTool(TYPE_NOR);
            GetStatusBar()->SetStatusText("NOR Gate tool selected");
            break;

            // 时钟工具
        case MainMenu::ID_TOOL_CLOCK:
            canvas->SetCurrentTool(TYPE_CLOCK);
            GetStatusBar()->SetStatusText("Clock tool selected");
            break;

            // RS触发器工具
        case MainMenu::ID_TOOL_RS_FLIPFLOP:
            canvas->SetCurrentTool(TYPE_RS_FLIPFLOP);
            GetStatusBar()->SetStatusText("RS Flip-Flop tool selected");
            break;
            // D触发器工具
        case MainMenu::ID_TOOL_D_FLIPFLOP:
            canvas->SetCurrentTool(TYPE_D_FLIPFLOP);
            GetStatusBar()->SetStatusText("D Flip-Flop tool selected");
            break;

            // JK触发器工具
        case MainMenu::ID_TOOL_JK_FLIPFLOP:
            canvas->SetCurrentTool(TYPE_JK_FLIPFLOP);
            GetStatusBar()->SetStatusText("JK Flip-Flop tool selected");
            break;

            // T触发器工具
        case MainMenu::ID_TOOL_T_FLIPFLOP:
            canvas->SetCurrentTool(TYPE_T_FLIPFLOP);
            GetStatusBar()->SetStatusText("T Flip-Flop tool selected");
            break;

            // 寄存器工具
        case MainMenu::ID_TOOL_REGISTER:
            canvas->SetCurrentTool(TYPE_REGISTER);
            GetStatusBar()->SetStatusText("Register tool selected");
            break;

            // 输入引脚工具
        case MainMenu::ID_TOOL_INPUT:
            canvas->SetCurrentTool(TYPE_INPUT);
            GetStatusBar()->SetStatusText("Input Pin tool selected");
            break;

            // 输出引脚工具
        case MainMenu::ID_TOOL_OUTPUT:
            canvas->SetCurrentTool(TYPE_OUTPUT);
            GetStatusBar()->SetStatusText("Output Pin tool selected");
            break;

            // 重命名元件
        case MainMenu::ID_RENAME:
            if (canvas->GetSelectedElement()) {
                wxString newName = wxGetTextFromUser("Enter new name:", "Rename Element",
                    canvas->GetSelectedElement()->GetDisplayName(), this);
                if (!newName.empty()) {
                    canvas->RenameSelectedElement(newName);
                    propertiesPanel->UpdateProperties();  // 更新属性面板
                }
            }
            break;

            // 删除所有元件
        case MainMenu::ID_DELETE_ALL:
            canvas->DeleteAll();
            GetStatusBar()->SetStatusText("All elements deleted");
            break;

            // 删除选择元件
        case MainToolbar::ID_DELETE:
            if (canvas->HasSelectedWire()) {
                canvas->DeleteSelectedWire();
            }
            // 然后删除选中的单个元件
            if (canvas->HasOneSelectedElement()) {
                canvas->DeleteSelectedElement();
            }
            // 最后删除所有选中的元件（多选情况）
            if (canvas->HasSelectedElements()) {
                canvas->DeleteAllSelectedElements();
            }
            break;

        case wxID_DELETE:
            if (canvas->HasSelectedWire()) {
                canvas->DeleteSelectedWire();
            }
            // 然后删除选中的单个元件
            if (canvas->HasOneSelectedElement()) {
                canvas->DeleteSelectedElement();
            }
            // 最后删除所有选中的元件（多选情况）
            if (canvas->HasSelectedElements()) {
                canvas->DeleteAllSelectedElements();
            }
            break;
            // 显示真值表
        case MainMenu::ID_TRUTH_TABLE:
            canvas->ShowTruthTable();
            GetStatusBar()->SetStatusText("Truth table displayed");
            break;

            // 关于对话框
        case wxID_ABOUT:
            wxMessageBox("Logisim-like Circuit Simulator\n\n"
                "A simple digital logic circuit simulator built with wxWidgets\n\n"
                "Features:\n"
                "- Basic logic gates (AND, OR, NOT, XOR, NAND, NOR)\n"
                "- Sequential elements (Flip-Flops, Registers, Clock)\n"
                "- Input and output pins\n"
                "- Wire connections\n"
                "- Real-time simulation\n"
                "- Grid alignment\n"
                "- Zoom and scroll functionality\n"
                "- Property editing\n"
                "- File save/load functionality\n"
                "- Truth table generation\n"
                "- Delete all elements functionality",
                "About", wxOK | wxICON_INFORMATION, this);
            break;

            // 帮助对话框
        case wxID_HELP:
            wxMessageBox("Help Documentation\n\n"
                "1. Select a tool from the toolbar or Tools menu\n"
                "2. Click on the canvas to place components\n"
                "3. Use the Wire tool to connect components\n"
                "4. Right-click to select components\n"
                "5. Press Delete to delete selected components\n"
                "6. Use Simulation menu to start/stop simulation\n"
                "7. Use View menu to toggle grid display, zoom and scroll\n"
                "8. Use Properties panel to edit component properties\n"
                "9. Use Truth Table to see circuit logic\n"
                "10. Use Delete All to remove all elements\n\n"
                "Keyboard Shortcuts:\n"
                "A - AND Gate\n"
                "O - OR Gate\n"
                "N - NOT Gate\n"
                "X - XOR Gate\n"
                "I - Input Pin\n"
                "P - Output Pin\n"
                "W - Wire Tool\n"
                "S - Selection Tool\n"
                "G - Toggle Grid\n"
                "T - Truth Table\n"
                "Delete - Delete selected component\n"
                "F2 - Rename selected element\n"
                "Ctrl++ - Zoom in\n"
                "Ctrl+- - Zoom out\n"
                "Ctrl+0 - Reset zoom\n"
                "Arrow Keys - Scroll view\n"
                "PageUp/PageDown - Fast scroll",
                "Help", wxOK | wxICON_INFORMATION, this);
            break;


        default:
            event.Skip();  // 传递未处理的事件
            break;
        }
    }

    // 工具栏事件处理函数
    void OnToolEvent(wxCommandEvent& event) {
        int id = event.GetId();

        switch (id) {

        case wxID_UNDO:
            if (canvas->CanUndo()) {
                canvas->Undo();
                propertiesPanel->UpdateProperties();
                GetStatusBar()->SetStatusText("Undo performed");
            }
            break;

        case wxID_REDO:
            if (canvas->CanRedo()) {
                canvas->Redo();
                propertiesPanel->UpdateProperties();
                GetStatusBar()->SetStatusText("Redo performed");
            }
            break;
        case MainToolbar::ID_SELECT:
            canvas->SetCurrentTool(TYPE_SELECT);
            GetStatusBar()->SetStatusText("Selection tool activated");
            break;

        case MainToolbar::ID_WIRE:
            canvas->SetCurrentTool(TYPE_WIRE);
            GetStatusBar()->SetStatusText("Wire tool selected - click on pins to connect");
            break;

        case MainToolbar::ID_AND:
            canvas->SetCurrentTool(TYPE_AND);
            GetStatusBar()->SetStatusText("AND Gate tool selected");
            break;

        case MainToolbar::ID_OR:
            canvas->SetCurrentTool(TYPE_OR);
            GetStatusBar()->SetStatusText("OR Gate tool selected");
            break;

        case MainToolbar::ID_NOT:
            canvas->SetCurrentTool(TYPE_NOT);
            GetStatusBar()->SetStatusText("NOT Gate tool selected");
            break;

        case MainToolbar::ID_XOR:
            canvas->SetCurrentTool(TYPE_XOR);
            GetStatusBar()->SetStatusText("XOR Gate tool selected");
            break;

        case MainToolbar::ID_NAND:
            canvas->SetCurrentTool(TYPE_NAND);
            GetStatusBar()->SetStatusText("NAND Gate tool selected");
            break;

        case MainToolbar::ID_NOR:
            canvas->SetCurrentTool(TYPE_NOR);
            GetStatusBar()->SetStatusText("NOR Gate tool selected");
            break;

        case MainToolbar::ID_CLOCK:
            canvas->SetCurrentTool(TYPE_CLOCK);
            GetStatusBar()->SetStatusText("Clock tool selected");
            break;

        case MainToolbar::ID_D_FLIPFLOP:
            canvas->SetCurrentTool(TYPE_D_FLIPFLOP);
            GetStatusBar()->SetStatusText("D Flip-Flop tool selected");
            break;

        case MainToolbar::ID_JK_FLIPFLOP:
            canvas->SetCurrentTool(TYPE_JK_FLIPFLOP);
            GetStatusBar()->SetStatusText("JK Flip-Flop tool selected");
            break;

        case MainToolbar::ID_T_FLIPFLOP:
            canvas->SetCurrentTool(TYPE_T_FLIPFLOP);
            GetStatusBar()->SetStatusText("T Flip-Flop tool selected");
            break;

        case MainToolbar::ID_REGISTER:
            canvas->SetCurrentTool(TYPE_REGISTER);
            GetStatusBar()->SetStatusText("Register tool selected");
            break;

        case MainToolbar::ID_INPUT:
            canvas->SetCurrentTool(TYPE_INPUT);
            GetStatusBar()->SetStatusText("Input Pin tool selected");
            break;

        case MainToolbar::ID_OUTPUT:
            canvas->SetCurrentTool(TYPE_OUTPUT);
            GetStatusBar()->SetStatusText("Output Pin tool selected");
            break;

        case MainToolbar::ID_START_SIM:
            canvas->StartSimulation();
            GetStatusBar()->SetStatusText("Simulation running");
            break;

        case MainToolbar::ID_STOP_SIM:
            canvas->StopSimulation();
            GetStatusBar()->SetStatusText("Simulation stopped");
            break;

        case MainToolbar::ID_STEP:
            canvas->UpdateCircuit();
            canvas->Refresh();
            GetStatusBar()->SetStatusText("Simulation step executed");
            break;

        case MainToolbar::ID_DELETE_ALL:
            canvas->DeleteAll();
            GetStatusBar()->SetStatusText("All elements deleted");
            break;

        case MainToolbar::ID_DELETE:
            if (canvas->HasSelectedWire()) {
                canvas->DeleteSelectedWire();
            }
            // 然后删除选中的单个元件
            if (canvas->HasOneSelectedElement()) {
                canvas->DeleteSelectedElement();
            }
            // 最后删除所有选中的元件（多选情况）
            if (canvas->HasSelectedElements()) {
                canvas->DeleteAllSelectedElements();
            }
            break;

        case MainToolbar::ID_TRUTH_TABLE:
            canvas->ShowTruthTable();
            GetStatusBar()->SetStatusText("Truth table displayed");
            break;

            // 剪切
        case wxID_CUT:
            if (canvas->HasSelectedElements()) {
                canvas->CopySelectedElements();
                canvas->DeleteAllSelectedElements();
                GetStatusBar()->SetStatusText("Cut selected elements");
            }
            break;

            // 复制
        case wxID_COPY:
            canvas->CopySelectedElements();
            GetStatusBar()->SetStatusText("Copied selected elements");
            break;

            // 粘贴
        case wxID_PASTE:
            canvas->PasteElements();
            GetStatusBar()->SetStatusText("Pasted elements");
            break;

            // 全选
        case wxID_SELECTALL:
            canvas->SelectAllElements();
            GetStatusBar()->SetStatusText("Selected all elements");
            break;

        case wxID_DELETE:
            if (canvas->HasSelectedWire()) {
                canvas->DeleteSelectedWire();
            }
            // 然后删除选中的单个元件
            if (canvas->HasOneSelectedElement()) {
                canvas->DeleteSelectedElement();
            }
            // 最后删除所有选中的元件（多选情况）
            if (canvas->HasSelectedElements()) {
                canvas->DeleteAllSelectedElements();
            }
            break;

        default:
            event.Skip();  // 传递未处理的事件
            break;
        }
        toolBar->EnableTool(wxID_UNDO, canvas->CanUndo());
        toolBar->EnableTool(wxID_REDO, canvas->CanRedo());
        propertiesPanel->UpdateProperties();  // 更新属性面板
    }



    // 切换元件树显示状态
    void ToggleElementTree() {
        treeVisible = !treeVisible;  // 切换可见性标志

        if (treeVisible) {
            // 显示树控件
            leftSplitter->SplitVertically(elementTree, canvas, 200);
            GetStatusBar()->SetStatusText("Element tree shown");
        }
        else {
            // 隐藏树控件
            leftSplitter->Unsplit(elementTree);
            GetStatusBar()->SetStatusText("Element tree hidden");
        }

        leftSplitter->Layout();  // 重新布局
    }

    // 另存为处理函数
    void OnSaveAs(wxCommandEvent& event) {
        wxFileDialog saveFileDialog(this, "Save Circuit File", "", "",
            "Circuit files (*.circ)|*.circ", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL)
            return;

        currentFilename = saveFileDialog.GetPath();  // 获取文件路径
        if (!currentFilename.Contains(".")) {
            currentFilename += ".circ";  // 添加文件扩展名
        }

        if (canvas->SaveCircuit(currentFilename)) {
            wxFileName fn(currentFilename);
            SetTitle(wxString::Format("Logisim-like Circuit Simulator - %s", fn.GetFullName()));  // 更新标题
            GetStatusBar()->SetStatusText("Circuit saved successfully");  // 更新状态栏
        }
        else {
            wxMessageBox("Failed to save circuit file", "Error", wxOK | wxICON_ERROR, this);
        }
    }

    // 确认保存（简化实现）
    bool ConfirmSave() {
        // 在实际应用中，这里应该检查电路是否已修改
        return true;
    }

    // 关闭窗口事件处理
    void OnClose(wxCloseEvent& event) {

        if (wxMessageBox("Are you sure you want to exit?", "Confirm Exit",
            wxYES_NO | wxICON_QUESTION, this) == wxYES) {
            Destroy();  // 销毁窗口
        }
        else {
            event.Veto();  // 阻止关闭
        }
    }

};
#endif
