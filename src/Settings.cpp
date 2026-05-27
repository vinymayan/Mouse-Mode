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
    int ButtonExitMouseMode = RE::BSWin32GamepadDevice::Keys::kBack + GAMEPAD_OFFSET;

    const char* SETTINGS_PATH = "Data/SKSE/Plugins/MouseMode_Settings.json";
    const char* LANG_PATH = "Data/SKSE/Plugins/MouseMode_Language.json";
    static std::unordered_map<std::string, std::string> LangMap;

    void LoadLanguage() {
        LangMap.clear();
        std::ifstream file(LANG_PATH, std::ios::binary);
        if (!file.is_open()) {
            SKSE::log::warn("Não foi possível carregar DodgeMod_Language.json. Usando textos padrões.");
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonStr = buffer.str();
        file.close();

        if (jsonStr.size() >= 3 && (unsigned char)jsonStr[0] == 0xEF && (unsigned char)jsonStr[1] == 0xBB && (unsigned char)jsonStr[2] == 0xBF) {
            jsonStr.erase(0, 3);
        }

        rapidjson::Document doc;
        doc.Parse(jsonStr.c_str());

        if (doc.HasParseError()) return;

        if (doc.IsObject()) {
            for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
                if (itr->value.IsObject()) {
                    std::string category = itr->name.GetString();
                    for (auto jtr = itr->value.MemberBegin(); jtr != itr->value.MemberEnd(); ++jtr) {
                        if (jtr->value.IsString()) {
                            LangMap[category + "." + jtr->name.GetString()] = jtr->value.GetString();
                        }
                    }
                }
                else if (itr->value.IsString()) {
                    LangMap[itr->name.GetString()] = itr->value.GetString();
                }
            }
        }
    }

    const char* GetLoc(const std::string& key, const char* defaultVal) {
        auto it = LangMap.find(key);
        if (it != LangMap.end()) return it->second.c_str();
        return defaultVal;
    }
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
        doc.AddMember("ButtonExitMouseMode", ButtonExitMouseMode, allocator);

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
                if (doc.HasMember("ButtonExitMouseMode")) ButtonExitMouseMode = doc["ButtonExitMouseMode"].GetInt();
            }
        }
    }

    void __stdcall Render() {
        bool changed = false;

        ImGui::Text(GetLoc("Menu.Title", "Mouse Mode Settings"));
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored({ 0.4f, 0.8f, 1.0f, 1.0f }, GetLoc("Section.InputManager", "Input Manager Integration"));

        if (!InputManagerAPI::_API) {
            ImGui::TextDisabled(GetLoc("InputManager.NotFound", "[Input Manager not detected]"));
        }
        else {
            size_t actionCount = InputManagerAPI::_API->GetInputCount(0);
            std::string previewValue = GetLoc("InputManager.NoAction", "[No Action Selected]");
            if (MouseModeActionID >= 0 && MouseModeActionID < actionCount) {
                auto info = InputManagerAPI::_API->GetActionInfo(MouseModeActionID);
                previewValue = "[" + std::to_string(MouseModeActionID) + "] " + (info.name ? std::string(info.name) : GetLoc("InputManager.Unnamed", "Unnamed"));
            }

            ImGui::SetNextItemWidth(300);
            // Concatenamos a tradução com a ID interna do ImGui (##ActionSelector) para evitar bugs de foco do combo
            std::string comboLabel = std::string(GetLoc("InputManager.ToggleAction", "Toggle Mouse Mode Action")) + "##ActionSelector";
            if (ImGui::BeginCombo(comboLabel.c_str(), previewValue.c_str())) {

                if (ImGui::Selectable(GetLoc("InputManager.Disabled", "[Disabled]"), MouseModeActionID == -1)) {
                    if (MouseModeActionID != -1) {
                        InputManagerAPI::_API->UpdateListener(0, MouseModeActionID, "Mouse Mode", "Toggle Mouse Mode", false, nullptr, 0, nullptr, 0);
                        MouseModeActionID = -1;
                        changed = true;
                    }
                }

                for (int i = 0; i < actionCount; ++i) {
                    auto info = InputManagerAPI::_API->GetActionInfo(i);
                    std::string itemLabel = "[" + std::to_string(i) + "] " + (info.name ? std::string(info.name) : GetLoc("InputManager.Unnamed", "Unnamed"));

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

        ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, GetLoc("Section.StickConfig", "Stick Configuration"));
        const char* joysticks[] = {
            GetLoc("Stick.Left", "Left Stick"),
            GetLoc("Stick.Right", "Right Stick")
        };
        if (ImGui::Combo(GetLoc("Stick.MouseMovement", "Mouse Movement Stick"), &MouseJoystick, joysticks, 2)) changed = true;
        if (ImGui::Combo(GetLoc("Stick.Directional", "Directional (Menu) Stick"), &DirectionJoystick, joysticks, 2)) changed = true;

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, GetLoc("Section.ButtonsMapping", "Buttons Mapping"));
        ImGui::PushStyleColor(ImGui::ImGuiCol_Text, { 0.9f, 0.6f, 0.2f, 1.0f });
        ImGui::TextWrapped(GetLoc("Buttons.Warning", "Buttons not configured below will continue to operate normally as default game controls during Mouse Mode."));
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // Lambda atualizada para buscar dinamicamente com base na chave e no fallback literal
        auto RenderKeyBind = [&](const char* labelKey, const char* defaultLabel, int& keyID) {
            int index = GetIndexFromID(keyID, gamepadKeyIDs, std::size(gamepadKeyIDs));
            if (SearchableCombo(GetLoc(labelKey, defaultLabel), &index, gamepadKeyNames, std::size(gamepadKeyNames))) {
                keyID = gamepadKeyIDs[index];
                changed = true;
            }
            };

        RenderKeyBind("Button.Exit", "Exit Mouse Mode", ButtonExitMouseMode);
        RenderKeyBind("Button.LeftClick", "Left Click", ButtonLeftClick);
        RenderKeyBind("Button.RightClick", "Right Click", ButtonRightClick);
        RenderKeyBind("Button.ScrollClick", "Middle/Scroll Click", ButtonScrollClick);
        RenderKeyBind("Button.ScrollUp", "Scroll Up", ButtonScrollUp);
        RenderKeyBind("Button.ScrollDown", "Scroll Down", ButtonScrollDown);
        RenderKeyBind("Button.Confirm", "Confirm Action (Accept)", ButtonConfirm);
        RenderKeyBind("Button.Cancel", "Cancel Action (Cancel)", ButtonCancel);

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
        LoadLanguage();
        Settings::InitializeInput();
        if (SKSEMenuFramework::IsInstalled()) {
            SKSEMenuFramework::SetSection("Mouse Mode");
            SKSEMenuFramework::AddSectionItem("Settings", Render);
        }
    }
}