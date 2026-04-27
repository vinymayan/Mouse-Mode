#include "Settings.h"

namespace ImGui = ImGuiMCP;

namespace Settings {
    int MouseModeActionID = -1;
    bool IsMouseModeActive = false;
    bool HasInitializedInput = false;

    int MouseJoystick = 1;     // Padrão: Direito para o mouse
    int DirectionJoystick = 0; // Padrão: Esquerdo para a direção

    int ButtonScrollUp = RE::BSWin32GamepadDevice::Keys::kUp + GAMEPAD_OFFSET;
    int ButtonScrollDown = RE::BSWin32GamepadDevice::Keys::kDown + GAMEPAD_OFFSET;
    int ButtonRightClick = RE::BSWin32GamepadDevice::Keys::kRightTrigger + GAMEPAD_OFFSET;
    int ButtonLeftClick = RE::BSWin32GamepadDevice::Keys::kLeftTrigger + GAMEPAD_OFFSET;
    int ButtonScrollClick = RE::BSWin32GamepadDevice::Keys::kRightThumb + GAMEPAD_OFFSET;
    int ButtonConfirm = RE::BSWin32GamepadDevice::Keys::kA + GAMEPAD_OFFSET;
    int ButtonCancel = RE::BSWin32GamepadDevice::Keys::kB + GAMEPAD_OFFSET;

    const char* SETTINGS_PATH = "Data/SKSE/Plugins/MouseMode_Settings.json";

    inline std::string ToLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    inline int GetIndexFromID(int id, const int* idArray, int arraySize) {
        for (int i = 0; i < arraySize; i++) {
            if (idArray[i] == id) return i;
        }
        return 0;
    }

    inline bool SearchableCombo(const char* label, int* current_item, const char* const items[], int items_count) {
        bool changed = false;
        const char* preview_value = (*current_item >= 0 && *current_item < items_count) ? items[*current_item] : "None";

        if (ImGui::BeginCombo(label, preview_value)) {
            static char searchBuf[128] = "";

            if (ImGui::IsWindowAppearing()) {
                searchBuf[0] = '\0';
                ImGui::SetKeyboardFocusHere();
            }

            ImGui::InputText("Filter...##Search", searchBuf, sizeof(searchBuf));
            ImGui::Separator();

            std::string searchLower = ToLower(searchBuf);

            for (int i = 0; i < items_count; i++) {
                if (searchLower.empty() || ToLower(items[i]).find(searchLower) != std::string::npos) {
                    bool is_selected = (*current_item == i);
                    if (ImGui::Selectable(items[i], is_selected)) {
                        *current_item = i;
                        changed = true;
                    }
                    if (is_selected && ImGui::IsWindowAppearing()) {
                        ImGui::SetScrollHereY();
                    }
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    void Save() {
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();
        doc.AddMember("HasInitializedInput", HasInitializedInput, allocator);
        doc.AddMember("MouseModeActionID", MouseModeActionID, allocator);
        doc.AddMember("MouseJoystick", MouseJoystick, allocator);
        doc.AddMember("DirectionJoystick", DirectionJoystick, allocator);
        doc.AddMember("ButtonScrollUp", ButtonScrollUp, allocator);
        doc.AddMember("ButtonScrollDown", ButtonScrollDown, allocator);
        doc.AddMember("ButtonRightClick", ButtonRightClick, allocator);
        doc.AddMember("ButtonLeftClick", ButtonLeftClick, allocator);
        doc.AddMember("ButtonScrollClick", ButtonScrollClick, allocator);
        doc.AddMember("ButtonConfirm", ButtonConfirm, allocator);
        doc.AddMember("ButtonCancel", ButtonCancel, allocator);

        FILE* fp = nullptr;
        fopen_s(&fp, SETTINGS_PATH, "wb");
        if (fp) {
            char writeBuffer[65536];
            rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
            rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
            doc.Accept(writer);
            fclose(fp);
        }
    }

    void Load() {
        FILE* fp = nullptr;
        fopen_s(&fp, SETTINGS_PATH, "rb");
        if (fp) {
            char readBuffer[65536];
            rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
            rapidjson::Document doc;
            doc.ParseStream(is);
            fclose(fp);

            if (doc.IsObject()) {
                if (doc.HasMember("HasInitializedInput")) HasInitializedInput = doc["HasInitializedInput"].GetBool();
                if (doc.HasMember("MouseModeActionID")) MouseModeActionID = doc["MouseModeActionID"].GetInt();
                if (doc.HasMember("MouseJoystick")) MouseJoystick = doc["MouseJoystick"].GetInt();
                if (doc.HasMember("DirectionJoystick")) DirectionJoystick = doc["DirectionJoystick"].GetInt();
                if (doc.HasMember("ButtonScrollUp")) ButtonScrollUp = doc["ButtonScrollUp"].GetInt();
                if (doc.HasMember("ButtonScrollDown")) ButtonScrollDown = doc["ButtonScrollDown"].GetInt();
                if (doc.HasMember("ButtonRightClick")) ButtonRightClick = doc["ButtonRightClick"].GetInt();
                if (doc.HasMember("ButtonLeftClick")) ButtonLeftClick = doc["ButtonLeftClick"].GetInt();
                if (doc.HasMember("ButtonScrollClick")) ButtonScrollClick = doc["ButtonScrollClick"].GetInt();
                if (doc.HasMember("ButtonConfirm")) ButtonConfirm = doc["ButtonConfirm"].GetInt();
                if (doc.HasMember("ButtonCancel")) ButtonCancel = doc["ButtonCancel"].GetInt();
            }
        }
    }

    void __stdcall Render() {
        bool changed = false;

        ImGui::Text("Mouse Mode Settings");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored({ 0.4f, 0.8f, 1.0f, 1.0f }, "Input Manager Integration");

        if (!InputManagerAPI::_API) {
            ImGui::TextDisabled("[Input Manager not detected]");
        }
        else {
            size_t actionCount = InputManagerAPI::_API->GetInputCount(0);
            std::string previewValue = "[No Action Selected]";
            if (MouseModeActionID >= 0 && MouseModeActionID < actionCount) {
                auto info = InputManagerAPI::_API->GetActionInfo(MouseModeActionID);
                previewValue = "[" + std::to_string(MouseModeActionID) + "] " + (info.name ? std::string(info.name) : "Unnamed");
            }

            ImGui::SetNextItemWidth(300);
            if (ImGui::BeginCombo("Toggle Mouse Mode Action##ActionSelector", previewValue.c_str())) {

                if (ImGui::Selectable("[Disabled]", MouseModeActionID == -1)) {
                    if (MouseModeActionID != -1) {
                        InputManagerAPI::_API->UpdateListener(0, MouseModeActionID, "Mouse Mode", "Toggle Mouse Mode", false, nullptr, 0, nullptr, 0);
                        MouseModeActionID = -1;
                        changed = true;
                    }
                }

                for (int i = 0; i < actionCount; ++i) {
                    auto info = InputManagerAPI::_API->GetActionInfo(i);
                    std::string itemLabel = "[" + std::to_string(i) + "] " + (info.name ? std::string(info.name) : "Unnamed");

                    if (ImGui::Selectable(itemLabel.c_str(), MouseModeActionID == i)) {
                        if (MouseModeActionID != -1 && MouseModeActionID != i) {
                            InputManagerAPI::_API->UpdateListener(0, MouseModeActionID, "Mouse Mode", "Toggle Mouse Mode", false, nullptr, 0, nullptr, 0);
                        }

                        MouseModeActionID = i;

                        int validMain[] = { 1, 4 };
                        InputManagerAPI::_API->UpdateListener(0, MouseModeActionID, "Mouse Mode", "Toggle Mouse Mode", true, validMain, 2, nullptr, 0);

                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, "Stick Configuration");
        const char* joysticks[] = { "Left Stick", "Right Stick" };
        if (ImGui::Combo("Mouse Movement Stick", &MouseJoystick, joysticks, 2)) changed = true;
        if (ImGui::Combo("Directional (Menu) Stick", &DirectionJoystick, joysticks, 2)) changed = true;

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, "Buttons Mapping");
        auto RenderKeyBind = [&](const char* label, int& keyID) {
            int index = GetIndexFromID(keyID, gamepadKeyIDs, std::size(gamepadKeyIDs));
            if (SearchableCombo(label, &index, gamepadKeyNames, std::size(gamepadKeyNames))) {
                keyID = gamepadKeyIDs[index];
                changed = true;
            }
            };

        RenderKeyBind("Left Click", ButtonLeftClick);
        RenderKeyBind("Right Click", ButtonRightClick);
        RenderKeyBind("Middle/Scroll Click", ButtonScrollClick);
        RenderKeyBind("Scroll Up", ButtonScrollUp);
        RenderKeyBind("Scroll Down", ButtonScrollDown);
        RenderKeyBind("Confirm Action (Accept)", ButtonConfirm);
        RenderKeyBind("Cancel Action (Cancel)", ButtonCancel);

        if (changed) {
            Save();
        }
    }

    void InitializeInput() {
        // Se já inicializamos uma vez no passado, não fazemos nada
        if (HasInitializedInput || !InputManagerAPI::_API) return;

        logger::info("Primeira execução detectada. Configurando padrões...");

        // 1. Tentar encontrar ou criar o input "Mouse Mode"
        int foundID = -1;
        size_t actionCount = InputManagerAPI::_API->GetInputCount(0);
        for (int i = 0; i < actionCount; i++) {
            const char* name = InputManagerAPI::_API->GetInputName(0, i);
            if (name && std::string(name) == "Mouse Mode") {
                foundID = i;
                break;
            }
        }

        if (foundID == -1) {
            foundID = static_cast<int>(InputManagerAPI::_API->CreateInput(0, "Mouse Mode"));

            // Configura RB Hold como padrão no Input Manager
            InputManagerAPI::ActionInfo info = InputManagerAPI::_API->GetActionInfo(foundID);
            info.gamepadMainKey = RE::BSWin32GamepadDevice::Keys::kRightShoulder + 266;
            info.gamepadMainAction = 2; // Hold
            info.holdDuration = 0.5f;
            info.isValid = true;
            InputManagerAPI::_API->UpdateActionMapping(foundID, info);
        }

        // 2. Atualizar dados e salvar para que isso não ocorra de novo
        MouseModeActionID = foundID;
        HasInitializedInput = true;

        Save();
        logger::info("Configuração inicial concluída e salva no JSON.");
    }

    void RegisterMenu() {
        Load();
        Settings::InitializeInput();
        if (SKSEMenuFramework::IsInstalled()) {
            SKSEMenuFramework::SetSection("Mouse Mode");
            SKSEMenuFramework::AddSectionItem("Settings", Render);
        }
    }
}