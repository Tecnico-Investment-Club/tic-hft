// Definição da struct Kline e todas as "globais"
// Este ficheiro pode e deve ser utilizado pelos outros "blocos", como o strategy
// Neste momento está apenas dentro do ETL para testes

#pragma once // Para que o ficheiro seja incluído apenas uma vez durante a compilação
#include <cstdint> // Necessário para int64_t

namespace hft { // Usado para evitar conflitos de nomes

    struct Kline {
        // Inicializamos tudo a 0 para evitar lixo de memória

        // Usa-se int64_t para timestamps Unix em milissegundos
        int64_t open_time = 0;
        int64_t close_time = 0;

        // Variáveis necessárias para identificação e contagem        
        int64_t id = 0;
        int64_t trades = 0;

        // Variáveis financeiras
        double open_price = 0.0;
        double high_price = 0.0;
        double low_price = 0.0;
        double close_price = 0.0;
        double volume = 0.0;
        double quote_volume = 0.0;
        double taker_buy_volume = 0.0;
        double taker_buy_quote_volume = 0.0;

        // 16 bytes cobre desde "BTCUSDT" (7 bytes), até "1000SHIBUSDT" (12 bytes) com segurança
        // Garante alinhamento perfeito na memória (múltiplo de 8)
        char symbol[16] = {0};

        // Útil para o parser saber se deve ignorar a vela ou não.
        bool is_closed = false;
    };
}