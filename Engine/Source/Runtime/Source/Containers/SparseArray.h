#pragma once

#include <stdexcept>
#include "Vector.h"
#include "Optional.h"
#include "Containers/Algorithm.h"
#include "Memory/Construct.h"


namespace Lumina
{
    /** Stable-index array; non-contiguous storage. */
    template <typename T>
    class TSparseArray
    {
    public:
        
        using Index = size_t;
        using value_type = T;
        using size_type = size_t;
    
        TSparseArray() = default;
        
        TSparseArray(const TSparseArray& other) = default;
        TSparseArray(TSparseArray&& other) noexcept = default;

        TSparseArray& operator=(const TSparseArray& other)
        {
            if (this != &other)
            {
                TSparseArray temp(other);
                *this = std::move(temp);
            }
            return *this;
        }
        
        TSparseArray& operator=(TSparseArray&& other) noexcept
        {
            if (this != &other)
            {
                Clear();
                Data = std::move(other.Data);
                FreeList = std::move(other.FreeList);
            }
            return *this;
        }
    
        ~TSparseArray()
        {
            Clear();
        }
    
        template<typename... Args>
        Index Emplace(Args&&... args)
        {
            Index idx;
            if (!FreeList.empty())
            {
                idx = FreeList.back();
                Data[idx].emplace(std::forward<Args>(args)...);
                FreeList.pop_back();
            }
            else
            {
                idx = Data.size();
                Data.emplace_back(Containers::InPlace, std::forward<Args>(args)...);
            }
            return idx;
        }
    
        Index Insert(const T& value)
        {
            return Emplace(value);
        }
    
        Index Insert(T&& value)
        {
            return Emplace(std::move(value));
        }
    
        bool Erase(Index idx)
        {
            if (idx >= Data.size() || !Data[idx].has_value())
            {
                return false;
            }
            
            FreeList.push_back(idx);
            Data[idx].reset();
            return true;
        }
    
        T* Get(Index idx) noexcept
        {
            return (idx < Data.size() && Data[idx].has_value()) ? Data[idx].operator->() : nullptr;
        }
    
        const T* Get(Index idx) const noexcept
        {
            return (idx < Data.size() && Data[idx].has_value()) ? Data[idx].operator->() : nullptr;
        }
    
        T& At(Index idx)
        {
            if (idx >= Data.size() || !Data[idx].has_value())
            {
                throw std::out_of_range("TSparseArray::At: invalid index");
            }
            return *Data[idx].operator->();
        }
    
        const T& At(Index idx) const
        {
            if (idx >= Data.size() || !Data[idx].has_value())
            {
                throw std::out_of_range("TSparseArray::At: invalid index");
            }
            return *Data[idx].operator->();
        }
    
        T& operator[](Index idx) noexcept
        {
            return *Data[idx].operator->();
        }
    
        const T& operator[](Index idx) const noexcept
        {
            return *Data[idx].operator->();
        }
    
        bool IsValid(Index idx) const noexcept
        {
            return idx < Data.size() && Data[idx].has_value();
        }
    
        void Clear() noexcept
        {
            Data.clear();
            FreeList.clear();
        }
    
        void Reserve(size_type capacity)
        {
            Data.reserve(capacity);
        }

        void ShrinkToFit()
        {
            Data.shrink_to_fit();
            FreeList.shrink_to_fit();
        }
    
        size_type Size() const noexcept { return Data.size() - FreeList.size(); }
        size_type Capacity() const noexcept { return Data.size(); }
        size_type FreeCount() const noexcept { return FreeList.size(); }
        bool Empty() const noexcept { return Size() == 0; }

        /** 0.0 = none, 1.0 = max fragmentation. */
        float FragmentationRatio() const noexcept 
        {
            return Data.empty() ? 0.0f : static_cast<float>(FreeList.size()) / Data.size();
        }
    
        void Defragment()
        {
            if (FreeList.empty())
            {
                return;
            }

            Algo::Sort(FreeList);

            Index writeIdx = 0;
            Index readIdx = 0;

            while (readIdx < Data.size()) {
                if (Data[readIdx].has_value()) {
                    if (writeIdx != readIdx) {
                        Data[writeIdx].emplace(std::move(*Data[readIdx]));
                        Data[readIdx].reset();
                    }
                    ++writeIdx;
                }
                ++readIdx;
            }

            Data.resize(writeIdx);
            FreeList.clear();
        }

        class const_iterator;

        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T*;
            using reference = T&;
    
            iterator() : Owner(nullptr), IndexValue(0) {}
            iterator(TSparseArray* owner, Index idx) : Owner(owner), IndexValue(idx) 
            {
                SkipHoles();
            }
    
            iterator& operator++()
            {
                ++IndexValue;
                SkipHoles();
                return *this;
            }
    
            iterator operator++(int)
            {
                iterator temp = *this;
                ++(*this);
                return temp;
            }
    
            reference operator*() const
            {
                return *Owner->Data[IndexValue].operator->();
            }
    
            pointer operator->() const
            {
                return Owner->Data[IndexValue].operator->();
            }
    
            bool operator==(const iterator& other) const
            {
                return Owner == other.Owner && IndexValue == other.IndexValue;
            }
    
            bool operator!=(const iterator& other) const
            {
                return !(*this == other);
            }
    
            Index GetIndex() const { return IndexValue; }
    
        private:
            
            void SkipHoles()
            {
                while (IndexValue < Owner->Data.size() && !Owner->Data[IndexValue].has_value())
                {
                    ++IndexValue;
                }
            }
    
            friend class const_iterator;

            TSparseArray* Owner;
            Index IndexValue;
        };
    
        class const_iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = const T;
            using difference_type = std::ptrdiff_t;
            using pointer = const T*;
            using reference = const T&;
    
            const_iterator() : Owner(nullptr), IndexValue(0) {}
            const_iterator(const TSparseArray* owner, Index idx) : Owner(owner), IndexValue(idx) 
            {
                SkipHoles();
            }
            const_iterator(const iterator& it) : Owner(it.Owner), IndexValue(it.IndexValue) {}
    
            const_iterator& operator++()
            {
                ++IndexValue;
                SkipHoles();
                return *this;
            }
    
            const_iterator operator++(int)
            {
                const_iterator temp = *this;
                ++(*this);
                return temp;
            }
    
            reference operator*() const
            {
                return *Owner->Data[IndexValue].operator->();
            }
    
            pointer operator->() const
            {
                return Owner->Data[IndexValue].operator->();
            }
    
            bool operator==(const const_iterator& other) const
            {
                return Owner == other.Owner && IndexValue == other.IndexValue;
            }
    
            bool operator!=(const const_iterator& other) const
            {
                return !(*this == other);
            }
    
            Index GetIndex() const { return IndexValue; }
    
        private:
            
            void SkipHoles()
            {
                while (IndexValue < Owner->Data.size() && !Owner->Data[IndexValue].has_value())
                {
                    ++IndexValue;
                }
            }
    
            const TSparseArray* Owner;
            Index IndexValue;
        };
    
        iterator begin() { return iterator(this, 0); }
        iterator end() { return iterator(this, Data.size()); }
        const_iterator begin() const { return const_iterator(this, 0); }
        const_iterator end() const { return const_iterator(this, Data.size()); }
        const_iterator cbegin() const { return const_iterator(this, 0); }
        const_iterator cend() const { return const_iterator(this, Data.size()); }
    
    private:
        
        TVector<Containers::TOptional<T>> Data;
        TVector<Index> FreeList;
    };
}
