#pragma once
#ifndef MAINMENU_H
#define MAINMENU_H

class MainMenu : public wxMenuBar {
public:
    MainMenu(CircuitCanvas* canvas) : canvas(canvas) {
        // 文件菜单
        wxMenu* fileMenu = new wxMenu();
        fileMenu->Append(wxID_NEW, "&New\tCtrl+N", "Create a new circuit");
        fileMenu->Append(wxID_OPEN, "&Open\tCtrl+O", "Open a circuit file");
        fileMenu->Append(wxID_SAVE, "&Save\tCtrl+S", "Save the circuit");
        fileMenu->Append(wxID_SAVEAS, "Save &As...", "Save the circuit with a new name");
        fileMenu->AppendSeparator();
        fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4", "Exit the application");

        // 编辑菜单
        wxMenu* editMenu = new wxMenu();
        editMenu->Append(wxID_UNDO, "&Undo\tCtrl+Z", "Undo the last action");
        editMenu->Append(wxID_REDO, "&Redo\tCtrl+Y", "Redo the undone action");
        editMenu->AppendSeparator();
        editMenu->Append(wxID_CUT, "Cu&t\tCtrl+X", "Cut the selection");
        editMenu->Append(wxID_COPY, "&Copy\tCtrl+C", "Copy the selection");
        editMenu->Append(wxID_PASTE, "&Paste\tCtrl+V", "Paste from clipboard");
        editMenu->AppendSeparator();
        editMenu->Append(wxID_DELETE, "&Delete\tDel", "Delete the selection");
        editMenu->Append(ID_DELETE_ALL, "Delete &All", "Delete all elements");
        editMenu->Append(wxID_SELECTALL, "Select &All\tCtrl+A", "Select all elements");
        editMenu->AppendSeparator();
        editMenu->Append(ID_RENAME, "Re&name\tF2", "Rename selected element");

        // 模拟菜单
        wxMenu* simMenu = new wxMenu();
        simMenu->Append(ID_START_SIM, "&Start Simulation\tF5", "Start circuit simulation");
        simMenu->Append(ID_STOP_SIM, "S&top Simulation\tF6", "Stop circuit simulation");
        simMenu->AppendSeparator();
        simMenu->Append(ID_RESET, "&Reset", "Reset the simulation");
        simMenu->Append(ID_STEP, "&Step\tF7", "Single simulation step");
        simMenu->AppendSeparator();
        simMenu->Append(ID_TRUTH_TABLE, "&Truth Table\tT", "Show truth table");

        // 视图菜单
        wxMenu* viewMenu = new wxMenu();
        viewMenu->Append(ID_ZOOM_IN, "Zoom &In\tCtrl++", "Zoom in");
        viewMenu->Append(ID_ZOOM_OUT, "Zoom &Out\tCtrl+-", "Zoom out");
        viewMenu->Append(ID_ZOOM_RESET, "Zoom &Reset\tCtrl+0", "Reset zoom to 100%");
        viewMenu->AppendSeparator();
        viewMenu->AppendCheckItem(ID_SHOW_GRID, "Show &Grid\tG", "Toggle grid display");
        viewMenu->Check(ID_SHOW_GRID, true);
        viewMenu->Append(ID_TOGGLE_GRID, "Toggle &Grid\tG", "Toggle grid display");
        viewMenu->AppendSeparator();
        viewMenu->Append(ID_CENTER_VIEW, "&Center View\tCtrl+C", "Center the view");
        viewMenu->Append(ID_FIT_TO_WINDOW, "&Fit to Window\tCtrl+F", "Fit circuit to window");

        // 工具菜单 - 添加时序元件
        wxMenu* toolsMenu = new wxMenu();
        toolsMenu->AppendRadioItem(ID_TOOL_SELECT, "&Select Tool\tS", "Selection tool");
        toolsMenu->AppendRadioItem(ID_TOOL_TOGGLE_VALUE, "Toggle &Value Tool\tV", "Toggle input values tool");
        toolsMenu->AppendRadioItem(ID_TOOL_WIRE, "&Wire Tool\tW", "Wire connection tool");
        toolsMenu->AppendSeparator();

        // 组合逻辑元件
        toolsMenu->AppendRadioItem(ID_TOOL_AND, "&AND Gate\tA", "AND Gate tool");
        toolsMenu->AppendRadioItem(ID_TOOL_OR, "&OR Gate\tO", "OR Gate tool");
        toolsMenu->AppendRadioItem(ID_TOOL_NOT, "&NOT Gate\tN", "NOT Gate tool");
        toolsMenu->AppendRadioItem(ID_TOOL_XOR, "&XOR Gate\tX", "XOR Gate tool");
        toolsMenu->AppendRadioItem(ID_TOOL_NAND, "&NAND Gate", "NAND Gate tool");
        toolsMenu->AppendRadioItem(ID_TOOL_NOR, "N&OR Gate", "NOR Gate tool");
        toolsMenu->AppendSeparator();

        // 时序逻辑元件
        toolsMenu->AppendRadioItem(ID_TOOL_CLOCK, "C&lock Signal\tC", "Clock signal tool");
        toolsMenu->AppendRadioItem(ID_TOOL_D_FLIPFLOP, "&D Flip-Flop\tD", "D Flip-Flop tool");
        toolsMenu->AppendRadioItem(ID_TOOL_JK_FLIPFLOP, "&JK Flip-Flop\tJ", "JK Flip-Flop tool");
        toolsMenu->AppendRadioItem(ID_TOOL_T_FLIPFLOP, "&T Flip-Flop\tT", "T Flip-Flop tool");
        toolsMenu->AppendRadioItem(ID_TOOL_REGISTER, "&Register\tR", "4-bit Register tool");
        toolsMenu->AppendSeparator();

        // 输入输出
        toolsMenu->AppendRadioItem(ID_TOOL_INPUT, "&Input Pin\tI", "Input Pin tool");
        toolsMenu->AppendRadioItem(ID_TOOL_OUTPUT, "&Output Pin\tP", "Output Pin tool");

        // 帮助菜单
        wxMenu* helpMenu = new wxMenu();
        helpMenu->Append(wxID_ABOUT, "&About", "About this application");
        helpMenu->Append(wxID_HELP, "&Help\tF1", "Show help documentation");

        // 添加到菜单栏
        Append(fileMenu, "&File");
        Append(editMenu, "&Edit");
        Append(simMenu, "&Simulation");
        Append(viewMenu, "&View");
        Append(toolsMenu, "&Tools");
        Append(helpMenu, "&Help");
    }

    enum {
        ID_START_SIM = wxID_HIGHEST + 1,
        ID_STOP_SIM,
        ID_RESET,
        ID_STEP,
        ID_ZOOM_IN,
        ID_ZOOM_OUT,
        ID_ZOOM_RESET,
        ID_SHOW_GRID,
        ID_TOGGLE_GRID,
        ID_TOOL_SELECT,
        ID_TOOL_TOGGLE_VALUE,
        ID_TOOL_WIRE,
        ID_TOOL_AND,
        ID_TOOL_OR,
        ID_TOOL_NOT,
        ID_TOOL_XOR,
        ID_TOOL_NAND,
        ID_TOOL_NOR,
        ID_TOOL_CLOCK,
        ID_TOOL_RS_FLIPFLOP,
        ID_TOOL_D_FLIPFLOP,
        ID_TOOL_JK_FLIPFLOP,
        ID_TOOL_T_FLIPFLOP,
        ID_TOOL_REGISTER,
        ID_TOOL_INPUT,
        ID_TOOL_OUTPUT,
        ID_RENAME,
        ID_DELETE_ALL,
        ID_DELETE,
        ID_TRUTH_TABLE,
        ID_CENTER_VIEW,
        ID_FIT_TO_WINDOW
    };

private:
    CircuitCanvas* canvas;
};
#endif
