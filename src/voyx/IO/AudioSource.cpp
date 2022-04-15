#include <voyx/IO/AudioSource.h>

#include <voyx/Voyx.h>

#include <optional>

AudioSource::AudioSource(const std::string& name, size_t samplerate, size_t framesize, size_t buffersize) :
  Source(samplerate, framesize, buffersize),
  audio_device_name(name),
  audio_frame_buffer(
    buffersize,
    [framesize](size_t index)
    {
      auto input = new InputFrame();
      input->index = index;
      input->frame.resize(framesize);
      input->timestamp = 0;
      input->status = 0;
      return input;
    },
    [](InputFrame* input)
    {
      delete input;
    })
{
}

void AudioSource::open()
{
  if (audio.isStreamRunning())
  {
    audio.stopStream();
  }

  if (audio.isStreamOpen())
  {
    audio.closeStream();
  }

  std::optional<uint32_t> id;

  if (audio_device_name.empty())
  {
    id = audio.getDefaultInputDevice();
  }
  else
  {
    const uint32_t devices = audio.getDeviceCount();

    for (uint32_t i = 0; i < devices; ++i)
    {
      const RtAudio::DeviceInfo device = audio.getDeviceInfo(i);

      if (!device.probed)
      {
        continue;
      }

      if (device.inputChannels < 1)
      {
        continue;
      }

      if (!$$::imatch(device.name, "*" + audio_device_name + "*"))
      {
        continue;
      }

      id = i;
      break;
    }
  }

  if (!id)
  {
    throw std::runtime_error(
      $("Audio source \"{0}\" not found!",
        audio_device_name));
  }

  RtAudio::StreamParameters stream_parameters;
  stream_parameters.deviceId = id.value();
  stream_parameters.nChannels = 1;
  stream_parameters.firstChannel = 0;

  const RtAudioFormat stream_format = RTAUDIO_FLOAT32;
  const uint32_t stream_samplerate = static_cast<uint32_t>(samplerate());
  uint32_t stream_framesize = static_cast<uint32_t>(framesize());

  audio.openStream(
    nullptr,
    &stream_parameters,
    stream_format,
    stream_samplerate,
    &stream_framesize,
    &AudioSource::callback,
    this);

  if (stream_framesize != framesize())
  {
    throw std::runtime_error(
      $("Unexpected audio source stream frame size {0} != {1}!",
        stream_framesize,
        framesize()));
  }
}

void AudioSource::close()
{
  if (audio.isStreamRunning())
  {
    audio.stopStream();
  }

  if (audio.isStreamOpen())
  {
    audio.closeStream();
  }
}

void AudioSource::start()
{
  if (!audio.isStreamOpen())
  {
    return;
  }

  if (audio.isStreamRunning())
  {
    audio.stopStream();
  }

  audio.startStream();
}

void AudioSource::stop()
{
  if (!audio.isStreamOpen())
  {
    return;
  }

  if (!audio.isStreamRunning())
  {
    return;
  }

  audio.stopStream();
}

bool AudioSource::read(std::function<void(const std::vector<float>& frame)> callback)
{
  const bool ok = audio_frame_buffer.read(timeout(), [&](InputFrame& input)
  {
    callback(input.frame);
  });

  if (!ok)
  {
    LOG(WARNING) << $("Audio source fifo underflow!");
  }

  return ok;
}

int AudioSource::callback(void* output_frame_data, void* input_frame_data, uint32_t framesize, double timestamp, RtAudioStreamStatus status, void* $this)
{
  auto& audio_frame_buffer = static_cast<AudioSource*>($this)->audio_frame_buffer;

  const auto ok = audio_frame_buffer.write([&](InputFrame& input)
  {
    if (framesize != input.frame.size())
    {
      LOG(WARNING) << $("Unexpected input frame size {0} != {1}!", framesize, input.frame.size());
    }

    const size_t size = std::min(input.frame.size(), static_cast<size_t>(framesize));
    const size_t bytes = size * sizeof(input.frame.front());

    std::memcpy(input.frame.data(), input_frame_data, bytes);
  });

  if (!ok)
  {
    LOG(WARNING) << $("Audio source fifo overflow!");
  }

  if (status)
  {
    LOG(WARNING) << $("Audio source stream status {0}!", status);
  }

  return 0;
}
