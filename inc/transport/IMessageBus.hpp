#pragma once

#include "core/Types.hpp"

namespace tic {

    // MESSAGE BUS INTERFACE
    // Interface that all message bus layers must implement
    class IMessageBus {
        public:
        // Attributes
        std::mutex mtx; // Mutex for thread safety
        std::vector<Tick> tickQueue; // Queue to store incoming ticks

        // Destructor
        virtual ~IMessageBus() = default;

        // Append a new tick to the message bus
        virtual void publishTick(const Tick& tick);

        // Retrieve the next tick from the message bus
        virtual Tick consumeTick();
    }