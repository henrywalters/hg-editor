//
// Created by henry on 10/3/24.
//
#include "hge/components/componentExplorer.h"
#include <hagame/math/aliases.h>
#include <hagame/graphics/color.h>
#include "imgui.h"
#include "../../thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "hge/widgets/textureField.h"

using namespace hge;

std::optional<std::string> hge::componentExplorer() {

    std::optional<std::string> out = std::nullopt;

    if (ImGui::BeginPopup(COMPONENT_EXPLORER.c_str())) {
        for (const auto& category : hg::ComponentFactory::GetCategories()) {
            ImGui::SeparatorText(category.c_str());
            for (const auto& component : hg::ComponentFactory::GetCategory(category)) {
                if (ImGui::Selectable(component.name.c_str())) {
                    out = component.name;
                }
            }
        }
        ImGui::EndPopup();
    }

    if (out.has_value()) {
        ImGui::CloseCurrentPopup();
    }

    return out;
}

#define PRIMITIVE_FIELD(Type, Func) if (field.type == #Type) {                  \
    Type initial = std::get<Type>(value);                                  \
    ImGui::Func(field.type.c_str(), &initial);                                 \
    if (std::get<Type>(field.getter(component)) != initial) {               \
        field.setter(component, initial);                                       \
        return true;                                                            \
    } else {                                                                     \
        return false;                                                           \
    }                                                                           \
}

#define NUMERIC_FIELD(Type, Func) if (field.type == #Type) {                  \
    Type initial = std::get<Type>(value);                                  \
    if (field.number.has_value()) {                                                                          \
        ImGui::Func(field.type.c_str(), &initial, field.number.value().step, field.number.value().min, field.number.value().max); \
    } else {                                                                    \
        ImGui::Func(field.type.c_str(), &initial);                                 \
    }                                                                          \
    if (std::get<Type>(field.getter(component)) != initial) {               \
        field.setter(component, initial);                                       \
        return true;                                                            \
    } else {                                                                     \
        return false;                                                           \
    }                                                                           \
}

#define HG_VECTOR_FIELD(Type, Func) if (field.type == #Type) {                  \
    Type initial = std::get<Type>(value);                                  \
    ImGui::Func(field.type.c_str(), initial.vector);                                 \
    if (std::get<Type>(field.getter(component)) != initial) {               \
        field.setter(component, initial);                                       \
        return true;                                                            \
    }\
    return false;                                                            \
}


bool hge::editComponentField(hg::Component* component, hg::ComponentFactory::ComponentField field) {
    auto value = field.getter(component);

    PRIMITIVE_FIELD(bool, Checkbox)
    NUMERIC_FIELD(int, DragInt)
    NUMERIC_FIELD(float, DragFloat)
    PRIMITIVE_FIELD(double, InputDouble)

    HG_VECTOR_FIELD(hg::Vec2, DragFloat2)
    HG_VECTOR_FIELD(hg::Vec3, DragFloat3)
    HG_VECTOR_FIELD(hg::Vec4, DragFloat4)

    HG_VECTOR_FIELD(hg::Vec2i, DragInt2)
    HG_VECTOR_FIELD(hg::Vec3i, DragInt3)
    HG_VECTOR_FIELD(hg::Vec4i, DragInt4)

    if (field.type == "std::string") {
        std::string initial = std::get<std::string>(value);
        ImGui::InputText(field.type.c_str(), &initial);
        if (std::get<std::string>(value) != std::string(initial.data())) {
            field.setter(component, std::string(initial.data()));
            return true;
        }
        return false;
    }

    if (field.type == "hg::graphics::Color") {
        auto initial = std::get<hg::graphics::Color>(value);
        //ImGui::ColorPicker4(field.type.c_str(), initial.vector);
        ImGui::ColorEdit4(field.type.c_str(), initial.vector, ImGuiColorEditFlags_Float);
        if (std::get<hg::graphics::Color>(value) != initial) {
            field.setter(component, initial);
            return true;
        }
        return false;
    }

    if (field.type == "hg::graphics::TextureAsset") {
        auto initial = std::get<hg::graphics::TextureAsset>(value);
        auto value = initial;
        textureField(field.field, value);
        if (initial != value) {
            field.setter(component, value);
            return true;
        }
        return false;
    }

    if (field.type == "hg::Rect") {
        auto initial = std::get<hg::Rect>(value);
        auto value = initial;
        ImGui::DragFloat2("pos", value.pos.vector);
        ImGui::DragFloat2("size", value.size.vector);
        if (initial != value) {
            field.setter(component, value);
            return true;
        }
        return false;
    }

    ImGui::Text("Unsupported Field Type: %s", field.type.c_str());
    return false;
}
