
#ifndef MEM_UTILS
#define MEM_UTILS

#include "graph_types.hpp"
#include <cstddef>
// #include <new>

namespace graphlib {

/**
 * @brief Get nearest multilpe of base greater or equal size
 */
inline offset_t paddToMultipleOf(offset_t size, offset_t base) {
  return ((size + base - 1) / base) * base;
}

/**
 * @brief Get pointer to an object of type T in buffer base_ptr at given offset
 * @warning Access within bounds of buffer pointed by base_ptr is user responsibility
 */
template <typename T>
T* offset_to_ptr(std::byte * base_ptr, offset_t offset) {
  return reinterpret_cast<T *>(base_ptr + offset);
}

// template <typename T, std::size_t Alignment = 64> struct AlignedAllocator {
//   using value_type = T;

//   AlignedAllocator() noexcept = default;
//   template <typename U>
//   AlignedAllocator(const AlignedAllocator<U, Alignment> &) noexcept {}

//   T *allocate(std::size_t n) {
//     return static_cast<T *>(
//         ::operator new[](n * sizeof(T), std::align_val_t{Alignment}));
//   }

//   void deallocate(T *p, std::size_t n) noexcept {
//     ::operator delete[](p, n * sizeof(T), std::align_val_t{Alignment});
//   }

//   template <typename U> struct rebind {
//     using other = AlignedAllocator<U, Alignment>;
//   };
// };


} // namespace graphlib

#endif