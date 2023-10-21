#ifndef VECTOR
#define VECTOR

#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>

template<typename T, typename Alloc = std::allocator<T>>
class Vector
{
  T* arr_;
  size_t size_ = 0;
  size_t cap_ = 0;
  Alloc alloc;

  template<bool IsConst>
  struct _iterator
  {
  private:
    using ConditionalPtr = std::conditional_t<IsConst, const T*, T*>;
    using ConditionalRef = std::conditional_t<IsConst, const T&, T&>;

    ConditionalPtr ptr;

  public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;   // or also value_type*
    using reference = T&; // or also value_type&
    _iterator(T* p = nullptr)
      : ptr(p)
    {
    }

    _iterator(const _iterator<IsConst>& it) = default;

    ~_iterator() {}

    ConditionalRef operator*() { return *ptr; }
    ConditionalPtr operator->() { return ptr; }

    _iterator<IsConst>& operator+=(int n)
    {
      ptr += n;
      return *this;
    }

    _iterator<IsConst>& operator-=(int n)
    {
      ptr -= n;
      return *this;
    }

    _iterator<IsConst> operator+(int n)
    {
      auto copy = *this;
      copy += n;
      return copy;
    }

    _iterator<IsConst> operator-(int n)
    {
      auto copy = *this;
      copy -= n;
      return copy;
    }
  };

public:
  using iterator = _iterator<false>;
  using const_iterator = _iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  using Alt = std::allocator_traits<Alloc>;

  Vector()
    : alloc(Alloc())
  {
    arr_ = Alt::allocate(alloc, 0);
  }

  Vector(size_t n, const Alloc& a = Alloc()) : alloc(a)
  {
    std::cout << "Vector(n)" << '\n';
    reserve(n);
    for (size_t i = 0; i < n; ++i) {
      Alt::construct(alloc, arr_ + i, T());
      ++size_;
    }
  }

  Vector(size_t n, const T& value, const Alloc& a = Alloc())
    : alloc(a)
  {
    std::cout << "Vector(n, val)" << '\n';
    reserve(n);
    for (size_t i = 0; i < n; ++i) {
      Alt::construct(alloc, arr_ + i, value);
      ++size_;
    }
  }

  // Vector(std::iterator first, std::iterator last, const Alloc& a = Alloc())
  // {
  // }

  // Vector(std::initializer_list) {}

  Vector(const Vector<T, Alloc>& v)
  {
    for (size_t i = 0; i < size_; ++i) {
      alloc.destroy(arr_ + i);
    }
    alloc.deallocate(arr_);
  }

  Vector& operator=(const Vector<T, Alloc>& v)
  {
    for (size_t i = 0; i < size_; ++i) {
      Alt::destroy(alloc, arr_ + i);
    }
    Alt::deallocate(arr_);

    if (alloc != v.alloc)
      alloc = v.alloc;

    try {
      reserve(v.size_);
      for (size_t i = 0; i < size_; ++i) {
        Alt::construct(alloc, arr_ + i, v[i]);
      }
    } catch (...) {
      //
    }
  }

  Vector(Vector&& v)
    : arr_(std::move(v.arr_))
    , size_(v.size_)
    , cap_(v.cap_)
    , alloc(v.alloc)
  {
    v.arr_ = nullptr;
    v.size_ = 0;
    v.cap_ = 0;
  }

  ~Vector() {}

  Vector operator=(Vector&& v) noexcept
  {
    Vector newVec(std::move(v));
    swap(newVec);
    return *this;
  }

  size_t size() const { return size_; }

  size_t capacity() const { return cap_; };

  void reserve(size_t n)
  {
    if (n <= cap_) {
      return;
    }

    T* newarr = Alt::allocate(alloc, n);
    size_t i = 0;

    try {
      for (i = 0; i < size_; ++i) {
        Alt::construct(alloc, newarr + i, std::move_if_noexcept(arr_[i]));
      }
    } catch (...) {
      for (size_t j = 0; j < i; ++j) {

        Alt::destroy(alloc, newarr + j);
      }
      Alt::deallocate(alloc, newarr, n);
      throw;
    }

    for (size_t i = 0; i < size_; ++i) {
      Alt::destroy(alloc, arr_ + i);
    }
    if (cap_ != 0) Alt::deallocate(alloc, arr_, cap_);

    arr_ = newarr;
    cap_ = n;
  }

  void resize(size_t n, const T& value = T())
  {
    if (n > cap_)
      reserve(n);
    for (size_t i = size_; i < n; ++i) {
      new (arr_ + i) T(value);
    }
    if (n < size_) {
      size_ = n;
    }
  }

  template<typename... Args>
  void emplace_back(Args&&... args)
  {
    if (cap_ == size_) {
      try {
        size_ != 0 ? reserve(2 * size_) : reserve(1);
      } catch (...) {
        // std::cerr << e.what() << '\n';
        throw;
      }
    }
    Alt::construct(alloc, arr_ + size_, std::forward<Args>(args)...);
    ++size_;
  }

  void push_back(const T& val)
  {
    if (cap_ == size_) {
      try {
        reserve(2 * size_);
      } catch (...) {
        // std::cerr << e.what() << '\n';
        throw;
      }
    }
    // new (arr_ + size_) T(val); -> alloctraits.constuct
    new (arr_ + size_) T(val);
    ++size_;
  }

  void push_back(T&& val)
  {
    if (cap_ == size_) {
      try {
        reserve(2 * size_);
        // arr_[];
      } catch (const std::exception& e) {
        // std::cerr << e.what() << '\n';
        throw;
      }
    }
    // new (arr_ + size_) T(val); -> alloctraits.constuct
    new (arr_ + size_) T(std::move(val));
    ++size_;
  }

  void pop_back()
  {
    --size_;
    (arr_ + size_)->~T();
  }

  T& operator[](size_t i) { return arr_[i]; }

  // const T& operator[](size_t i) { return arr_[i]; }

  T& at(size_t i)
  {
    if (i >= size_) {
      throw std::out_of_range("Out of range");
    }
    return arr_[i];
  }

  // operator =

  iterator begin() { return iterator(&arr_[0]); }

  // assign
  // front
  // back

  // empty

  // capacity

  void shrink_to_fit(); // деаллоцировать хвост О(1)

  // incert за линию

  // erase // удаление со сдвигом

  void swap(Vector& other)
  {
    T t = std::move(other);
    other = std::move(*this);
    *this = std::move(t);
  }
  // T* iterator();
};
// template <typename T, typename Alloc = std::allocator<T>>
// Vector<T, Alloc>::iterator operator+(int n, const Vector<T, Alloc>::iterator
// iter) {
//   return iter + n;
// }

#endif