#include <iostream>
#include <bit>

int main()
{
  // C++ 20
  if constexpr (std::endian::native == std::endian::little) {
    std::cout << "little-endian\n";
  } else if constexpr (std::endian::native == std::endian::big) {
    std::cout << "big-endian\n";
  }
}
