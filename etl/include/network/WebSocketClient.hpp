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

        void connect(const std::string& url);
        void disconnect();
        bool isConnected() const;
        void send(const std::string& message);

        // Callbacks para conexão
        using ConnectCallback = std::function<void()>;

        // Callback para mensagens 
        using MessageCallback = std::function<void(const std::string&)>;

        // Funções para a Broker configurar os callbacks
        void setOnConnect(ConnectCallback callback);
        void setOnMessage(MessageCallback callback);

    private:
        ix::WebSocket webSocket_;
        
        // Variável atómica para evitar conflitos de leitura/escrita entre threads
        std::atomic<bool> connected_{false};

        // Onde são guardadas as funções que a Broker define
        ConnectCallback onConnect_;
        MessageCallback onMessage_;
    };
}