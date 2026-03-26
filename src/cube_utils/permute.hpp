#include <span>

template<typename d, typename idx_t, std::size_t size>
void permute(std::span<d, size> original,
             const std::span<idx_t, size> permutation) {
    std::array<d, size> result;
    for(size_t i = 0; i < size; i++) {
        result[permutation[i]] = original[i];
    }
    for(size_t i = 0; i < size; i++) {
        original[i] = result[i];
    }
}
