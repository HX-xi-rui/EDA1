#pragma once
#ifndef TRUTHTABLEDIALOG_H
#define TRUTHTABLEDIALOG_H

#include <wx/wx.h>         
#include <wx/grid.h>               // 网格控件     
#include "CircuitCanvas.h"

// 真值表对话框
class TruthTableDialog : public wxDialog {
public:
    TruthTableDialog(wxWindow* parent, CircuitCanvas* canvas)
        : wxDialog(parent, wxID_ANY, "Truth Table", wxDefaultPosition, wxSize(600, 400)), canvas(canvas) {

        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        // 创建网格
        grid = new wxGrid(this, wxID_ANY);
        mainSizer->Add(grid, 1, wxEXPAND | wxALL, 5);

        // 按钮
        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        buttonSizer->Add(new wxButton(this, wxID_CLOSE, "Close"), 0, wxALL, 5);
        buttonSizer->Add(new wxButton(this, wxID_REFRESH, "Refresh"), 0, wxALL, 5);

        mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

        SetSizer(mainSizer);

        // 绑定事件
        Bind(wxEVT_BUTTON, &TruthTableDialog::OnClose, this, wxID_CLOSE);
        Bind(wxEVT_BUTTON, &TruthTableDialog::OnRefresh, this, wxID_REFRESH);

        GenerateTruthTable();
    }

    void GenerateTruthTable() {
        // 从画布获取实际的输入输出引脚
        std::vector<InputOutput*> inputs = canvas->GetInputPins();
        std::vector<InputOutput*> outputs = canvas->GetOutputPins();

        int numInputs = static_cast<int>(inputs.size());
        int numOutputs = static_cast<int>(outputs.size());

        // 清除现有网格
        if (grid->GetNumberRows() > 0) {
            grid->DeleteRows(0, grid->GetNumberRows());
        }
        if (grid->GetNumberCols() > 0) {
            grid->DeleteCols(0, grid->GetNumberCols());
        }

        if (numInputs == 0 && numOutputs == 0) {
            // 没有 IO，显示提示
            grid->CreateGrid(1, 1);
            grid->SetCellValue(0, 0, "No inputs/outputs");
            grid->AutoSize();
            return;
        }

        int rows = (numInputs > 0) ? (1 << numInputs) : 1;
        int cols = numInputs + numOutputs;
        if (cols == 0) cols = 1; // 防御

        grid->CreateGrid(rows, cols);

        // 设置列标签
        for (int i = 0; i < numInputs; ++i) {
            grid->SetColLabelValue(i, wxString::Format("Input %d", i + 1));
        }
        for (int i = 0; i < numOutputs; ++i) {
            grid->SetColLabelValue(numInputs + i, wxString::Format("Output %d", i + 1));
        }

        // 对每个输入组合，设置输入 -> 运行画布仿真 -> 读取输出填表
        for (int row = 0; row < rows; ++row) {
            // 设置输入值（高位到低位对应 Input1..InputN）
            for (int i = 0; i < numInputs; ++i) {
                bool bit = ((row >> (numInputs - i - 1)) & 1) != 0;
                // 将值写入 InputOutput 的 value（它的 Update() 会把值写到 pin）
                inputs[i]->SetValue(bit);
                grid->SetCellValue(row, i, bit ? "1" : "0");
                grid->SetCellAlignment(row, i, wxALIGN_CENTER, wxALIGN_CENTER);
            }

            // 让画布进行一次完整传播（会调用每个 element->Update() 与 wire->Update()）
            canvas->UpdateCircuit();

            // 读取输出引脚的值
            for (int j = 0; j < numOutputs; ++j) {
                bool outVal = outputs[j]->GetValue();
                grid->SetCellValue(row, numInputs + j, outVal ? "1" : "0");
                grid->SetCellAlignment(row, numInputs + j, wxALIGN_CENTER, wxALIGN_CENTER);
            }
        }

        grid->AutoSize();
    }


private:
    void OnClose(wxCommandEvent& event) {
        Close();
    }

    void OnRefresh(wxCommandEvent& event) {
        GenerateTruthTable();
    }

    wxGrid* grid;
    CircuitCanvas* canvas;
};

void CircuitCanvas::ShowTruthTable() {
    TruthTableDialog dialog(GetParent(), this);
    dialog.ShowModal();
}
#endif
