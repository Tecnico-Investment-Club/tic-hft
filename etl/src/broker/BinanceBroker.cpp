#include "../../include/broker/BinanceBroker.hpp"
#include <iostream> // Para podermos fazer logs na consola

namespace hft {

    // Exibir apenas um log enquanto não existem argumentos
    // No futuro, podemos pôr o BINANCE_TESTNET_URL como argumento do construtor
    BinanceBroker::BinanceBroker() {
        std::cout << "[BinanceBroker] Objeto criado. URL base: " << websocket_url_ << std::endl;
    }

    // Função que conecta ao WebSocket da Binance
    void BinanceBroker::connect() {
        if (this->is_connected()) return; // Se já estiver ligado, não faz nada

        // Função que define o que acontece quando é feita a conexão à Broker
        webSocketClient_.setOnConnect([this]() {
            std::cout << "[BinanceBroker] Conectado! A enviar subscrição..." << std::endl;
            
            // Usamos R"(...)" para criar uma string JSON limpa sem ter de escapar aspas
            std::string json_pedido = R"(
            {
                "method": "SUBSCRIBE",
                "params": [
                    "btcusdt@kline_1m"
                ],
                "id": 1
            }
            )";
            
            // Enviamos o pedido usando o nosso estafeta
            webSocketClient_.send(json_pedido);

        });

        webSocketClient_.setOnMessage([this](const std::string& msg) {
            parser_.processMessage(msg);
        });
        

        webSocketClient_.connect(websocket_url_);
    }

    // Função que disconecta do WebSocket da Binance
    void BinanceBroker::disconnect() {
        std::cout << "[BinanceBroker] A desligar..." << std::endl;
                
        connected_ = false;
    }

    bool BinanceBroker::is_connected() const {
        return connected_;
    }

}