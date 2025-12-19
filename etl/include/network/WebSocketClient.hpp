// Comunicação via WebSocket com o broker para receber dados de mercado em tempo real
// Usa a abstração do diretório broker, não sabendo com que broker em específico está a falar
// Deve usar a biblioteca IXWebSocket e reconectar automaticamente em caso de falha

#pragma once

#include <string>
#include <atomic> // Para gerir o estado thread-safe
#include <ixwebsocket/IXWebSocket.h> 

namespace hft {

    class WebSocketClient {
    public:
        WebSocketClient();
        ~WebSocketClient();

        // Configura e inicia a conexão
        void connect(const std::string& url);

        // Fecha a conexão
        void disconnect();

        // Verifica se o handshake foi bem sucedido
        bool isConnected() const;

    private:
        ix::WebSocket webSocket_;
        
        // Variável atómica para evitar conflitos de leitura/escrita entre threads
        std::atomic<bool> connected_{false};
    };
}