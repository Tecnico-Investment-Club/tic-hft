#include "../../include/network/WebSocketClient.hpp"
#include <iostream>

namespace hft {

    WebSocketClient::WebSocketClient() {
        // FIXEME opções de configuração 
    }

    WebSocketClient::~WebSocketClient() {
        disconnect();
    }

    void WebSocketClient::connect(const std::string& url) {
        webSocket_.setUrl(url);

        // Precisamos deste callback interno apenas para saber quando a conexão abre ou fecha
        webSocket_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
            if (msg->type == ix::WebSocketMessageType::Open) {
                std::cout << "[WebSocketClient] Handshake efetuado! Conectado." << std::endl;
                connected_ = true;

                // Se a Broker definiu algua função para este evento, chamamos aqui
                if (onConnect_) onConnect_(); 
            }
            else if (msg->type == ix::WebSocketMessageType::Message) {
                // Se a Broker definiu alguma função para este evento, chamamos aqui
                if (onMessage_) onMessage_(msg->str);
            }
            else if (msg->type == ix::WebSocketMessageType::Close) {
                std::cout << "[WebSocketClient] Conexão fechada pelo servidor." << std::endl;
                std::cout << " > Motivo: " << msg->closeInfo.reason << std::endl;
                connected_ = false;
            }
            else if (msg->type == ix::WebSocketMessageType::Error) {
                std::cout << "[WebSocketClient] ERRO CRÍTICO: " << msg->errorInfo.reason << std::endl;
                connected_ = false;
            }
        });

        // Inicia a thread de rede em background
        webSocket_.start();
    }

    void WebSocketClient::disconnect() {

        //FIXEME lógica de desconexão

        connected_ = false;
    }

    void WebSocketClient::setOnConnect(ConnectCallback callback) {
        onConnect_ = callback;
    }

    void WebSocketClient::setOnMessage(MessageCallback callback) {
        onMessage_ = callback;
    }

    void WebSocketClient::send(const std::string& message) {
        webSocket_.send(message);
    }

    bool WebSocketClient::isConnected() const {
        return connected_;
    }
}