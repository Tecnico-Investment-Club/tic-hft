#include "../../include/transform/MarketDataParser.hpp"
#include <iostream>
#include <cstring> // Necessário para strncpy
#include <string>  // Necessário para std::stod

namespace hft {

    void MarketDataParser::processMessage(std::string_view json_message) {
        simdjson::padded_string json(json_message);
        simdjson::ondemand::document doc;

        auto error = parser_.iterate(json).get(doc);
        if (error) return;

        try {
            simdjson::ondemand::object k_obj;
            // Verifica se existe o objeto "k"
            if (doc["k"].get(k_obj) != simdjson::SUCCESS) return;

            // Verifica se a vela fechou ("x": true)
            bool is_closed = false;
            if (k_obj["x"].get(is_closed) != simdjson::SUCCESS) return;

            if (is_closed) {
                Kline kline;
                
                // Simbolo (s): É uma string, precisamos copiar para o char[]
                std::string_view symbol_sv;
                if (doc["s"].get(symbol_sv) == simdjson::SUCCESS) {
                    // Copia com segurança para evitar buffer overflow
                    std::strncpy(kline.symbol, symbol_sv.data(), sizeof(kline.symbol) - 1);
                }
                
                int64_t val_int;
                if (k_obj["t"].get(val_int) == simdjson::SUCCESS) kline.open_time = val_int;
                if (k_obj["T"].get(val_int) == simdjson::SUCCESS) kline.close_time = val_int;
                if (k_obj["n"].get(val_int) == simdjson::SUCCESS) kline.trades = val_int;
                
                std::string_view val_str;

                if (k_obj["o"].get(val_str) == simdjson::SUCCESS) kline.open_price = std::stod(std::string(val_str));
                if (k_obj["c"].get(val_str) == simdjson::SUCCESS) kline.close_price = std::stod(std::string(val_str));
                if (k_obj["h"].get(val_str) == simdjson::SUCCESS) kline.high_price = std::stod(std::string(val_str));
                if (k_obj["l"].get(val_str) == simdjson::SUCCESS) kline.low_price = std::stod(std::string(val_str));
                if (k_obj["v"].get(val_str) == simdjson::SUCCESS) kline.volume = std::stod(std::string(val_str));
                
                // FIXME ADICIONAR O RESTO DOS PARÃMETROS DA KLINE

                // FIXEME DAR JÁ ESTAMOS APENAS A DAR PRINT
                std::cout << "=== KLINE FECHADA ===" << std::endl;
                std::cout << "Par: " << kline.symbol << std::endl;
                std::cout << "Close: " << kline.close_price << std::endl;
                std::cout << "Volume: " << kline.volume << std::endl;
                std::cout << "Trades: " << kline.trades << std::endl;
                std::cout << "---------------------" << std::endl;
            }

        } catch (const std::exception& e) {
            std::cerr << "Erro no parser: " << e.what() << std::endl;
        }
    }
}