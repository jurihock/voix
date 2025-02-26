#include <voyx/dsp/StftPitchShiftPipeline.h>

#include <voyx/Source.h>

using namespace stftpitchshift;

StftPitchShiftPipeline::StftPitchShiftPipeline(const double samplerate, const size_t framesize, const size_t hopsize, const size_t dftsize,
                                               std::shared_ptr<Source<sample_t>> source, std::shared_ptr<Sink<sample_t>> sink,
                                               std::shared_ptr<MidiObserver> midi, std::shared_ptr<Plot> plot) :
  SyncPipeline<>(source, sink),
  samplerate(samplerate),
  framesize(std::make_tuple(dftsize * 2 - 2, framesize)),
  hopsize(hopsize),
  midi(midi),
  plot(plot)
{
  if (plot != nullptr)
  {
    plot->xmap(samplerate / 2);
    plot->xlim(0, 2e3);
    plot->ylim(-120, 0);
  }

  const size_t total_buffer_size =
    std::get<0>(this->framesize) +
    std::get<1>(this->framesize);

  buffer.input.resize(total_buffer_size);
  buffer.output.resize(total_buffer_size);

  stft = std::make_shared<STFT<double>>(this->framesize, hopsize);
  core = std::make_shared<StftPitchShiftCore<double>>(this->framesize, hopsize, samplerate);

  core->factors({ 1 });
  core->quefrency(0 * 1e-3);
  core->distortion(1);
  core->normalization(false);
}

void StftPitchShiftPipeline::operator()(const size_t index, const voyx::vector<sample_t> input, voyx::vector<sample_t> output)
{
  auto show = [&](std::span<std::complex<double>> dft)
  {
    if (plot != nullptr)
    {
      std::vector<double> abs(dft.size());

      for (size_t i = 0; i < dft.size(); ++i)
      {
        abs[i] = 20 * std::log10(std::abs(dft[i]));
      }

      plot->plot(abs);
    }
  };

  const auto analysis_window_size = std::get<0>(framesize);
  const auto synthesis_window_size = std::get<1>(framesize);

  std::copy(
    buffer.input.begin() + synthesis_window_size,
    buffer.input.end(),
    buffer.input.begin());

  std::copy(
    input.begin(),
    input.end(),
    buffer.input.begin() + analysis_window_size);

  size_t hop = 0;

  (*stft)(buffer.input, buffer.output, [&](std::span<std::complex<double>> dft)
  {
    if (!hop)
    {
      show(dft);
    }

    ++hop;

    core->shiftpitch(dft);
  });

  std::copy(
    buffer.output.begin() - synthesis_window_size + analysis_window_size,
    buffer.output.end() - synthesis_window_size,
    output.begin());

  std::copy(
    buffer.output.begin() + synthesis_window_size,
    buffer.output.end(),
    buffer.output.begin());

  std::fill(
    buffer.output.begin() + analysis_window_size,
    buffer.output.end(),
    0);
}
