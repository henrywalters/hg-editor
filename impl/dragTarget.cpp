//
// Created by henry on 8/24/26.
//

#include <hge/components/dragTarget.h>

#include "imgui.h"

void hge::dragTarget(std::string label, ImTextureRef image, int height, std::vector<hge::ExtensionHandler> handlers) {
    ImVec2 boxSize(ImGui::GetContentRegionAvail().x, height);
    ImGui::InvisibleButton("##dropzone", boxSize);
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();

    bool hovered = ImGui::IsItemHovered();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(min, max, IM_COL32(50, 50, 50, 255), 4.0f);
    drawList->AddRect(min, max, hovered ? IM_COL32(100, 200, 255, 255) : IM_COL32(120, 120, 120, 255), 4.0f);

    const char* labelText = label.c_str();
    ImVec2 textSize = ImGui::CalcTextSize(labelText);

    bool hasImage = image.GetTexID() != 0;
    float imageSize = height * 0.7f;
    float spacing = 8.0f;

    float groupWidth = hasImage ? (imageSize + spacing + textSize.x) : textSize.x;
    float groupStartX = min.x + (boxSize.x - groupWidth) * 0.5f;
    float centerY = min.y + boxSize.y * 0.5f;

    float cursorX = groupStartX;

    if (hasImage) {
        ImVec2 imgMin(cursorX, centerY - imageSize * 0.5f);
        ImVec2 imgMax(cursorX + imageSize, centerY + imageSize * 0.5f);
        drawList->AddImage(image, imgMin, imgMax);
        cursorX += imageSize + spacing;
    }

    ImVec2 textPos(cursorX, centerY - textSize.y * 0.5f);
    drawList->AddText(textPos, IM_COL32(200, 200, 200, 255), labelText);

    if (ImGui::BeginDragDropTarget()) {
        for (const auto& handler : handlers) {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(handler.extension.c_str())) {
                handler.handler(std::string(static_cast<char*>(payload->Data)));
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void hge::dragTarget(std::string label, int height, std::vector<hge::ExtensionHandler> handlers) {
    ImVec2 boxSize(ImGui::GetContentRegionAvail().x, height);
    ImGui::InvisibleButton("##dropzone", boxSize); // gives us an item + bounding rect
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();

    bool hovered = ImGui::IsItemHovered();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(min, max, IM_COL32(50, 50, 50, 255), 4.0f);
    drawList->AddRect(min, max, hovered ? IM_COL32(100, 200, 255, 255) : IM_COL32(120, 120, 120, 255), 4.0f);

    // Center the text
    const char* labelText = label.c_str();
    ImVec2 textSize = ImGui::CalcTextSize(labelText);
    ImVec2 textPos(min.x + (boxSize.x - textSize.x) * 0.5f,
                   min.y + (boxSize.y - textSize.y) * 0.5f);
    drawList->AddText(textPos, IM_COL32(200, 200, 200, 255), labelText);

    if (ImGui::BeginDragDropTarget()) {

        for (const auto& handler : handlers) {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(handler.extension.c_str())) {
                handler.handler(std::string(static_cast<char*>(payload->Data)));
            }
        }

        ImGui::EndDragDropTarget();
    }
}
