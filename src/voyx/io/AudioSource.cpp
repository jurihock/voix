#include <voyx/io/AudioSource.h>

#include <voyx/Source.h>

AudioSource::AudioSource(const std::string& name, double samplerate, size_t framesize, size_t buffersize) :
  Source(samplerate, framesize, buffersize),
  audio_device_name(name),
  audio_frame_buffer(
    buffersize,
    [framesize](size_t index)
    {
      auto input = new InputFrame();
      input->index = index;
      input->frame.resize(framesize);
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

  const uint32_t devices = audio.getDeviceCount();

  if (!devices)
  {
    throw std::runtime_error(
      "No audio sources available!");
  }

  std::optional<uint32_t> id;

  if (audio_device_name.empty())
  {
    id = audio.getDefaultInputDevice();
  }
  else
  {
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

      if (!$$::imatch(device.name, ".*" + audio_device_name + ".*"))
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

  const RtAudio::DeviceInfo device = audio.getDeviceInfo(id.value());

  RtAudio::StreamParameters stream_parameters;
  stream_parameters.deviceId = id.value();
  stream_parameters.nChannels = 1;
  stream_parameters.firstChannel = 0;

  const RtAudioFormat stream_format = (typeid(sample_t) == typeid(float)) ? RTAUDIO_FLOAT32 : RTAUDIO_FLOAT64;
  uint32_t stream_samplerate = device.preferredSampleRate;
  uint32_t stream_framesize = static_cast<uint32_t>(framesize());

  for (const uint32_t native_samplerate : device.sampleRates)
  {
    if (native_samplerate == samplerate())
    {
      stream_samplerate = native_samplerate;
      break;
    }
  }

  audio_samplerate_converter = { stream_samplerate, samplerate() };

  stream_framesize /= audio_samplerate_converter.quotient();

  if (stream_samplerate != samplerate())
  {
    LOG(INFO) << $("Opening audio source stream with sr={0} and fs={1}.",
                   stream_samplerate, stream_framesize);
  }

  audio.openStream(
    nullptr,
    &stream_parameters,
    stream_format,
    stream_samplerate,
    &stream_framesize,
    &AudioSource::callback,
    this,
    nullptr,
    &AudioSource::error);

  if (stream_framesize * audio_samplerate_converter.quotient() != framesize())
  {
    throw std::runtime_error(
      $("Unexpected audio source stream frame size {0} * {2} != {1}!",
        stream_framesize, framesize(), audio_samplerate_converter.quotient()));
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

bool AudioSource::read(const size_t index, std::function<void(const voyx::vector<sample_t> frame)> callback)
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
  auto& audio_samplerate_converter = static_cast<AudioSource*>($this)->audio_samplerate_converter;

  const auto ok = audio_frame_buffer.write([&](InputFrame& input)
  {
    if (framesize * audio_samplerate_converter.quotient() != input.frame.size())
    {
      LOG(WARNING) << $("Unexpected input frame size {0} * {2} != {1}!",
                        framesize, input.frame.size(), audio_samplerate_converter.quotient());
    }

    voyx::vector<sample_t> src = { static_cast<sample_t*>(input_frame_data), framesize };
    voyx::vector<sample_t> dst = { input.frame.data(), input.frame.size() };

    audio_samplerate_converter(src, dst);
  });

  if (!ok)
  {
    LOG(WARNING) << $("Audio source fifo overflow!");
  }

  if (status == RTAUDIO_INPUT_OVERFLOW)
  {
    LOG(WARNING) << $("Audio source stream overflow!");
  }
  else if (status)
  {
    LOG(WARNING) << $("Audio source stream status {0}!", status);
  }

  return 0;
}

void AudioSource::error(RtAudioError::Type type, const std::string& error)
{
  LOG(ERROR) << "Audio source stream error: " << error;
}
