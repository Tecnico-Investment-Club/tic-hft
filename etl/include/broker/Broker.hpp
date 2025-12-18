// Classe abstrata que define o que cada broker deve ter como base

#pragma once // Para que o ficheiro seja incluído apenas uma vez durante a compilação

namespace hft {

    class Broker {
    public:
        // O "~" indica que é um destrutor, prevenindo leaks de memória em heranças
        virtual ~Broker() = default;

        // Quem herdar desta classe TEM de implementar estes métodos
        
        virtual void connect() = 0;
        
        virtual void disconnect() = 0;

        virtual bool is_connected() const = 0;
    };
}