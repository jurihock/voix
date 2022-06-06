#include <voyx/io/SweepSource.h>

#include <voyx/Source.h>

SweepSource::SweepSource(std::pair<voyx_t, voyx_t> frequencies, voyx_t duration, voyx_t samplerate, size_t framesize, size_t buffersize) :
  SweepSource(1, frequencies, duration, samplerate, framesize, buffersize)
{
}

SweepSource::SweepSource(voyx_t amplitude, std::pair<voyx_t, voyx_t> frequencies, voyx_t duration, voyx_t samplerate, size_t framesize, size_t buffersize) :
  Source(samplerate, framesize, buffersize),
  amplitude(amplitude),
  frequencies(frequencies),
  duration(duration),
  osc(frequencies, duration, samplerate),
  frame(framesize)
{
}

bool SweepSource::read(const size_t index, std::function<void(const voyx::vector<voyx_t> frame)> callback)
{
  for (size_t i = 0; i < frame.size(); ++i)
  {
    frame[i] = amplitude * osc.sin();
  }

  callback(frame);

  return true;
}
