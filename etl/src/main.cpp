// Não deve ter lógica complexa, apenas seguir os passos:
// 1) Cria a BlockingQueue e o RingBuffer
// 2) Instanciar o MarketDataParser e dar-lhe acesso ao RingBuffer(escrita)
// 3) Lançar as threads e conexão com a broker

#include "../include/broker/BinanceBroker.hpp"
#include "../include/transform/MarketDataParser.hpp"
#include <iostream>
#include <thread>
#include <chrono> // Necessário para tempos

int main() {
    // Criar o Buffer
    hft::KlineRingBuffer klineBuffer;

    // Criar o Broker e ligar
    hft::BinanceBroker broker(klineBuffer);
    broker.connect();

    // Loop principal que processa as klines do RingBuffer
    // FIXME apenas para testes iniciais, pois isto será feito no Strategy
    while (true) {
        hft::Kline kline;
        
        // Loop que esvazia o buffer todo de uma vez quando acorda
        // Usamos um while interno para processar tudo o que acumulou enquanto dormia
        while (klineBuffer.pop(kline)) {
            std::cout << "--------------------------------" << std::endl;
            std::cout << "⏰ KLINE PROCESSADA" << std::endl;
            std::cout << "Symbol: " << kline.symbol << std::endl;
            std::cout << "Close Price: " << kline.close_price << std::endl;
            std::cout << "--------------------------------" << std::endl;
        }

        // FIXME Dorme durante 5 segundos 
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    return 0;
}