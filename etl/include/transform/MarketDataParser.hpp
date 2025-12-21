// Transforma a Raw Data em Klines
// Deve usar a biblioteca simdjson
// Está num loop infinito que dispara quando a BlickingQueue tem novos dados 
#pragma once
#include <string_view>
#include "simdjson.h"
#include "../core/Types.hpp" // Onde está a tua struct Kline

namespace hft {

    class MarketDataParser {
    public:
        MarketDataParser() = default; 
        ~MarketDataParser() = default;

        void processMessage(std::string_view json_message);

    private:
        simdjson::ondemand::parser parser_;
    };
}