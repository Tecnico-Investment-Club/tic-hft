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

        std::cout << "[BinanceBroker] A tentar conectar a " << websocket_url_ << "..." << std::endl;

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