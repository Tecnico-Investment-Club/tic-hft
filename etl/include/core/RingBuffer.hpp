// Estrutura circular de tamanho fixo sem bloqueios que guarda a informação vinda do ETL
// Passa as klines definidas no Transform, para o Strategy

#pragma once
#include <array>
#include <atomic>

namespace hft {

    // O template serve para conseguirmos criar RingBuffers de qualquer tipo e tamanho (Klines, Ordens, etc)
    template <typename T, size_t Size>
    class RingBuffer {
    public:
        RingBuffer() : head_(0), tail_(0) {}

        void push(const T& item) {

            // Calculamos a próxima posição da cabeça para ver se estará bater na cauda
            const auto current_head = head_.load(std::memory_order_relaxed);
            const auto next_head = (current_head + 1) % Size;
            const auto current_tail = tail_.load(std::memory_order_acquire);
            
            if (next_head == current_tail) {
                // Se estivermos a bater na cauda, apagamos o item mais antigo 
                tail_.store((current_tail + 1) % Size, std::memory_order_release);
            }

            // Escrevemos o novo item
            buffer_[current_head] = item;
            
            // Atualizamos a cabeça
            head_.store(next_head, std::memory_order_release);
        }

        bool pop(T& item) {
            const auto current_tail = tail_.load(std::memory_order_relaxed);

            if (current_tail == head_.load(std::memory_order_acquire)) {
                return false; 
            }

            item = buffer_[current_tail];
            tail_.store((current_tail + 1) % Size, std::memory_order_release);
            return true;
        }

    private:
        std::array<T, Size> buffer_;
        std::atomic<size_t> head_;
        std::atomic<size_t> tail_;
    };
}