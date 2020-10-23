// Copyright 2017 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef STARBOARD_ANDROID_SHARED_PLAYER_COMPONENTS_FACTORY_H_
#define STARBOARD_ANDROID_SHARED_PLAYER_COMPONENTS_FACTORY_H_

#include <string>

#include "starboard/android/shared/audio_decoder.h"
#include "starboard/android/shared/video_decoder.h"
#include "starboard/android/shared/video_render_algorithm.h"
#include "starboard/common/log.h"
#include "starboard/common/ref_counted.h"
#include "starboard/common/scoped_ptr.h"
#include "starboard/media.h"
#include "starboard/shared/opus/opus_audio_decoder.h"
#include "starboard/shared/starboard/player/filter/adaptive_audio_decoder_internal.h"
#include "starboard/shared/starboard/player/filter/audio_decoder_internal.h"
#include "starboard/shared/starboard/player/filter/audio_renderer_sink.h"
#include "starboard/shared/starboard/player/filter/audio_renderer_sink_impl.h"
#include "starboard/shared/starboard/player/filter/player_components.h"
#include "starboard/shared/starboard/player/filter/video_decoder_internal.h"
#include "starboard/shared/starboard/player/filter/video_render_algorithm.h"
#include "starboard/shared/starboard/player/filter/video_render_algorithm_impl.h"
#include "starboard/shared/starboard/player/filter/video_renderer_sink.h"

namespace starboard {
namespace android {
namespace shared {

class PlayerComponentsFactory : public starboard::shared::starboard::player::
                                    filter::PlayerComponents::Factory {
  typedef starboard::shared::opus::OpusAudioDecoder OpusAudioDecoder;
  typedef starboard::shared::starboard::player::filter::AdaptiveAudioDecoder
      AdaptiveAudioDecoder;
  typedef starboard::shared::starboard::player::filter::AudioDecoder
      AudioDecoderBase;
  typedef starboard::shared::starboard::player::filter::AudioRendererSink
      AudioRendererSink;
  typedef starboard::shared::starboard::player::filter::AudioRendererSinkImpl
      AudioRendererSinkImpl;
  typedef starboard::shared::starboard::player::filter::VideoDecoder
      VideoDecoderBase;
  typedef starboard::shared::starboard::player::filter::VideoRenderAlgorithm
      VideoRenderAlgorithmBase;
  typedef starboard::shared::starboard::player::filter::VideoRendererSink
      VideoRendererSink;

  const int kAudioSinkFramesAlignment = 256;
  const int kDefaultAudioSinkMinFramesPerAppend = 1024;
  const int kDefaultAudioSinkMaxCachedFrames =
      8 * kDefaultAudioSinkMinFramesPerAppend;

  virtual SbDrmSystem GetExtendedDrmSystem(SbDrmSystem drm_system) {
    return drm_system;
  }

  static int AlignUp(int value, int alignment) {
    return (value + alignment - 1) / alignment * alignment;
  }

  bool CreateSubComponents(
      const CreationParameters& creation_parameters,
      scoped_ptr<AudioDecoderBase>* audio_decoder,
      scoped_ptr<AudioRendererSink>* audio_renderer_sink,
      scoped_ptr<VideoDecoderBase>* video_decoder,
      scoped_ptr<VideoRenderAlgorithmBase>* video_render_algorithm,
      scoped_refptr<VideoRendererSink>* video_renderer_sink,
      std::string* error_message) override {
    SB_DCHECK(error_message);

    if (creation_parameters.audio_codec() != kSbMediaAudioCodecNone) {
      SB_DCHECK(audio_decoder);
      SB_DCHECK(audio_renderer_sink);

      auto decoder_creator = [](const SbMediaAudioSampleInfo& audio_sample_info,
                                SbDrmSystem drm_system) {
        if (audio_sample_info.codec == kSbMediaAudioCodecAac) {
          scoped_ptr<AudioDecoder> audio_decoder_impl(new AudioDecoder(
              audio_sample_info.codec, audio_sample_info, drm_system));
          if (audio_decoder_impl->is_valid()) {
            return audio_decoder_impl.PassAs<AudioDecoderBase>();
          }
        } else if (audio_sample_info.codec == kSbMediaAudioCodecOpus) {
          scoped_ptr<OpusAudioDecoder> audio_decoder_impl(
              new OpusAudioDecoder(audio_sample_info));
          if (audio_decoder_impl->is_valid()) {
            return audio_decoder_impl.PassAs<AudioDecoderBase>();
          }
        } else {
          SB_NOTREACHED();
        }
        return scoped_ptr<AudioDecoderBase>();
      };

      audio_decoder->reset(new AdaptiveAudioDecoder(
          creation_parameters.audio_sample_info(),
          GetExtendedDrmSystem(creation_parameters.drm_system()),
          decoder_creator));
      audio_renderer_sink->reset(new AudioRendererSinkImpl);
    }

    if (creation_parameters.video_codec() != kSbMediaVideoCodecNone) {
      SB_DCHECK(video_decoder);
      SB_DCHECK(video_render_algorithm);
      SB_DCHECK(video_renderer_sink);
      SB_DCHECK(error_message);

      scoped_ptr<VideoDecoder> video_decoder_impl(new VideoDecoder(
          creation_parameters.video_codec(),
          GetExtendedDrmSystem(creation_parameters.drm_system()),
          creation_parameters.output_mode(),
          creation_parameters.decode_target_graphics_context_provider(),
          creation_parameters.max_video_capabilities(), error_message));
      if (video_decoder_impl->is_valid()) {
        video_render_algorithm->reset(
            new VideoRenderAlgorithm(video_decoder_impl.get()));
        *video_renderer_sink = video_decoder_impl->GetSink();
        video_decoder->reset(video_decoder_impl.release());
      } else {
        video_decoder->reset();
        *video_renderer_sink = NULL;
        *error_message =
            "Failed to create video decoder with error: " + *error_message;
        return false;
      }
    }

    return true;
  }

  void GetAudioRendererParams(const CreationParameters& creation_parameters,
                              int* max_cached_frames,
                              int* min_frames_per_append) const override {
    SB_DCHECK(max_cached_frames);
    SB_DCHECK(min_frames_per_append);
    SB_DCHECK(kDefaultAudioSinkMinFramesPerAppend % kAudioSinkFramesAlignment ==
              0);
    *min_frames_per_append = kDefaultAudioSinkMinFramesPerAppend;

    // AudioRenderer prefers to use kSbMediaAudioSampleTypeFloat32 and only uses
    // kSbMediaAudioSampleTypeInt16Deprecated when float32 is not supported.
    int min_frames_required = SbAudioSinkGetMinBufferSizeInFrames(
        creation_parameters.audio_sample_info().number_of_channels,
        SbAudioSinkIsAudioSampleTypeSupported(kSbMediaAudioSampleTypeFloat32)
            ? kSbMediaAudioSampleTypeFloat32
            : kSbMediaAudioSampleTypeInt16Deprecated,
        creation_parameters.audio_sample_info().samples_per_second);
    // On Android 5.0, the size of audio renderer sink buffer need to be two
    // times larger than AudioTrack minBufferSize. Otherwise, AudioTrack may
    // stop working after pause.
    *max_cached_frames =
        min_frames_required * 2 + kDefaultAudioSinkMinFramesPerAppend;
    *max_cached_frames = AlignUp(*max_cached_frames, kAudioSinkFramesAlignment);
  }
};

}  // namespace shared
}  // namespace android
}  // namespace starboard

#endif  // STARBOARD_ANDROID_SHARED_PLAYER_COMPONENTS_FACTORY_H_