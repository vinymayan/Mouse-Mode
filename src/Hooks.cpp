#include "Hooks.h"
#include "InputEventHandler.h"
#include "Settings.h"
#include <unordered_map>
#include <mutex>
#include <unordered_set>
#include <cmath> 

namespace ActionManager {
    void ProcessEvent(const std::string_view& eventName, int actionID) {
        if (eventName == "InputManager_ActionTriggered") {
            if (actionID == Settings::MouseModeActionID && actionID != -1) {
                Settings::IsMouseModeActive = !Settings::IsMouseModeActive;
				std::string msg = std::format("Mouse Mode {}", Settings::IsMouseModeActive ? "ON" : "OFF");
				RE::SendHUDMessage::ShowHUDMessage(msg.c_str());
                //SKSE::log::info("Mouse Mode Toggled: {}", Settings::IsMouseModeActive ? "ON" : "OFF");
            }
        }
        if (eventName == "MouseMode_Trigger") {
            if (actionID == 0) {
                Settings::IsMouseModeActive = false;
            }
            else if (actionID == 1) {
                Settings::IsMouseModeActive = true;
            }
            else {
                return; // Ignora se o actionID não for 0 nem 1
            }

            std::string msg = std::format("Mouse Mode {}", Settings::IsMouseModeActive ? "ON" : "OFF");
            RE::SendHUDMessage::ShowHUDMessage(msg.c_str());
                //SKSE::log::info("Mouse Mode Toggled: {}", Settings::IsMouseModeActive ? "ON" : "OFF");
        }
    }
}

class TweenInputListener : public RE::BSTEventSink<SKSE::ModCallbackEvent> {
public:
    static TweenInputListener* GetSingleton() {
        static TweenInputListener singleton;
        return &singleton;
    }

    static void Register() {
        auto eventSource = SKSE::GetModCallbackEventSource();
        if (eventSource) {
            eventSource->AddEventSink(GetSingleton());
        }
    }

    RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>*) override {
        if (!a_event) return RE::BSEventNotifyControl::kContinue;

        std::string_view eventName = a_event->eventName.c_str();
        int actionID = static_cast<int>(a_event->numArg);

        ActionManager::ProcessEvent(eventName, actionID);

        return RE::BSEventNotifyControl::kContinue;
    }
};

struct ProcessInputQueueHook {
    static void thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent* const* a_event) {

        auto processedEventList = InputEventHandler::Process(const_cast<RE::InputEvent**>(a_event));

        RE::InputEvent* head = processedEventList ? *processedEventList : nullptr;
        RE::InputEvent* prev = nullptr;
        RE::InputEvent* curr = head;

        // Verifica se algum menu está aberto baseando-se no contextPriorityStack
        bool inMenu = false;
        if (auto controlMap = RE::ControlMap::GetSingleton()) {
            auto& contextStack = controlMap->GetRuntimeData().contextPriorityStack;
            // Se a pilha não for vazia e o item no topo não for kGameplay
            if (!contextStack.empty() && contextStack.back() != RE::UserEvents::INPUT_CONTEXT_ID::kGameplay) {
                inMenu = true;
            }
        }

        auto userEvents = RE::UserEvents::GetSingleton();

        while (curr) {
            RE::InputEvent* nextEvent = curr->next;
            bool keepEvent = true;

            if (curr->GetDevice() == RE::INPUT_DEVICE::kGamepad) {

                if (Settings::IsMouseModeActive) {
                    keepEvent = false;

                    if (curr->GetEventType() == RE::INPUT_EVENT_TYPE::kThumbstick) {
                        auto thumbEvent = curr->AsThumbstickEvent();

                        bool isMouseStick = (Settings::MouseJoystick == 0 && thumbEvent->IsLeft()) ||
                            (Settings::MouseJoystick == 1 && !thumbEvent->IsLeft());

                        bool isDirStick = (Settings::DirectionJoystick == 0 && thumbEvent->IsLeft()) ||
                            (Settings::DirectionJoystick == 1 && !thumbEvent->IsLeft());

                        if (thumbEvent) {
                            float x = thumbEvent->xValue;
                            float y = thumbEvent->yValue;
                            const float deadzone = 0.15f;

                            if (std::abs(x) > deadzone || std::abs(y) > deadzone) {
                                // 1. Função de Mouse
                                if (isMouseStick) {
                                    const float speedMultiplier = 12.0f;
                                    LONG deltaX = static_cast<LONG>(x * speedMultiplier);
                                    LONG deltaY = static_cast<LONG>(-y * speedMultiplier);

                                    INPUT input = { 0 };
                                    input.type = INPUT_MOUSE;
                                    input.mi.dx = deltaX;
                                    input.mi.dy = deltaY;
                                    input.mi.dwFlags = MOUSEEVENTF_MOVE;

                                    SendInput(1, &input, sizeof(INPUT));
                                }

                                // 2. Função de Direção (Menus ou Movimento)
                                if (isDirStick) {
                                    // Acessa o IDEvent para modificar o userEvent seguramente
                                    if (auto idEvent = thumbEvent->AsIDEvent()) {
                                        if (std::abs(x) > std::abs(y)) {
                                            if (x > 0.0f) idEvent->userEvent = inMenu ? userEvents->right : userEvents->strafeRight;
                                            else          idEvent->userEvent = inMenu ? userEvents->left : userEvents->strafeLeft;
                                        }
                                        else {
                                            if (y > 0.0f) idEvent->userEvent = inMenu ? userEvents->up : userEvents->forward;
                                            else          idEvent->userEvent = inMenu ? userEvents->down : userEvents->back;
                                        }
                                    }
                                    // Se o stick tem a função de direção, nós permitimos o evento passar 
                                    // para o Skyrim computar o UserEvent que acabamos de alterar.
                                    keepEvent = true;
                                }
                            }
                            else {
                                // Se estiver na deadzone e for o analógico direcional, precisa passar
                                // o evento pro Skyrim perceber que soltou o botão (resetar a magnitude).
                                if (isDirStick) {
                                    keepEvent = true;
                                }
                            }

                            if (!isMouseStick && !isDirStick) {
                                keepEvent = true;
                            }
                        }
                    }
                    else if (curr->GetEventType() == RE::INPUT_EVENT_TYPE::kButton) {
                        auto btnEvent = curr->AsButtonEvent();
                        if (btnEvent) {
                            uint32_t keyID = btnEvent->GetIDCode() + GAMEPAD_OFFSET;

                            bool isDown = btnEvent->IsDown();
                            bool isUp = btnEvent->IsUp();
                            bool isHeld = btnEvent->IsHeld();

                            // 1. CHECAGEM PRIORITÁRIA: Botão de Saída de Emergência do Mouse Mode
                            if (keyID == Settings::ButtonExitMouseMode) {
                                if (isDown) {
                                    Settings::IsMouseModeActive = false;
                                    std::string msg = "Mouse Mode OFF";
                                    RE::SendHUDMessage::ShowHUDMessage(msg.c_str());
                                }
                                keepEvent = false; // Consome o input para sair de forma limpa sem disparar ações nativas
                            }
                            else {
                                INPUT input = { 0 };
                                bool shouldSendMouse = false;
                                bool wasHandledInternally = false;
                                bool isMapped = false; // Identifica se pertence ao nosso layout

                                input.type = INPUT_MOUSE;

                                if (keyID == Settings::ButtonLeftClick) {
                                    isMapped = true;
                                    if (isDown) input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                                    else if (isUp) input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                                    shouldSendMouse = (isDown || isUp);
                                }
                                else if (keyID == Settings::ButtonRightClick) {
                                    isMapped = true;
                                    if (isDown) input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
                                    else if (isUp) input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
                                    shouldSendMouse = (isDown || isUp);
                                }
                                else if (keyID == Settings::ButtonScrollClick) {
                                    isMapped = true;
                                    if (isDown) input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
                                    else if (isUp) input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
                                    shouldSendMouse = (isDown || isUp);
                                }
                                else if (keyID == Settings::ButtonScrollUp) {
                                    isMapped = true;
                                    if (isDown || isHeld) {
                                        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
                                        input.mi.mouseData = WHEEL_DELTA;
                                        shouldSendMouse = true;
                                    }
                                }
                                else if (keyID == Settings::ButtonScrollDown) {
                                    isMapped = true;
                                    if (isDown || isHeld) {
                                        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
                                        input.mi.mouseData = -WHEEL_DELTA;
                                        shouldSendMouse = true;
                                    }
                                }
                                else if (keyID == Settings::ButtonConfirm) {
                                    isMapped = true;
                                    if (inMenu) {
                                        if (auto idEvent = btnEvent->AsIDEvent()) {
                                            idEvent->userEvent = userEvents->accept;
                                        }
                                        keepEvent = true;
                                        wasHandledInternally = true;
                                    }
                                }
                                else if (keyID == Settings::ButtonCancel) {
                                    isMapped = true;
                                    if (inMenu) {
                                        if (auto idEvent = btnEvent->AsIDEvent()) {
                                            idEvent->userEvent = userEvents->cancel;
                                        }
                                        keepEvent = true;
                                        wasHandledInternally = true;
                                    }
                                }

                                if (shouldSendMouse) {
                                    SendInput(1, &input, sizeof(INPUT));
                                    keepEvent = false;
                                }
                                else if (wasHandledInternally) {
                                    // keepEvent já foi setado corretamente como true
                                }
                                else if (isMapped) {
                                    // É mapeado no menu mas não atendeu às restrições (ex: Confirm fora de menus)
                                    keepEvent = false;
                                }
                                else {
                                    keepEvent = true;
                                }
                            }
                        }
                    }
                }
                else {
                    keepEvent = true;
                }
            }

            if (!keepEvent) {
                if (prev) prev->next = nextEvent;
                else      head = nextEvent;
            }
            else {
                prev = curr;
            }

            curr = nextEvent;
        }

        originalFunction(a_dispatcher, &head);
    }

    static inline REL::Relocation<decltype(thunk)> originalFunction;

    static void install() {
        SKSE::AllocTrampoline(28);
        auto& trampoline = SKSE::GetTrampoline();
        originalFunction = trampoline.write_call<5>(REL::RelocationID(67315, 68617, 67315).address() + REL::Relocate(0x7B, 0x7B, 0x81), thunk);
    }
};

bool OnInput(RE::InputEvent* event) {
    if (!event) return false;
    if (event->device != RE::INPUT_DEVICE::kKeyboard) return false;
    auto button = event->AsButtonEvent();
    if (!button) return false;
    return false;
}

void Hooks::Install() {
    ProcessInputQueueHook::install();
    TweenInputListener::Register();
}