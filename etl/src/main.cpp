// Não deve ter lógica complexa, apenas seguir os passos:
// 1) Cria a BlockingQueue e o RingBuffer
// 2) Instanciar a broker em causa e dar-lhe acesso à BlockingQueue(escrita)
// 3) Instanciar o MarketDataParser e dar-lhe acesso à BlockingQueue(leitura) e ao RingBuffer(escrita)
// 4) Lançar as threads e conexão com a broker

#include <iostream>

int main() {
    std::cout << "SETUP INICIAL HTF ETL!" << std::endl;
    return 0;
}