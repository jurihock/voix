#pragma once

#include <voyx/Header.h>

namespace $$::dft
{
  template<typename T>
  static inline T bin(const T freq, const double samplerate, const size_t framesize)
  {
    static_assert(std::is_floating_point<T>::value);

    return static_cast<T>(
      freq * framesize / samplerate);
  }

  template<typename T>
  static inline T freq(const T bin, const double samplerate, const size_t framesize)
  {
    static_assert(std::is_floating_point<T>::value);

    return static_cast<T>(
      bin * samplerate / framesize);
  }

  template<typename T>
  static inline std::vector<T> bins(const std::vector<T>& freqs, const double samplerate, const size_t framesize)
  {
    static_assert(std::is_floating_point<T>::value);

    std::vector<T> values(freqs.size());
    std::transform(freqs.begin(), freqs.end(), values.begin(),
      [samplerate, framesize](T i) { return $$::dft::bin(i, samplerate, framesize); });

    return values;
  }

  template<typename T>
  static inline std::vector<T> bins(const size_t framesize)
  {
    static_assert(std::is_arithmetic<T>::value);

    std::vector<T> values(framesize / 2 + /* nyquist */ 1);
    std::iota(values.begin(), values.end(), 0);

    return values;
  }

  template<typename T>
  static inline std::vector<T> freqs(const double samplerate, const size_t framesize)
  {
    static_assert(std::is_floating_point<T>::value);

    std::vector<T> values(framesize / 2 + /* nyquist */ 1);
    std::iota(values.begin(), values.end(), 0);
    std::transform(values.begin(), values.end(), values.begin(),
      [samplerate, framesize](T i) { return $$::dft::freq(i, samplerate, framesize); });

    return values;
  }
}
