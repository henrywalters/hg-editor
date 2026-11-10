//
// Created by henry on 10/3/24.
//
#include "hge/components/componentExplorer.h"
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <hagame/math/aliases.h>
#include <hagame/graphics/color.h>
#include "imgui.h"
#include "../../thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "hge/widgets/textureField.h"
#include <hagame/common/components/tilemap.h>

#include "hge/components/dragTarget.h"
#include "hge/core/events.h"

using namespace hge;

static std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
    return value;
}

// Component names in a category, alphabetically
static std::vector<std::string> sortedComponents(std::string const& category) {
    std::vector<std::string> names;
    for (const auto& component : hg::ComponentFactory::GetCategory(category)) {
        names.push_back(component.name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::optional<std::string> hge::componentExplorer() {

    static std::string search;

    std::optional<std::string> out = std::nullopt;

    if (ImGui::BeginPopup(COMPONENT_EXPLORER.c_str())) {

        if (ImGui::IsWindowAppearing()) {
            search.clear();
            ImGui::SetKeyboardFocusHere();
        }

        bool submitted = ImGui::InputTextWithHint("##search", "Search", &search, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Separator();

        auto categories = hg::ComponentFactory::GetCategories();
        std::sort(categories.begin(), categories.end());

        if (search.empty()) {
            // Browse: one submenu per category
            for (const auto& category : categories) {
                if (ImGui::BeginMenu(category.c_str())) {
                    for (const auto& name : sortedComponents(category)) {
                        if (ImGui::MenuItem(name.c_str())) {
                            out = name;
                        }
                    }
                    ImGui::EndMenu();
                }
            }
        } else {
            // Search: every matching component, flat. Enter picks the first match
            auto query = toLower(search);
            bool any = false;

            for (const auto& category : categories) {
                for (const auto& name : sortedComponents(category)) {
                    if (toLower(name).find(query) == std::string::npos) {
                        continue;
                    }
                    if (submitted && !any) {
                        out = name;
                    }
                    any = true;
                    if (ImGui::Selectable(name.c_str())) {
                        out = name;
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled("%s", category.c_str());
                }
            }

            if (!any) {
                ImGui::TextDisabled("No components match");
            }
        }

        if (out.has_value()) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    return out;
}

#define PRIMITIVE_FIELD(Type, Func) if (field.type == #Type) {                  \
    Type initial = std::get<Type>(value);                                  \
    ImGui::Func(field.type.c_str(), &initial);                                 \
    if (std::get<Type>(details.getter(component)) != initial) {               \
        details.setter(component, initial);                                       \
        return true;                                                            \
    } else {                                                                     \
        return false;                                                           \
    }                                                                           \
}

#define NUMERIC_FIELD(Type, Func) if (field.type == #Type) {                  \
    Type initial = std::get<Type>(value);                                  \
    if (details.number.has_value()) {                                                                          \
        ImGui::Func(field.type.c_str(), &initial, details.number.value().step, details.number.value().min, details.number.value().max); \
    } else {                                                                    \
        ImGui::Func(field.type.c_str(), &initial);                                 \
    }                                                                          \
    if (std::get<Type>(details.getter(component)) != initial) {               \
        details.setter(component, initial);                                       \
        return true;                                                            \
    } else {                                                                     \
        return false;                                                           \
    }                                                                           \
}

#define HG_VECTOR_FIELD(Type, Func) if (field.type == #Type) {                  \
    Type initial = std::get<Type>(value);                                  \
    ImGui::Func(field.type.c_str(), initial.vector.data());                                 \
    if (std::get<Type>(details.getter(component)) != initial) {               \
        details.setter(component, initial);                                       \
        return true;                                                            \
    }\
    return false;                                                            \
}


// An asset field set by dropping a file with the given extension on it. The asset's path is the file's path without
// its extension. Textures pass a preview to show the image in the drop target. With a pickFolder (under the asset
// directory) it can also be picked from a dropdown of the files there
template <typename T>
bool assetField(hg::Component* component, hg::ComponentFactory::PrimitiveDetails const& details, hg::utils::variant const& current,
                std::string const& extension, std::string const& emptyLabel, std::optional<ImTextureRef> preview = std::nullopt,
                std::string const& pickFolder = "") {
    auto initial = std::get<T>(current);
    auto value = initial;
    auto label = value.path().empty() ? emptyLabel : value.path();

    std::vector<ExtensionHandler> handlers = {
        {extension, [&](std::string const& path) {
            value.path(hg::utils::s_replace(path, "." + extension, ""));
        }}
    };

    if (preview.has_value()) {
        dragTarget(label, preview.value(), 60, handlers);
    } else {
        dragTarget(label, 40, handlers);
    }

    // A dropdown of every file with this extension under the folder, for when dragging isn't handy
    if (!pickFolder.empty()) {
        std::optional<std::string> picked;
        if (ImGui::BeginCombo("##pick", value.path().empty() ? "Pick..." : value.path().c_str())) {
            auto dir = hg::ASSET_DIR + pickFolder;
            std::vector<std::string> paths;
            if (hg::utils::f_exists(dir)) {
                for (const auto& file : hg::utils::d_listFiles(dir, true)) {
                    if (hg::utils::s_endsWith(file, "." + extension)) {
                        paths.push_back(hg::utils::s_replace(hg::utils::s_replace(file, hg::ASSET_DIR, ""), "." + extension, ""));
                    }
                }
            }
            std::sort(paths.begin(), paths.end());
            for (const auto& path : paths) {
                if (ImGui::Selectable(path.c_str(), path == value.path())) {
                    picked = path;
                }
            }
            if (paths.empty()) {
                ImGui::TextDisabled("No .%s files in %s", extension.c_str(), pickFolder.c_str());
            }
            ImGui::EndCombo();
        }
        // Applied once the combo is closed: loading the asset can throw, which mid-combo would leave ImGui's stack
        // unbalanced
        if (picked) {
            value.path(*picked);
        }
    }

    if (initial != value) {
        details.setter(component, value);
        return true;
    }
    return false;
}

struct StringFieldOptions {
    StringOptions options;
    StringChosen onChosen;
};

// Component name -> field name -> its dropdown options
static std::unordered_map<std::string, std::unordered_map<std::string, StringFieldOptions>>& stringFieldOptions() {
    static std::unordered_map<std::string, std::unordered_map<std::string, StringFieldOptions>> options;
    return options;
}

void hge::setStringFieldOptions(std::string const& component, std::string const& field, StringOptions options,
                                StringChosen onChosen) {
    stringFieldOptions()[component][field] = StringFieldOptions{options, onChosen};
}

// The options registered for this field, if any
static StringFieldOptions const* findStringOptions(hg::Component* component, std::string const& field) {
    auto& all = stringFieldOptions();
    auto byComponent = all.find(component->className());
    if (byComponent == all.end()) {
        return nullptr;
    }
    auto options = byComponent->second.find(field);
    return options == byComponent->second.end() ? nullptr : &options->second;
}

bool hge::editComponentField(hg::Component* component, hg::ComponentFactory::ComponentField field) {

    return std::visit(hg::utils::overloaded{
    [&](hg::ComponentFactory::PrimitiveDetails const& details) {
        auto value = details.getter(component);

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
            if (auto options = findStringOptions(component, field.field)) {
                std::string current = std::get<std::string>(value);
                std::string chosen = current;
                if (ImGui::BeginCombo(field.type.c_str(), current.empty() ? "None" : current.c_str())) {
                    if (ImGui::Selectable("None", current.empty())) {
                        chosen = "";
                    }
                    for (const auto& option : options->options()) {
                        bool selected = option == current;
                        if (ImGui::Selectable(option.c_str(), selected)) {
                            chosen = option;
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                if (chosen != current) {
                    details.setter(component, chosen);
                    if (options->onChosen) {
                        options->onChosen(component, chosen);
                    }
                    return true;
                }
                return false;
            }

            std::string initial = std::get<std::string>(value);
            ImGui::InputText(field.type.c_str(), &initial);
            if (std::get<std::string>(value) != std::string(initial.data())) {
                details.setter(component, std::string(initial.data()));
                return true;
            }
            return false;
        }

        if (field.type == "hg::graphics::Color") {
            auto initial = std::get<hg::graphics::Color>(value);
            //ImGui::ColorPicker4(field.type.c_str(), initial.vector);
            ImGui::ColorEdit4(field.type.c_str(), initial.vector.data(), ImGuiColorEditFlags_Float);
            if (std::get<hg::graphics::Color>(value) != initial) {
                details.setter(component, initial);
                return true;
            }
            return false;
        }

        if (field.type == "hg::graphics::TextureAsset") {
            auto texture = std::get<hg::graphics::TextureAsset>(value);
            ImTextureRef preview;
            if (texture.asset()) {
                preview = texture.asset()->id;
            }
            return assetField<hg::graphics::TextureAsset>(component, details, value, "png", "Drag Texture to Select", preview);
        }

        if (field.type == "hg::graphics::SpriteSheetAsset") {
            return assetField<hg::graphics::SpriteSheetAsset>(component, details, value, "hgss", "Load Sprite Sheet");
        }

        if (field.type == "hg::graphics::RigAtlasAsset") {
            return assetField<hg::graphics::RigAtlasAsset>(component, details, value, "hgra", "Load Rig Atlas");
        }

        if (field.type == "hg::graphics::SkeletonAsset") {
            return assetField<hg::graphics::SkeletonAsset>(component, details, value, "hg_skel", "Load Skeleton");
        }

        if (field.type == "hg::graphics::RigAnimationSetAsset") {
            return assetField<hg::graphics::RigAnimationSetAsset>(component, details, value, "hg_anims", "Load Rig Animations");
        }

        if (field.type == "hg::graphics::FontAsset") {
            return assetField<hg::graphics::FontAsset>(component, details, value, "ttf", "Load Font");
        }

        if (field.type == "hg::PrefabAsset") {
            return assetField<hg::PrefabAsset>(component, details, value, "hg_pfb", "Load Prefab");
        }

        if (field.type == "hg::audio::StreamAsset") {
            return assetField<hg::audio::StreamAsset>(component, details, value, "wav", "Drag .wav to Select", std::nullopt, "audio");
        }

        if (field.type == "hg::Rect") {
            auto initial = std::get<hg::Rect>(value);
            auto value = initial;
            ImGui::DragFloat2("pos", value.pos.vector.data());
            ImGui::DragFloat2("size", value.size.vector.data());
            if (initial != value) {
                details.setter(component, value);
                return true;
            }
            return false;
        }

        ImGui::Text("Unsupported Field Type: %s", field.type.c_str());
        return false;
    },
    [&](hg::ComponentFactory::JsonDetails const& details) {
        return false;
    }
    }, field.details);


}
