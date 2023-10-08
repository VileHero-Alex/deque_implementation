#include <iostream>
#include <iterator>
#include <type_traits>

template <typename T>
class Deque {
  private:
    static const int block_sz = 16;

    T** data;
    size_t sz = 0;
    int cap = 0;
    int start_pos = 0, last_pos = 0;

    int block(int idx) const {
        return idx / block_sz;
    }

    int pos(int idx) const {
        return idx % block_sz;
    }

    void swap(Deque<T>& other) {
        std::swap(sz, other.sz);
        std::swap(cap, other.cap);
        std::swap(start_pos, other.start_pos);
        std::swap(last_pos, other.last_pos);
        std::swap(data, other.data);
    }

    void Delete() {
        for (int i = start_pos; i < last_pos; ++i) {
            (data[block(i)] + pos(i))->~T();
        }
        for (int i = 0; i < cap; ++i) {
            delete[] reinterpret_cast<char*>(data[i]);
        }
        delete[] data;
    }

    void relocate(T** tmp_data, int new_cap, int block_to_begin) {
        for (int i = 0; i < new_cap; ++i) {
            if (i < block_to_begin || i >= block_to_begin + cap) {
                data[i] = reinterpret_cast<T*>(new char[block_sz * sizeof(T)]);
            } else {
                data[i] = tmp_data[i - block_to_begin];
            }
        }
    }

  public:
    template <bool is_const>
    class common_iterator {
      private:
        T** data_ptr;
        T* block_ptr;

        friend Deque;

        int getAbsPos() const {
            return data_pos * block_sz + block_pos;
        }

      public:
        int data_pos;
        int block_pos;

        using value_type = std::conditional_t<is_const, const T, T>;
        using reference = std::conditional_t<is_const, T const&, T&>;
        using pointer = std::conditional_t<is_const, const T*, T*>;
        using difference_type = int;
        using iterator_category = std::random_access_iterator_tag;

        common_iterator() = default;
        common_iterator(const common_iterator& other) = default;
        common_iterator& operator=(const common_iterator& other) = default;
        explicit common_iterator(T** data_ptr, int data_pos, int block_pos)
            : data_ptr(data_ptr), data_pos(data_pos), block_pos(block_pos) {
            block_ptr = data_ptr[data_pos];
        }

        reference operator*() const {
            return *(block_ptr + block_pos);
        }

        pointer operator->() const {
            return block_ptr + block_pos;
        }

        common_iterator& operator+=(int dist) {
            int upd_val = dist + getAbsPos();
            data_pos = upd_val / block_sz;
            block_pos = upd_val % block_sz;
            block_ptr = data_ptr[data_pos];
            return *this;
        }

        common_iterator& operator-=(int dist) {
            return operator+=(-1 * dist);
        }
        common_iterator& operator++() {
            return operator+=(1);
        }
        common_iterator& operator--() {
            return operator-=(1);
        }

        common_iterator operator++(int) {
            auto copy = *this;
            operator+=(1);
            return copy;
        }

        common_iterator operator--(int) {
            auto copy = *this;
            operator-=(1);
            return copy;
        }

        common_iterator operator+(const int dist) const {
            auto copy = *this;
            copy += dist;
            return copy;
        }

        common_iterator operator-(const int dist) const {
            auto copy = *this;
            copy -= dist;
            return copy;
        }

        int operator-(const common_iterator& other) const {
            return getAbsPos() - other.getAbsPos();
        }

        bool operator==(const common_iterator& other) const {
            return (data_ptr == other.data_ptr &&
                    block_ptr == other.block_ptr &&
                    data_pos == other.data_pos && block_pos == other.block_pos);
        }

        bool operator!=(const common_iterator& other) const {
            return !(*this == other);
        }

        bool operator<(const common_iterator& other) const {
            return (data_pos < other.data_pos) ||
                   (data_pos == other.data_pos && block_pos < other.block_pos);
        }

        bool operator>(const common_iterator& other) const {
            return other < *this;
        }

        bool operator<=(const common_iterator& other) const {
            return !(other < *this);
        }

        bool operator>=(const common_iterator& other) const {
            return !(other > *this);
        }

        operator common_iterator<true>() const {
            return common_iterator<true>(data_ptr, data_pos, block_pos);
        }
    };

    using iterator = common_iterator<false>;
    using const_iterator = common_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() {
        return iterator(data, start_pos / block_sz, start_pos % block_sz);
    }
    const_iterator begin() const {
        return const_iterator(data, start_pos / block_sz, start_pos % block_sz);
    }

    iterator end() {
        return iterator(data, last_pos / block_sz, last_pos % block_sz);
    }
    const_iterator end() const {
        return iterator(data, last_pos / block_sz, last_pos % block_sz);
    }

    const_iterator cbegin() const {
        return const_iterator(data, start_pos / block_sz, start_pos % block_sz);
    }
    const_iterator cend() const {
        return iterator(data, last_pos / block_sz, last_pos % block_sz);
    }

    reverse_iterator rbegin() {
        return std::reverse_iterator(end());
    }
    reverse_iterator rend() {
        return std::reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const {
        return std::make_reverse_iterator(cend());
    }
    const_reverse_iterator crend() const {
        return std::make_reverse_iterator(cbegin());
    }

    void erase(iterator it) {
        while (it + 1 != (*this).end()) {
            *it = *(it + 1);
            ++it;
        }
        (*this).pop_back();
    }

    void insert(iterator it, const T& val) {
        T copy = val;
        while (it != (*this).end()) {
            std::swap(copy, *it);
            ++it;
        }
        (*this).push_back(copy);
    }

    T& operator[](int idx) {
        return data[block(start_pos + idx)][pos(start_pos + idx)];
    }

    const T& operator[](int idx) const {
        return data[block(start_pos + idx)][pos(start_pos + idx)];
    }

    ~Deque() {
        Delete();
    }

    Deque() {
        cap = 1;
        data = new T*[cap];
        data[0] = reinterpret_cast<T*>(new char[block_sz * sizeof(T)]);
    }

    Deque<T>& operator=(const Deque<T>& other) {
        if (this == &other) {
            return *this;
        }
        Deque<T> copy(other);
        swap(copy);
        return *this;
    }

    Deque(const Deque<T>& other) {
        data = new T*[other.cap];
        sz = other.sz;
        cap = other.cap;
        last_pos = other.last_pos;
        start_pos = other.start_pos;
        for (int i = 0; i < cap; ++i) {
            data[i] = reinterpret_cast<T*>(new char[block_sz * sizeof(T)]);
        }
        try {
            for (last_pos = start_pos;
                 last_pos < start_pos + static_cast<int>(sz); ++last_pos) {
                new (data[block(last_pos)] + pos(last_pos))
                    T(other[last_pos - start_pos]);
            }
        } catch (...) {
            Delete();
            sz = cap = last_pos = start_pos = 0;
            throw;
        }
    }

    Deque(int given_sz) : sz(given_sz), cap(3 * (sz / block_sz + 1)) {
        data = (new T*[cap]);
        for (int i = 0; i < cap; ++i) {
            data[i] = reinterpret_cast<T*>(new char[block_sz * sizeof(T)]);
        }
        start_pos = last_pos = cap * block_sz / 3;
        if constexpr (std::is_default_constructible<T>::value) {
            try {
                for (last_pos = start_pos;
                     last_pos < start_pos + static_cast<int>(sz); ++last_pos) {
                    new (data[block(last_pos)] + pos(last_pos)) T();
                }
            } catch (...) {
                Delete();
                sz = cap = last_pos = start_pos = 0;
                throw;
            }
        }
    }

    Deque(int given_sz, const T& val)
        : sz(given_sz), cap(3 * (sz / block_sz + 1)) {
        data = (new T*[cap]);
        for (int i = 0; i < cap; ++i) {
            data[i] = reinterpret_cast<T*>(new char[block_sz * sizeof(T)]);
        }
        start_pos = last_pos = cap * block_sz / 3;
        try {
            for (last_pos = start_pos;
                 last_pos < start_pos + static_cast<int>(sz); ++last_pos) {
                new (data[block(last_pos)] + pos(last_pos)) T(val);
            }
        } catch (...) {
            Delete();
            sz = cap = last_pos = start_pos = 0;
            throw;
        }
    }

    size_t size() const {
        return sz;
    }

    T& at(size_t idx) {
        if (idx >= sz) {
            throw std::out_of_range("");
        }
        return data[block(start_pos + idx)][pos(start_pos + idx)];
    }

    const T& at(size_t idx) const {
        if (idx < 0 || idx >= sz) {
            throw std::out_of_range("");
        }
        return data[block(start_pos + idx)][pos(start_pos + idx)];
    }

    void pop_back() {
        (data[block(last_pos - 1)] + pos(last_pos - 1))->~T();
        --last_pos;
        --sz;
    }

    void pop_front() {
        (data[block(start_pos)] + pos(start_pos))->~T();
        ++start_pos;
        --sz;
    }

    void push_back(const T& val) {
        if (last_pos < cap * block_sz) {
            new (data[block(last_pos)] + pos(last_pos)) T(val);
            ++last_pos;
            ++sz;
        }

        if (last_pos == cap * block_sz) {
            int new_sz = sz, new_cap = cap * 3;
            int new_start_pos = start_pos + new_cap * block_sz / 3;
            int new_last_pos = new_start_pos + new_sz;
            int block_to_begin = (new_start_pos - start_pos) / block_sz;
            T** tmp_data = data;
            data = (new T*[new_cap]);
            relocate(tmp_data, new_cap, block_to_begin);
            delete[] tmp_data;
            cap = new_cap;
            sz = new_sz;
            last_pos = new_last_pos;
            start_pos = new_start_pos;
        }
    }

    void push_front(const T& val) {
        if (start_pos - 1 > 0) {
            new (data[block(start_pos - 1)] + pos(start_pos - 1)) T(val);
            --start_pos;
            ++sz;
        } else {
            int new_sz = sz, new_cap = cap * 3;
            int new_start_pos = start_pos + new_cap * block_sz / 3;
            int new_last_pos = new_start_pos + new_sz;
            int block_to_begin = (new_start_pos - start_pos) / block_sz;
            T** tmp_data = data;
            data = (new T*[new_cap]);
            relocate(tmp_data, new_cap, block_to_begin);
            try {
                new (data[block(new_start_pos - 1)] + pos(new_start_pos - 1))
                    T(val);
                --new_start_pos;
                ++new_sz;
            } catch (...) {
                delete[] data;
                data = tmp_data;
                throw;
            }
            delete[] tmp_data;
            cap = new_cap;
            sz = new_sz;
            last_pos = new_last_pos;
            start_pos = new_start_pos;
        }
    }
};
