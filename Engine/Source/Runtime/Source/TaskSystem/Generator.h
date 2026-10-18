#pragma once

#include "Core/Assertions/Assert.h"

#include <coroutine>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace Lumina
{
    // A lazy sequence written with co_yield, run one value at a time by a range for, with no container behind it.
    template<typename T>
    class TGenerator
    {
    public:
        struct promise_type
        {
            std::add_pointer_t<std::remove_reference_t<T>> Current = nullptr;

            TGenerator get_return_object() noexcept { return TGenerator(std::coroutine_handle<promise_type>::from_promise(*this)); }

            std::suspend_always initial_suspend() const noexcept { return {}; }
            std::suspend_always final_suspend() const noexcept { return {}; }

            std::suspend_always yield_value(std::remove_reference_t<T>& Value) noexcept
            {
                Current = std::addressof(Value);
                return {};
            }

            std::suspend_always yield_value(std::remove_reference_t<T>&& Value) noexcept
            {
                Current = std::addressof(Value);
                return {};
            }

            void return_void() const noexcept {}

            void unhandled_exception() { ASSERT(false); }

            // A generator only yields, so awaiting inside one is a mistake caught here.
            template<typename U>
            std::suspend_never await_transform(U&&) = delete;
        };

        using FHandle = std::coroutine_handle<promise_type>;

        class FIterator
        {
        public:
            using iterator_category = std::input_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = std::remove_cvref_t<T>;

            FIterator() = default;
            explicit FIterator(FHandle InHandle) : Handle(InHandle) {}

            FIterator& operator++()
            {
                Handle.resume();
                return *this;
            }

            void operator++(int) { ++*this; }

            std::remove_reference_t<T>& operator*() const { return *Handle.promise().Current; }

            bool operator==(std::default_sentinel_t) const { return !Handle || Handle.done(); }

        private:
            FHandle Handle{};
        };

        TGenerator() = default;
        explicit TGenerator(FHandle InHandle) : Handle(InHandle) {}
        TGenerator(TGenerator&& Other) noexcept : Handle(std::exchange(Other.Handle, {})) {}
        TGenerator& operator=(TGenerator&& Other) noexcept
        {
            if (this != &Other)
            {
                Reset();
                Handle = std::exchange(Other.Handle, {});
            }
            return *this;
        }
        TGenerator(const TGenerator&) = delete;
        TGenerator& operator=(const TGenerator&) = delete;
        ~TGenerator() { Reset(); }

        FIterator begin()
        {
            if (Handle)
            {
                Handle.resume();
            }
            return FIterator(Handle);
        }

        std::default_sentinel_t end() const noexcept { return {}; }

        // Advances to the next value, false once the body has ended.
        bool Resume()
        {
            if (!Handle || Handle.done())
            {
                return false;
            }
            Handle.resume();
            return !Handle.done();
        }

        std::remove_reference_t<T>& Current() const { return *Handle.promise().Current; }

    private:
        void Reset()
        {
            if (Handle)
            {
                Handle.destroy();
                Handle = {};
            }
        }

        FHandle Handle{};
    };
}
