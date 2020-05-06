#ifndef UDP_COMMONS_QUEUE_HPP_
#define UDP_COMMONS_QUEUE_HPP_


#include <atomic>
#include <cinttypes>
#include <memory>

#include "commons/macros.h"
#include "commons/utils.hpp"


namespace udp { ;


template <typename T>
class MpmcBoundedQueue final {
    NOCOPY(MpmcBoundedQueue)
    NOMOVE(MpmcBoundedQueue)
public:

    using SPtr = std::shared_ptr<MpmcBoundedQueue>;
    using UPtr = std::shared_ptr<MpmcBoundedQueue>;
    

    explicit MpmcBoundedQueue(size_t szBuffer) noexcept {

        size_t size = ToPowerOf2(szBuffer);

        if (size > 0) {
            _buffer = new Cell[size];
            _mask = (size - 1);

            for (size_t i = 0; i < size; ++i)
                _buffer[i].mSeq.store(i, std::memory_order_relaxed);
        } else {
            _buffer = nullptr;
            _mask = 0;
        }

        _posEnqueue.store(0, std::memory_order_relaxed);
        _posDequeue.store(0, std::memory_order_relaxed);
    }


    ~MpmcBoundedQueue() noexcept {

        delete[] _buffer;
    }


    bool valid() const noexcept {

        return !!_buffer;
    }


    bool enqueue(T&& data) noexcept {

        Cell* cell;
        size_t pos = _posEnqueue.load(std::memory_order_relaxed);

        while(true) {
            cell = &_buffer[pos & _mask];
            size_t seq = cell->mSeq.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)pos;
            if (0 == diff) {
                if (_posEnqueue.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    break;
            } else if (diff < 0) {
                return false;
            } else {
                pos = _posEnqueue.load(std::memory_order_relaxed);
            }
        }

        cell->mData = std::move(data);
        cell->mSeq.store(pos + 1, std::memory_order_release);

        return true;
    }


    bool dequeue(T& outData) noexcept {

        Cell* cell;
        size_t pos = _posDequeue.load(std::memory_order_relaxed);

        while(true) {
            cell = &_buffer[pos & _mask];
            size_t seq = cell->mSeq.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);
            if (0 == diff) {
                if (_posDequeue.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    break;
            } else if (diff < 0) {
                return false;
            } else {
                pos = _posDequeue.load(std::memory_order_relaxed);
            }
        }

        outData = std::move(cell->mData);
        cell->mSeq.store(pos + _mask + 1, std::memory_order_release);

        return true;
    }

private:

    struct Cell {
        std::atomic<size_t> mSeq;
        T                   mData;
    };

    CACHELINE(0);

    Cell*  _buffer;
    size_t _mask;

    CACHELINE(1);

    std::atomic<size_t> _posEnqueue;

    CACHELINE(2);

    std::atomic<size_t> _posDequeue;

    CACHELINE(3);

};


}


#endif//UDP_COMMONS_QUEUE_HPP_
