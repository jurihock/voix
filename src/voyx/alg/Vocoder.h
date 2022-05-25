#pragma once

#include <voyx/Header.h>

template<typename T>
class Vocoder
{

public:

  Vocoder(const voyx_t samplerate, const size_t framesize, const size_t hopsize) :
    framesize(framesize),
    hopsize(hopsize),
    dftsize(framesize / 2),
    freqinc(samplerate / framesize),
    phaseinc(pi * hopsize / framesize)
  {
    cursor.encode = 0;
    cursor.decode = 0;

    buffer.encode.resize(dftsize + 1);
    buffer.decode.resize(dftsize + 1);
  }

  void encode(voyx::matrix<std::complex<T>> dfts)
  {
    for (auto dft : dfts)
    {
      encode(dft);
    }
  }

  void decode(voyx::matrix<std::complex<T>> dfts)
  {
    for (auto dft : dfts)
    {
      decode(dft);
    }
  }

  void encode(voyx::vector<std::complex<T>> dft)
  {
    T frequency,
      phase,
      delta,
      j;

    for (size_t i = 0; i < dft.size(); ++i)
    {
      phase = std::arg(dft[i]);

      delta = phase - std::exchange(buffer.encode[i], phase);

      j = wrap(delta - i * phaseinc) / phaseinc;

      frequency = (i + j) * freqinc;

      dft[i] = std::complex<T>(std::abs(dft[i]), frequency);
    }

    if ((cursor.encode += hopsize) >= dftsize)
    {
      cursor.encode = 0;

      std::fill(buffer.encode.begin(), buffer.encode.end(), 0);
    }
  }

  void decode(voyx::vector<std::complex<T>> dft)
  {
    T frequency,
      phase,
      delta,
      j;

    for (size_t i = 0; i < dft.size(); ++i)
    {
      frequency = dft[i].imag();

      j = (frequency - i * freqinc) / freqinc;

      delta = (i + j) * phaseinc;

      phase = buffer.decode[i] += delta;

      dft[i] = std::polar<T>(dft[i].real(), phase);
    }

    if ((cursor.decode += hopsize) >= dftsize)
    {
      cursor.decode = 0;

      std::fill(buffer.decode.begin(), buffer.decode.end(), 0);
    }
  }

private:

  const T pi = T(2) * std::acos(T(-1));

  const size_t framesize;
  const size_t hopsize;
  const size_t dftsize;

  const T freqinc;
  const T phaseinc;

  struct
  {
    size_t encode;
    size_t decode;
  }
  cursor;

  struct
  {
    std::vector<T> encode;
    std::vector<T> decode;
  }
  buffer;

  inline T wrap(const T phase) const
  {
    return phase - pi * std::floor(phase / pi + T(0.5));
  }

};