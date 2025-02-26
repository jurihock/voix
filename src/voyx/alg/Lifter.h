#pragma once

#include <voyx/Header.h>
#include <voyx/alg/FFT.h>

template<typename T>
class Lifter
{

public:

  Lifter(const double quefrency, const double samplerate, const size_t framesize) :
    samplerate(samplerate),
    quefrency(static_cast<size_t>(quefrency * samplerate)),
    fft(framesize),
    spectrum(fft.dftsize()),
    cepstrum(fft.framesize())
  {
  }

  std::vector<T> lowpass(const voyx::vector<T>& dft)
  {
    std::vector<T> envelope(dft.size());
    lowpass(dft, envelope);
    return envelope;
  }

  void lowpass(const voyx::vector<T> dft, voyx::vector<T> envelope)
  {
    voyxassert(dft.size() == envelope.size());

    for (size_t i = 1; i < dft.size() - 1; ++i)
    {
      const T value = dft[i];

      spectrum[i] = value ? std::log10(value) : -12;
    }

    fft.ifft(spectrum, cepstrum);
    lowpass(cepstrum, quefrency);
    fft.fft(cepstrum, spectrum);

    for (size_t i = 1; i < dft.size() - 1; ++i)
    {
      envelope[i] = std::pow(T(10), spectrum[i].real());
    }
  }

  template<typename value_getter_t>
  std::vector<T> lowpass(const voyx::vector<std::complex<T>> dft)
  {
    std::vector<T> envelope(dft.size());
    lowpass<value_getter_t>(dft, envelope);
    return envelope;
  }

  template<typename value_getter_t>
  void lowpass(const voyx::vector<std::complex<T>> dft, voyx::vector<T> envelope)
  {
    const value_getter_t getvalue;

    voyxassert(dft.size() == envelope.size());

    for (size_t i = 1; i < dft.size() - 1; ++i)
    {
      const T value = getvalue(dft[i]);

      spectrum[i] = value ? std::log10(value) : -12;
    }

    fft.ifft(spectrum, cepstrum);
    lowpass(cepstrum, quefrency);
    fft.fft(cepstrum, spectrum);

    for (size_t i = 1; i < dft.size() - 1; ++i)
    {
      envelope[i] = std::pow(T(10), spectrum[i].real());
    }
  }

  template<typename value_getter_t>
  void lowpass(const voyx::vector<std::complex<T>> dft, voyx::vector<T> envelope, voyx::vector<T> logspectrum, voyx::vector<T> logcepstrum)
  {
    const value_getter_t getvalue;

    voyxassert(dft.size() == envelope.size());
    voyxassert(logcepstrum.size() == cepstrum.size());

    for (size_t i = 1; i < dft.size() - 1; ++i)
    {
      const T value = getvalue(dft[i]);

      spectrum[i] = value ? std::log10(value) : -12;

      logspectrum[i] = spectrum[i].real();
    }

    fft.ifft(spectrum, cepstrum);

    logcepstrum = cepstrum;
    lowpass(cepstrum, quefrency);

    fft.fft(cepstrum, spectrum);

    for (size_t i = 1; i < dft.size() - 1; ++i)
    {
      envelope[i] = std::pow(T(10), spectrum[i].real());
    }
  }

  template<typename value_getter_setter_t>
  void divide(voyx::vector<std::complex<T>> dft, const voyx::vector<T> envelope) const
  {
    const value_getter_setter_t value;

    for (size_t i = 0; i < dft.size(); ++i)
    {
      const bool ok = std::isnormal(envelope[i]);

      value(dft[i], ok ? value(dft[i]) / envelope[i] : 0);
    }
  }

  template<typename value_getter_setter_t>
  void multiply(voyx::vector<std::complex<T>> dft, const voyx::vector<T> envelope) const
  {
    const value_getter_setter_t value;

    for (size_t i = 0; i < dft.size(); ++i)
    {
      const bool ok = std::isnormal(envelope[i]);

      value(dft[i], ok ? value(dft[i]) * envelope[i] : 0);
    }
  }

private:

  const double samplerate;
  const size_t quefrency;

  const FFT<T> fft;

  std::vector<std::complex<T>> spectrum;
  std::vector<T> cepstrum;

  static void lowpass(std::span<T> cepstrum, const size_t quefrency)
  {
    for (size_t i = 1; i < std::min(quefrency, cepstrum.size()); ++i)
    {
      cepstrum[i] *= 2;
    }

    for (size_t i = quefrency + 1; i < cepstrum.size(); ++i)
    {
      cepstrum[i] = 0;
    }
  }

};
