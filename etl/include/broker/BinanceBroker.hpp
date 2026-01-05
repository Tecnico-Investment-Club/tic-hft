// Herança de Broker, que implementa a comunicação com a broker Binance via WebSocket

#pragma once

#include "Broker.hpp"
#include "../../common/Constants.hpp" 
#include "../network/WebSocketClient.hpp"
#include "../transform/MarketDataParser.hpp"
#include <string>

namespace hft {

    class BinanceBroker : public Broker {
    public:
        // O Construtor agora é vazio de argumentos, pois o URL já é conhecido internamente
        BinanceBroker(KlineRingBuffer& buffer);

        ~BinanceBroker() override = default;

        void connect() override;
        void disconnect() override;
        bool is_connected() const override;

    private:
        // AQUI: Inicializamos a variável diretamente com o valor da constante.
        // Como estamos dentro do namespace 'hft', não precisamos de prefixo se o Constants também estiver lá.
        std::string websocket_url_ = BINANCE_TESTNET_URL;
        
        bool connected_ = false;

        WebSocketClient webSocketClient_; 

        MarketDataParser<BinanceTag> parser_;
    };
}