#pragma once
#ifndef MAINTOOLBAR_H
#define MAINTOOLBAR_H

#include "MainMenu.h"
// 工具栏类
class MainToolbar : public wxToolBar {
public:
    MainToolbar(wxWindow* parent, CircuitCanvas* canvas) : wxToolBar(parent, wxID_ANY), canvas(canvas) {
        AddTool(wxID_NEW, "New", wxArtProvider::GetBitmap(wxART_NEW), "New Circuit");
        AddTool(wxID_OPEN, "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN), "Open Circuit");
        AddTool(wxID_SAVE, "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE), "Save Circuit");

        // 撤销/重做按钮
        AddTool(wxID_UNDO, "Undo", wxArtProvider::GetBitmap(wxART_UNDO), "Undo (Ctrl+Z)");
        AddTool(wxID_REDO, "Redo", wxArtProvider::GetBitmap(wxART_REDO), "Redo (Ctrl+Y)");

        AddTool(ID_SELECT, "Select", wxBitmap(wxT("images/mouse.png"), wxBITMAP_TYPE_PNG), "Select", wxITEM_RADIO);
        AddTool(ID_TOGGLE_VALUE, "Toggle Value", wxArtProvider::GetBitmap(wxART_TIP), "Toggle Value Tool", wxITEM_RADIO);
        AddTool(ID_WIRE, "Wire", wxArtProvider::GetBitmap(wxART_PLUS), "Wire Tool", wxITEM_RADIO);


        AddTool(ID_AND, "AND", wxBitmap(wxT("images/AND.png"), wxBITMAP_TYPE_PNG), "AND Gate", wxITEM_RADIO);
        AddTool(ID_OR, "OR", wxBitmap(wxT("images/OR.png"), wxBITMAP_TYPE_PNG), "OR Gate", wxITEM_RADIO);
        AddTool(ID_NOT, "NOT", wxBitmap(wxT("images/NOT.png"), wxBITMAP_TYPE_PNG), "NOT Gate", wxITEM_RADIO);
        AddTool(ID_XOR, "XOR", wxBitmap(wxT("images/XOR.png"), wxBITMAP_TYPE_PNG), "XOR Gate", wxITEM_RADIO);
        AddTool(ID_NAND, "NAND", wxBitmap(wxT("images/NAND.png"), wxBITMAP_TYPE_PNG), "NAND Gate", wxITEM_RADIO);
        AddTool(ID_NOR, "NOR", wxBitmap(wxT("images/NOR.png"), wxBITMAP_TYPE_PNG), "NOR Gate", wxITEM_RADIO);


        AddTool(ID_INPUT, "Input", wxBitmap(wxT("images/INPUT.png"), wxBITMAP_TYPE_PNG), "Input Pin", wxITEM_RADIO);
        AddTool(ID_OUTPUT, "Output", wxBitmap(wxT("images/OUTPUT.png"), wxBITMAP_TYPE_PNG), "Output Pin", wxITEM_RADIO);


        AddTool(ID_START_SIM, "Start", wxArtProvider::GetBitmap(wxART_GO_FORWARD), "Start Simulation");
        AddTool(ID_STOP_SIM, "Stop", wxArtProvider::GetBitmap(wxART_STOP), "Stop Simulation");
        AddTool(ID_TRUTH_TABLE, "Truth Table", wxArtProvider::GetBitmap(wxART_LIST_VIEW), "Show Truth Table");


        AddTool(ID_DELETE_ALL, "Delete All", wxArtProvider::GetBitmap(wxART_DELETE), "Delete All Elements");
        AddTool(ID_DELETE, "Delete", wxArtProvider::GetBitmap(wxART_DELETE), "Delete Selected Element");

        Realize();
        ToggleTool(ID_SELECT, true);

        // 初始时禁用撤销/重做按钮（因为没有操作历史）
        EnableTool(wxID_UNDO, false);
        EnableTool(wxID_REDO, false);

        // 绑定事件
        Bind(wxEVT_TOOL, &MainToolbar::OnToolClicked, this);

        // 绑定更新UI事件来动态启用/禁用撤销重做按钮
        Bind(wxEVT_UPDATE_UI, &MainToolbar::OnUpdateUI, this, wxID_UNDO);
        Bind(wxEVT_UPDATE_UI, &MainToolbar::OnUpdateUI, this, wxID_REDO);
    }

    enum {
        ID_SELECT = MainMenu::ID_FIT_TO_WINDOW + 1,
        ID_TOGGLE_VALUE,
        ID_WIRE,
        ID_AND,
        ID_OR,
        ID_NOT,
        ID_XOR,
        ID_NAND,
        ID_NOR,
        ID_CLOCK,
        ID_TOOL_RS_FLIPFLOP,
        ID_D_FLIPFLOP,
        ID_JK_FLIPFLOP,
        ID_T_FLIPFLOP,
        ID_REGISTER,
        ID_INPUT,
        ID_OUTPUT,
        ID_START_SIM,
        ID_STOP_SIM,
        ID_STEP,
        ID_DELETE_ALL,
        ID_DELETE,
        ID_TRUTH_TABLE,
        ID_TOGGLE_TREE
    };

private:
    void OnToolClicked(wxCommandEvent& event) {
        int id = event.GetId();
        ElementType toolType = TYPE_SELECT;

        switch (id) {
        case ID_SELECT:
            toolType = TYPE_SELECT;
            break;
        case ID_TOGGLE_VALUE:
            toolType = TYPE_TOGGLE_VALUE;
            break;
        case ID_WIRE:
            toolType = TYPE_WIRE;
            break;
        case ID_AND:
            toolType = TYPE_AND;
            break;
        case ID_OR:
            toolType = TYPE_OR;
            break;
        case ID_NOT:
            toolType = TYPE_NOT;
            break;
        case ID_XOR:
            toolType = TYPE_XOR;
            break;
        case ID_NAND:
            toolType = TYPE_NAND;
            break;
        case ID_NOR:
            toolType = TYPE_NOR;
            break;
        case ID_CLOCK:
            toolType = TYPE_CLOCK;
            break;
        case ID_D_FLIPFLOP:
            toolType = TYPE_D_FLIPFLOP;
            break;
        case ID_JK_FLIPFLOP:
            toolType = TYPE_JK_FLIPFLOP;
            break;
        case ID_T_FLIPFLOP:
            toolType = TYPE_T_FLIPFLOP;
            break;
        case ID_REGISTER:
            toolType = TYPE_REGISTER;
            break;
        case ID_INPUT:
            toolType = TYPE_INPUT;
            break;
        case ID_OUTPUT:
            toolType = TYPE_OUTPUT;
            break;
        default:
            event.Skip();
            return;
        }
        // 设置自动放置模式
        if (toolType != TYPE_SELECT && toolType != TYPE_WIRE && toolType != TYPE_TOGGLE_VALUE) {
            canvas->SetAutoPlaceMode(toolType);
        }
        else {
            canvas->SetCurrentTool(toolType);
        }

        // 更新工具栏状态
        for (int toolId = ID_SELECT; toolId <= ID_OUTPUT; toolId++) {
            ToggleTool(toolId, toolId == id);
        }
    }

    // 更新UI事件处理函数 - 用于动态启用/禁用撤销重做按钮
    void OnUpdateUI(wxUpdateUIEvent& event) {
        int id = event.GetId();

        if (id == wxID_UNDO) {
            event.Enable(canvas->CanUndo());
        }
        else if (id == wxID_REDO) {
            event.Enable(canvas->CanRedo());
        }
        else {
            event.Skip();
        }
    }
    CircuitCanvas* canvas;
};
#endif
