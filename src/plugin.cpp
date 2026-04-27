#include "Logger.h"
#include "Hooks.h"
#include "Settings.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == InputManagerAPI::kMessage_ProvideAPI) {
        InputManagerAPI::ReceiveAPI(message);
        logger::info("API do Input Manager recebida com sucesso!");
    }
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        InputManagerAPI::RequestAPIDirect();

        if (InputManagerAPI::_API) {
            logger::info("API do Input Manager conectada");
        }
        Settings::RegisterMenu();
        if (InputManagerAPI::_API && Settings::MouseModeActionID != -1) {
            // Permitir apenas Tap (1) ou Press (4) para ativar o Mouse Mode

            InputManagerAPI::_API->UpdateListener(
                0,                           // Tipo de Input (0 = Action)
                Settings::MouseModeActionID, // ID da Ação
                "Mouse Mode",          // Nome do Mod (Pode mudar pro nome do seu)
                "Toggle Mouse Mode",         // Propósito
                true,                        // isRegistering (Estamos registrando)
                nullptr,                   // Array com os estados permitidos
                0,                           // Quantidade de estados (Tap e Press)
                nullptr,                     // Sem restrição de modificador
                0                            // 0 modificadores restritos
            );
        }
    }
    if (message->type == SKSE::MessagingInterface::kPostLoad) {

    }


}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    Hooks::Install();
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    SetupLog();
    logger::info("Plugin loaded");
    
    return true; Settings::RegisterMenu();
}