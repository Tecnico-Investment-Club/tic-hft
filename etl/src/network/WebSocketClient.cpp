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

        // Precisamos deste callback INTERNO apenas para saber quando a conexão abre ou fecha.
        // NADA sai daqui para fora (sem carteiros para a Broker).
        webSocket_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
            if (msg->type == ix::WebSocketMessageType::Open) {
                std::cout << "[WebSocketClient] Handshake efetuado! Conectado." << std::endl;
                connected_ = true;
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
            // Ignoramos qualquer mensagem de dados (msg->str) por agora.
        });

        // Inicia a thread de rede em background
        webSocket_.start();
    }

    void WebSocketClient::disconnect() {

        //FIXEME lógica de desconexão

        connected_ = false;
    }

    bool WebSocketClient::isConnected() const {
        return connected_;
    }
}