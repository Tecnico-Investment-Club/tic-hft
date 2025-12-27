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
                if (k_obj["s"].get(symbol_sv) == simdjson::SUCCESS) {
                    size_t len = symbol_sv.length();
                    if (len >= sizeof(kline.symbol)) {
                        len = sizeof(kline.symbol) - 1; // Deixa espaço para o \0
                    }
                    std::memcpy(kline.symbol, symbol_sv.data(), len);

                    kline.symbol[len] = '\0';
                }
                
                // FIXME perceber o que fazer quanto ao ID

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
                if (k_obj["q"].get(val_str) == simdjson::SUCCESS) kline.quote_volume = std::stod(std::string(val_str));
                if (k_obj["V"].get(val_str) == simdjson::SUCCESS) kline.taker_buy_volume = std::stod(std::string(val_str));
                if (k_obj["Q"].get(val_str) == simdjson::SUCCESS) kline.taker_buy_quote_volume = std::stod(std::string(val_str));
                

                // Prints de debug
                std::cout << "\n========================================" << std::endl;
                std::cout << "✅ KLINE FINALIZADA PROCESSADA COM SUCESSO" << std::endl;
                std::cout << "========================================" << std::endl;
                std::cout << "🔹 Identificação:" << std::endl;
                std::cout << "   Symbol: " << kline.symbol << std::endl;
                std::cout << "   ID:     " << kline.id << std::endl;
                std::cout << "🔹 Tempos:" << std::endl;
                std::cout << "   Open Time:  " << kline.open_time << std::endl;
                std::cout << "   Close Time: " << kline.close_time << std::endl;
                std::cout << "🔹 Preços:" << std::endl;
                std::cout << "   Open:  " << kline.open_price << std::endl;
                std::cout << "   High:  " << kline.high_price << std::endl;
                std::cout << "   Low:   " << kline.low_price << std::endl;
                std::cout << "   Close: " << kline.close_price << std::endl;
                std::cout << "🔹 Volumes & Atividade:" << std::endl;
                std::cout << "   Volume (Base):  " << kline.volume << std::endl;
                std::cout << "   Volume (Quote): " << kline.quote_volume << std::endl;
                std::cout << "   Taker Buy Vol:  " << kline.taker_buy_volume << std::endl;
                std::cout << "   Taker Buy Quote Vol: " << kline.taker_buy_quote_volume << std::endl;
                std::cout << "   Num Trades:     " << kline.trades << std::endl;
                std::cout << "========================================\n" << std::endl;

                //FIXME enviar a kline para o RingBuffer
            }

        } catch (const std::exception& e) {
            std::cerr << "Erro no parser: " << e.what() << std::endl;
        }
    }
}