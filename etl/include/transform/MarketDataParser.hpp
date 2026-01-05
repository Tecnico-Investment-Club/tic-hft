// Transforma a Raw Data em Klines
// Deve usar a biblioteca simdjson
#pragma once
#include <string_view>
#include "simdjson.h"
#include "../core/Types.hpp" // Onde está a tua struct Kline
#include "../core/RingBuffer.hpp"
#include "../../common/BrokerTags.hpp"

namespace hft {

    using KlineRingBuffer = RingBuffer<Kline, 1024>;

    template <typename BrokerTag> // Dependendo da broker, o parser pode mudar
    class MarketDataParser {
    public:
    
        MarketDataParser(KlineRingBuffer& buffer); 
        ~MarketDataParser() = default;

        void processMessage(std::string_view json_message);

    private:
        simdjson::ondemand::parser parser_;

        KlineRingBuffer& ringBuffer_;
    };
}