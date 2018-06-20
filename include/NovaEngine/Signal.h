#pragma once

//http://simmesimme.github.io/tutorials/2015/09/20/signal-slot
//Copyright 2018 Simon Schneegans
//
//Permission is hereby granted, free of charge, to any person obtaining a
//copy of this software and associated documentation files(the "Software"),
//to deal in the Software without restriction, including without limitation
//the rights to use, copy, modify, merge, publish, distribute, sublicense,
//and/or sell copies of the Software, and to permit persons to whom the
//Software is furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in
//all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//DEALINGS IN THE SOFTWARE.

#include <functional>
#include <unordered_map>

// A signal object may call multiple slots with the
// same signature. You can connect functions to the signal
// which will be called when the emit() method on the
// signal object is invoked. Any argument passed to emit()
// will be passed to the given functions.

namespace Nova {
    template <typename... Args>
    class Signal {
    public:
        class Slot {
        public:
            Slot() {}
            Slot(Signal<Args...>* signal, uint32_t id) {
                this->signal = signal;
                this->id = id;
            }

            Slot(const Slot& other) = delete;
            Slot& operator = (const Slot& other) = delete;

            Slot(Slot&& other) {
                *this = std::move(other);
                signal->registerSlot(*this);
            }

            Slot& operator = (Slot&& other) {
                signal = other.signal;
                other.signal = nullptr;
                id = other.id;
                return *this;
            }

            ~Slot() {
                if (valid()) {
                    signal->disconnect(*this);
                }
            }

            bool valid() const {
                return signal != nullptr;
            }

        private:
            Signal<Args...>* signal = nullptr;
            uint32_t id;
            friend class Signal<Args>;
        };

    private:
        struct Item {
            std::function<void(Args...)> callback;
            Slot* slot;
        };

    public:
        Signal() {}
        Signal(const Signal& other) = delete;
        Signal& operator = (const Signal& other) = delete;
        Signal(Signal&& other) = delete;
        Signal& operator = (Signal&& other) = delete;

        ~Signal() {
            disconnectAll();
        }

        // connects a member function to this Signal
        template <typename T>
        Slot connectMember(T& inst, void (T::*func)(Args...)) {
            T* ptr = &inst;
            return connect([=](Args... args) {
                (ptr->*func)(args...);
            });
        }

        // connects a const member function to this Signal
        template <typename T>
        Slot connectMember(T& inst, void (T::*func)(Args...) const) {
            T* ptr = &inst;
            return connect([=](Args... args) {
                (ptr->*func)(args...);
            });
        }

        // connects a std::function to the signal
        Slot connect(std::function<void(Args...)> const& callback) {
            m_currentID++;
            m_slots.insert(std::make_pair(m_currentID, Item{ callback }));
            return Slot(this, m_currentID);
        }

        // disconnects a previously connected function
        void disconnect(Slot& slot) {
            m_slots.erase(slot.id);
            slot.signal = nullptr;
        }

        // disconnects all previously connected functions
        void disconnectAll() {
            for (auto& item : m_slots) {
                item.second.slot->signal = nullptr;
            }

            m_slots.clear();
        }

        // calls all connected functions
        void emit(Args... p) {
            for (auto& item : m_slots) {
                item.second.callback(p...);
            }
        }

    private:
        std::unordered_map<uint32_t, Item> m_slots;
        uint32_t m_currentID = 0;

        void registerSlot(Slot& slot) {
            m_slots[slot.id].slot = &slot;
        }
    };

    template <typename... Args>
    using Slot = typename Signal<Args...>::Slot;
}