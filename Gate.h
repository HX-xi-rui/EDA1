#pragma once
#ifndef GATE_H
#define GATE_H

// 逻辑门基类
class Gate : public CircuitElement {
public:
    Gate(ElementType type, int x, int y) : CircuitElement(type, x, y) {
        const int PIN_OFFSET = 40; // 扩大后的引脚偏移
        if (type == TYPE_NOT) {
            inputs.push_back(std::make_unique<Pin>(x - PIN_OFFSET, y, true, this));
            outputs.push_back(std::make_unique<Pin>(x + PIN_OFFSET, y, false, this));
        }
        else {
            inputs.push_back(std::make_unique<Pin>(x - PIN_OFFSET, y - 20, true, this));
            inputs.push_back(std::make_unique<Pin>(x - PIN_OFFSET, y + 20, true, this));
            outputs.push_back(std::make_unique<Pin>(x + PIN_OFFSET, y, false, this));
        }
    }

    virtual void Draw(wxDC& dc) override {
        // 保存初始字体状态（关键：记录绘制前的默认字体）
        wxFont originalFont = dc.GetFont();

        wxPen borderPen = selected ? wxPen(*wxRED, 2) : wxPen(*wxBLACK, 1);
        wxBrush fillBrush = *wxWHITE_BRUSH;
        dc.SetPen(borderPen);
        dc.SetBrush(fillBrush);

        const int GATE_WIDTH = 80;
        const int GATE_HEIGHT = 60;
        const int PIN_LENGTH = 16;
        const int NOT_CIRCLE_RADIUS = 8;

        switch (type) {
        case TYPE_AND: {
            const int PIN_OFFSET = 40;
            wxPoint andPoints[] = {
                wxPoint(posX - PIN_OFFSET, posY - GATE_HEIGHT / 2),
                wxPoint(posX - PIN_OFFSET / 3, posY - GATE_HEIGHT / 2),
                wxPoint(posX + PIN_OFFSET, posY),
                wxPoint(posX - PIN_OFFSET / 3, posY + GATE_HEIGHT / 2),
                wxPoint(posX - PIN_OFFSET, posY + GATE_HEIGHT / 2)
            };
            dc.DrawPolygon(5, andPoints);

            if (inputs.size() >= 2) {
                dc.DrawLine(inputs[0]->GetX(), inputs[0]->GetY(), inputs[0]->GetX() - PIN_LENGTH, inputs[0]->GetY());
                dc.DrawLine(inputs[1]->GetX(), inputs[1]->GetY(), inputs[1]->GetX() - PIN_LENGTH, inputs[1]->GetY());
                dc.SetBrush(*wxBLACK_BRUSH);
                dc.DrawCircle(inputs[0]->GetX(), inputs[0]->GetY(), 6);
                dc.DrawCircle(inputs[1]->GetX(), inputs[1]->GetY(), 6);
                dc.SetBrush(*wxWHITE_BRUSH);
            }

            if (!outputs.empty()) {
                dc.DrawLine(outputs[0]->GetX(), outputs[0]->GetY(), outputs[0]->GetX() + PIN_LENGTH, outputs[0]->GetY());
                dc.SetBrush(*wxBLACK_BRUSH);
                dc.DrawCircle(outputs[0]->GetX(), outputs[0]->GetY(), 6);
                dc.SetBrush(*wxWHITE_BRUSH);
            }

            // 设置AND门标签字体（放大）
            wxFont labelFont = originalFont; // 基于初始字体修改，避免多层继承
            labelFont.SetPointSize(14);
            dc.SetFont(labelFont);
            dc.SetTextForeground(*wxBLACK);
            dc.DrawText("AND", posX - 30, posY - 14);
            break;
        }
        case TYPE_OR: {
            const int PIN_OFFSET = 40;
            wxPoint gateCenter(posX, posY);
            int arcLeftX = posX - GATE_WIDTH / 2;
            int arcTopY = posY - GATE_HEIGHT / 2;
            wxRect arcRect(arcLeftX, arcTopY, GATE_WIDTH, GATE_HEIGHT);

            auto DrawEllipticalSector = [](wxDC& dc, const wxRect& rect, double startAngle, double endAngle, int segments = 36) {
                wxPoint center(rect.x + rect.width / 2, rect.y + rect.height / 2);
                double rx = rect.width / 2.0;
                double ry = rect.height / 2.0;
                std::vector<wxPoint> points;
                points.push_back(center);
                for (int i = segments; i >= 0; i--) {
                    double angleRad = startAngle + (endAngle - startAngle) * i / segments;
                    int x = center.x - rx * cos(angleRad);
                    int y = center.y + ry * sin(angleRad);
                    points.push_back(wxPoint(x, y));
                }
                dc.DrawPolygon(static_cast<int>(points.size()), &points[0]);
                };

            DrawEllipticalSector(dc, arcRect, M_PI / 6, 11 * M_PI / 6);

            wxPoint upperInputPin(arcLeftX, posY - GATE_HEIGHT / 4);
            wxPoint lowerInputPin(arcLeftX, posY + GATE_HEIGHT / 4);
            dc.DrawLine(upperInputPin.x - PIN_LENGTH, upperInputPin.y, upperInputPin.x, upperInputPin.y);
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawCircle(upperInputPin, 4);
            dc.SetBrush(*wxWHITE_BRUSH);
            dc.DrawLine(lowerInputPin.x - PIN_LENGTH, lowerInputPin.y, lowerInputPin.x, lowerInputPin.y);
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawCircle(lowerInputPin, 4);
            dc.SetBrush(*wxWHITE_BRUSH);

            wxPoint outputPin(arcLeftX + GATE_WIDTH, posY);
            dc.DrawLine(outputPin.x, outputPin.y, outputPin.x + PIN_LENGTH, outputPin.y);
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawCircle(outputPin, 4);
            dc.SetBrush(*wxWHITE_BRUSH);

            // 设置OR门标签字体（放大）
            wxFont labelFont = originalFont;
            labelFont.SetPointSize(14);
            dc.SetFont(labelFont);
            dc.SetTextForeground(*wxBLACK);
            dc.DrawText("OR", posX - 16, posY - 14);
            break;
        }
        case TYPE_NOT: {
            wxPoint triRight(posX + GATE_WIDTH / 2, posY);
            wxPoint triTop(posX - GATE_WIDTH / 2, posY - GATE_HEIGHT / 2);
            wxPoint triBottom(posX - GATE_WIDTH / 2, posY + GATE_HEIGHT / 2);
            wxPoint triVertices[] = { triRight, triTop, triBottom };
            dc.DrawPolygon(3, triVertices);

            wxPoint inputPin(posX - GATE_WIDTH / 2, posY);
            dc.DrawLine(inputPin.x - PIN_LENGTH, inputPin.y, inputPin.x, inputPin.y);
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawCircle(inputPin, 4);
            dc.SetBrush(*wxWHITE_BRUSH);

            wxPoint outputPin(triRight.x, triRight.y);
            dc.DrawLine(outputPin.x, outputPin.y, outputPin.x + PIN_LENGTH, outputPin.y);
            dc.SetBrush(*wxRED_BRUSH);
            dc.DrawCircle(wxPoint(outputPin.x + NOT_CIRCLE_RADIUS * 2, outputPin.y), NOT_CIRCLE_RADIUS);
            dc.SetBrush(*wxWHITE_BRUSH);

            // 设置NOT门标签字体（放大）
            wxFont labelFont = originalFont;
            labelFont.SetPointSize(14);
            dc.SetFont(labelFont);
            dc.SetTextForeground(*wxBLACK);
            dc.DrawText("NOT", posX - 30, posY - 14);
            break;
        }
        case TYPE_XOR: {
            wxPoint gateCenter(posX, posY);
            int arcLeftX = posX - GATE_WIDTH / 2;
            int arcTopY = posY - GATE_HEIGHT / 2;
            wxRect arcRect(arcLeftX, arcTopY, GATE_WIDTH, GATE_HEIGHT);

            auto DrawEllipticalSector = [](wxDC& dc, const wxRect& rect, double startAngle, double endAngle, int segments = 36) {
                wxPoint center(rect.x + rect.width / 2, rect.y + rect.height / 2);
                double rx = rect.width / 2.0;
                double ry = rect.height / 2.0;
                std::vector<wxPoint> points;
                points.push_back(center);
                for (int i = segments; i >= 0; i--) {
                    double angleRad = startAngle + (endAngle - startAngle) * i / segments;
                    int x = center.x - rx * cos(angleRad);
                    int y = center.y + ry * sin(angleRad);
                    points.push_back(wxPoint(x, y));
                }
                dc.DrawPolygon(static_cast<int>(points.size()), &points[0]);
                };

            DrawEllipticalSector(dc, arcRect, M_PI / 6, 11 * M_PI / 6);

            wxPoint upperInputPin(arcLeftX, posY - GATE_HEIGHT / 4);
            wxPoint lowerInputPin(arcLeftX, posY + GATE_HEIGHT / 4);
            dc.DrawLine(upperInputPin.x - PIN_LENGTH, upperInputPin.y, upperInputPin.x, upperInputPin.y);
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawCircle(upperInputPin, 4);
            dc.SetBrush(*wxWHITE_BRUSH);
            dc.DrawLine(lowerInputPin.x - PIN_LENGTH, lowerInputPin.y, lowerInputPin.x, lowerInputPin.y);
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawCircle(lowerInputPin, 4);
            dc.SetBrush(*wxWHITE_BRUSH);

            wxPoint outputPin(arcLeftX + GATE_WIDTH, posY);
            dc.DrawLine(outputPin.x, outputPin.y, outputPin.x + PIN_LENGTH, outputPin.y);
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawCircle(outputPin, 4);
            dc.SetBrush(*wxWHITE_BRUSH);

            dc.DrawLine(posX + 30, posY - 20, posX - 30, posY + 20);

            // 设置XOR门标签字体（放大）
            wxFont labelFont = originalFont;
            labelFont.SetPointSize(14);
            dc.SetFont(labelFont);
            dc.SetTextForeground(*wxBLACK);
            dc.DrawText("XOR", posX - 21, posY - 14);
            break;
        }
        case TYPE_NAND: {
            const int PIN_OFFSET = 40;
            wxPoint nandPoints[] = {
                wxPoint(posX - PIN_OFFSET, posY - GATE_HEIGHT / 2),
                wxPoint(posX - PIN_OFFSET / 3, posY - GATE_HEIGHT / 2),
                wxPoint(posX + PIN_OFFSET, posY),
                wxPoint(posX - PIN_OFFSET / 3, posY + GATE_HEIGHT / 2),
                wxPoint(posX - PIN_OFFSET, posY + GATE_HEIGHT / 2)
            };
            dc.DrawPolygon(5, nandPoints);

            if (inputs.size() >= 2) {
                dc.DrawLine(inputs[0]->GetX(), inputs[0]->GetY(), inputs[0]->GetX() - PIN_LENGTH, inputs[0]->GetY());
                dc.DrawLine(inputs[1]->GetX(), inputs[1]->GetY(), inputs[1]->GetX() - PIN_LENGTH, inputs[1]->GetY());
                dc.SetBrush(*wxBLACK_BRUSH);
                dc.DrawCircle(inputs[0]->GetX(), inputs[0]->GetY(), 6);
                dc.DrawCircle(inputs[1]->GetX(), inputs[1]->GetY(), 6);
                dc.SetBrush(*wxWHITE_BRUSH);
            }

            if (!outputs.empty()) {
                dc.DrawLine(outputs[0]->GetX(), outputs[0]->GetY(), outputs[0]->GetX() + PIN_LENGTH, outputs[0]->GetY());
                dc.SetBrush(*wxRED_BRUSH);
                dc.DrawCircle(outputs[0]->GetX() + PIN_LENGTH + NOT_CIRCLE_RADIUS, outputs[0]->GetY(), NOT_CIRCLE_RADIUS);
                dc.SetBrush(*wxWHITE_BRUSH);
                dc.SetBrush(*wxBLACK_BRUSH);
                dc.DrawCircle(outputs[0]->GetX(), outputs[0]->GetY(), 6);
                dc.SetBrush(*wxWHITE_BRUSH);
            }

            // 设置NAND门标签字体（放大）
            wxFont labelFont = originalFont;
            labelFont.SetPointSize(14);
            dc.SetFont(labelFont);
            dc.SetTextForeground(*wxBLACK);
            dc.DrawText("NAND", posX - 47, posY - 14);
            break;
        }
        case TYPE_NOR: {
            wxPoint gateCenter(posX, posY);
            int arcLeftX = posX - GATE_WIDTH / 2;
            int arcTopY = posY - GATE_HEIGHT / 2;
            wxRect arcRect(arcLeftX, arcTopY, GATE_WIDTH, GATE_HEIGHT);

            auto DrawEllipticalSector = [](wxDC& dc, const wxRect& rect, double startAngle, double endAngle, int segments = 36) {
                wxPoint center(rect.x + rect.width / 2, rect.y + rect.height / 2);
                double rx = rect.width / 2.0;
                double ry = rect.height / 2.0;
                std::vector<wxPoint> points;
                points.push_back(center);
                for (int i = segments; i >= 0; i--) {
                    double angleRad = startAngle + (endAngle - startAngle) * i / segments;
                    int x = center.x - rx * cos(angleRad);
                    int y = center.y + ry * sin(angleRad);
                    points.push_back(wxPoint(x, y));
                }
                dc.DrawPolygon(static_cast<int>(points.size()), &points[0]);
                };

            DrawEllipticalSector(dc, arcRect, M_PI / 6, 11 * M_PI / 6);

            wxPoint upperInputPin(arcLeftX, posY - GATE_HEIGHT / 4);
            wxPoint lowerInputPin(arcLeftX, posY + GATE_HEIGHT / 4);
            dc.DrawLine(upperInputPin.x - PIN_LENGTH, upperInputPin.y, upperInputPin.x, upperInputPin.y);
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawCircle(upperInputPin, 4);
            dc.SetBrush(*wxWHITE_BRUSH);
            dc.DrawLine(lowerInputPin.x - PIN_LENGTH, lowerInputPin.y, lowerInputPin.x, lowerInputPin.y);
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawCircle(lowerInputPin, 4);
            dc.SetBrush(*wxWHITE_BRUSH);

            wxPoint outputPin(arcLeftX + GATE_WIDTH, posY);
            dc.DrawLine(outputPin.x, outputPin.y, outputPin.x + PIN_LENGTH, outputPin.y);
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawCircle(outputPin, 4);
            dc.SetBrush(*wxWHITE_BRUSH);

            if (!outputs.empty()) {
                dc.DrawLine(outputs[0]->GetX(), outputs[0]->GetY(), outputs[0]->GetX() + PIN_LENGTH, outputs[0]->GetY());
                dc.SetBrush(*wxRED_BRUSH);
                dc.DrawCircle(outputs[0]->GetX() + PIN_LENGTH + NOT_CIRCLE_RADIUS, outputs[0]->GetY(), NOT_CIRCLE_RADIUS);
                dc.SetBrush(*wxWHITE_BRUSH);
                dc.SetBrush(*wxBLACK_BRUSH);
                dc.DrawCircle(outputs[0]->GetX(), outputs[0]->GetY(), 4);
                dc.SetBrush(*wxWHITE_BRUSH);
            }

            // 设置NOR门标签字体（放大）
            wxFont labelFont = originalFont;
            labelFont.SetPointSize(14);
            dc.SetFont(labelFont);
            dc.SetTextForeground(*wxBLACK);
            dc.DrawText("NOR", posX - 21, posY - 14);
            break;
        }
        default:
            break;
        }

        // 绘制引脚圆点
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxWHITE_BRUSH);
        for (auto& pin : inputs) {
            int drawX = pin->GetX();
            int drawY = pin->GetY();
            if (type == TYPE_OR || type == TYPE_NOR || type == TYPE_XOR) {
                if (drawY < posY) drawY += 6;
                else if (drawY > posY) drawY -= 6;
            }
            dc.DrawCircle(drawX, drawY, 6);
        }
        for (auto& pin : outputs) {
            int drawX = pin->GetX();
            dc.DrawCircle(drawX, pin->GetY(), 6);
        }

        // 关键修复：强制恢复初始默认字体，避免影响后续元件
        dc.SetFont(originalFont);
    }

    // 以下Update、GetPins、GetBoundingBox等方法保持不变（与之前提供的代码一致）
    virtual void Update() override {
        if (inputs.empty() || outputs.empty()) return;
        bool result = false;
        switch (type) {
        case TYPE_AND: result = inputs.size() >= 2 ? (inputs[0]->GetValue() && inputs[1]->GetValue()) : false; break;
        case TYPE_OR: result = inputs.size() >= 2 ? (inputs[0]->GetValue() || inputs[1]->GetValue()) : false; break;
        case TYPE_NOT: result = inputs.size() >= 1 ? !inputs[0]->GetValue() : false; break;
        case TYPE_XOR: result = inputs.size() >= 2 ? (inputs[0]->GetValue() != inputs[1]->GetValue()) : false; break;
        case TYPE_NAND: result = inputs.size() >= 2 ? !(inputs[0]->GetValue() && inputs[1]->GetValue()) : false; break;
        case TYPE_NOR: result = inputs.size() >= 2 ? !(inputs[0]->GetValue() || inputs[1]->GetValue()) : false; break;
        default: break;
        }
        for (auto& pin : outputs) pin->SetValue(result);
    }

    virtual std::vector<Pin*> GetPins() override {
        std::vector<Pin*> allPins;
        for (auto& pin : inputs) allPins.push_back(pin.get());
        for (auto& pin : outputs) allPins.push_back(pin.get());
        return allPins;
    }

    virtual wxRect GetBoundingBox() const override { return wxRect(posX - 50, posY - 40, 100, 80); }
    virtual wxString GetName() const override {
        switch (type) {
        case TYPE_AND: return "AND"; case TYPE_OR: return "OR"; case TYPE_NOT: return "NOT";
        case TYPE_XOR: return "XOR"; case TYPE_NAND: return "NAND"; case TYPE_NOR: return "NOR";
        default: return "UnknownGate";
        }
    }
    virtual wxString GetDisplayName() const override { return GetName() + " Gate"; }
    virtual void Serialize(wxString& data) const override { data += wxString::Format("%d,%d,%d", type, posX, posY); }
    virtual void Deserialize(const wxString& data) override {
        wxStringTokenizer tokenizer(data, ",");
        if (tokenizer.CountTokens() >= 3) {
            long typeVal, x, y;
            if (tokenizer.GetNextToken().ToLong(&typeVal) && tokenizer.GetNextToken().ToLong(&x) && tokenizer.GetNextToken().ToLong(&y)) {
                type = static_cast<ElementType>(typeVal);
                SetPosition(static_cast<int>(x), static_cast<int>(y));
                const int PIN_OFFSET = 40;
                inputs.clear(); outputs.clear();
                if (type == TYPE_NOT) {
                    inputs.push_back(std::make_unique<Pin>(x - PIN_OFFSET, y, true, this));
                    outputs.push_back(std::make_unique<Pin>(x + PIN_OFFSET, y, false, this));
                }
                else {
                    inputs.push_back(std::make_unique<Pin>(x - PIN_OFFSET, y - 20, true, this));
                    inputs.push_back(std::make_unique<Pin>(x - PIN_OFFSET, y + 20, true, this));
                    outputs.push_back(std::make_unique<Pin>(x + PIN_OFFSET, y, false, this));
                }
            }
        }
    }
    virtual void GetProperties(wxPropertyGrid* pg) const override {
        pg->Append(new wxStringProperty("Type", "Type", GetDisplayName()));
        pg->Append(new wxIntProperty("X Position", "X", posX));
        pg->Append(new wxIntProperty("Y Position", "Y", posY));
    }
    virtual void SetProperties(wxPropertyGrid* pg) override {
        wxVariant xVar = pg->GetPropertyValue("X");
        wxVariant yVar = pg->GetPropertyValue("Y");
        if (xVar.IsType("long") && yVar.IsType("long")) {
            int newX = static_cast<int>(xVar.GetLong());
            int newY = static_cast<int>(yVar.GetLong());
            SetPosition(newX, newY);
            const int PIN_OFFSET = 40;
            if (type == TYPE_NOT) {
                inputs[0]->SetPosition(newX - PIN_OFFSET, newY);
                outputs[0]->SetPosition(newX + PIN_OFFSET, newY);
            }
            else {
                inputs[0]->SetPosition(newX - PIN_OFFSET, newY - 20);
                inputs[1]->SetPosition(newX - PIN_OFFSET, newY + 20);
                outputs[0]->SetPosition(newX + PIN_OFFSET, newY);
            }
        }
    }

private:
    std::vector<std::unique_ptr<Pin>> inputs;
    std::vector<std::unique_ptr<Pin>> outputs;
};

#endif
