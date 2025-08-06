#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum NtscRsChromaDemodulationFilter {
  ChromaDemodFilterBox,
  ChromaDemodFilterNotch,
  ChromaDemodFilterOneLineComb,
  ChromaDemodFilterTwoLineComb,
} NtscRsChromaDemodulationFilter;

typedef enum NtscRsChromaLowpass {
  ChromaLowpassNone,
  ChromaLowpassLight,
  ChromaLowpassFull,
} NtscRsChromaLowpass;

typedef enum NtscRsFilterType {
  FilterTypeConstantK,
  FilterTypeButterworth,
} NtscRsFilterType;

typedef enum NtscRsLumaLowpass {
  LumaLowpassNone,
  LumaLowpassBox,
  LumaLowpassNotch,
} NtscRsLumaLowpass;

typedef enum NtscRsPhaseShift {
  PhaseShiftDegrees0,
  PhaseShiftDegrees90,
  PhaseShiftDegrees180,
  PhaseShiftDegrees270,
} NtscRsPhaseShift;

typedef enum NtscRsPixelFormat {
  Rgbx8,
  Xrgb8,
  Bgrx8,
  Xbgr8,
  Rgb8,
  Bgr8,
  Rgbx16,
  Xrgb16,
  Bgrx16,
  Xbgr16,
  Rgb16,
  Bgr16,
  Rgbx16s,
  Xrgb16s,
  Bgrx16s,
  Xbgr16s,
  Rgb16s,
  Bgr16s,
  Rgbx32f,
  Xrgb32f,
  Bgrx32f,
  Xbgr32f,
  Rgb32f,
  Bgr32f,
} NtscRsPixelFormat;

typedef enum NtscRsTapeSpeed {
  TapeSpeedNONE,
  TapeSpeedSP,
  TapeSpeedLP,
  TapeSpeedEP,
} NtscRsTapeSpeed;

typedef enum NtscRsUseField {
  UseFieldAlternating,
  UseFieldUpper,
  UseFieldLower,
  UseFieldInterleavedUpper,
  UseFieldInterleavedLower,
  UseFieldBoth,
} NtscRsUseField;

typedef struct NtscRsHeadSwitchingSettings {
  uint32_t height;
  uint32_t offset;
  float horiz_shift;
  float mid_line_position;
  float mid_line_jitter;
  bool enable_mid_line;
} NtscRsHeadSwitchingSettings;

typedef struct NtscRsTrackingNoiseSettings {
  uint32_t height;
  float wave_intensity;
  float snow_intensity;
  float snow_anisotropy;
  float noise_intensity;
} NtscRsTrackingNoiseSettings;

typedef struct NtscRsFbmNoiseSettings {
  float frequency;
  float intensity;
  uint32_t detail;
} NtscRsFbmNoiseSettings;

typedef struct NtscRsRingingSettings {
  float frequency;
  float power;
  float intensity;
} NtscRsRingingSettings;

typedef struct NtscRsVHSSettings {
  enum NtscRsTapeSpeed tape_speed;
  float chroma_loss;
  float sharpen_intensity;
  float sharpen_frequency;
  float edge_wave_intensity;
  float edge_wave_speed;
  float edge_wave_frequency;
  int32_t edge_wave_detail;
  bool enable_sharpen;
  bool enable_edge_wave;
} NtscRsVHSSettings;

typedef struct NtscRsScaleSettings {
  float horizontal_scale;
  float vertical_scale;
  bool scale_with_video_size;
} NtscRsScaleSettings;

typedef struct NtscRsEffectParams {
  int32_t random_seed;
  enum NtscRsUseField use_field;
  enum NtscRsFilterType filter_type;
  enum NtscRsLumaLowpass input_luma_filter;
  enum NtscRsChromaLowpass chroma_lowpass_in;
  enum NtscRsChromaDemodulationFilter chroma_demodulation;
  float luma_smear;
  float composite_sharpening;
  enum NtscRsPhaseShift video_scanline_phase_shift;
  int32_t video_scanline_phase_shift_offset;
  struct NtscRsHeadSwitchingSettings head_switching;
  struct NtscRsTrackingNoiseSettings tracking_noise;
  struct NtscRsFbmNoiseSettings composite_noise;
  struct NtscRsRingingSettings ringing;
  struct NtscRsFbmNoiseSettings luma_noise;
  struct NtscRsFbmNoiseSettings chroma_noise;
  float snow_intensity;
  float snow_anisotropy;
  float chroma_phase_noise_intensity;
  float chroma_phase_error;
  float chroma_delay_horizontal;
  int32_t chroma_delay_vertical;
  struct NtscRsVHSSettings vhs_settings;
  bool chroma_vert_blend;
  enum NtscRsChromaLowpass chroma_lowpass_out;
  struct NtscRsScaleSettings scale;
  bool enable_head_switching;
  bool enable_tracking_noise;
  bool enable_composite_noise;
  bool enable_ringing;
  bool enable_luma_noise;
  bool enable_chroma_noise;
  bool enable_vhs;
} NtscRsEffectParams;

void ntscrs_default_effect_params(struct NtscRsEffectParams *params);

void ntscrs_apply_effect_to_buffer_rgbx8(struct NtscRsEffectParams params,
                                         uintptr_t dimension_x,
                                         uintptr_t dimension_y,
                                         uint8_t *input_frame,
                                         uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_xrgb8(struct NtscRsEffectParams params,
                                         uintptr_t dimension_x,
                                         uintptr_t dimension_y,
                                         uint8_t *input_frame,
                                         uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_bgrx8(struct NtscRsEffectParams params,
                                         uintptr_t dimension_x,
                                         uintptr_t dimension_y,
                                         uint8_t *input_frame,
                                         uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_xbgr8(struct NtscRsEffectParams params,
                                         uintptr_t dimension_x,
                                         uintptr_t dimension_y,
                                         uint8_t *input_frame,
                                         uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_rgbx16(struct NtscRsEffectParams params,
                                          uintptr_t dimension_x,
                                          uintptr_t dimension_y,
                                          uint8_t *input_frame,
                                          uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_xrgb16(struct NtscRsEffectParams params,
                                          uintptr_t dimension_x,
                                          uintptr_t dimension_y,
                                          uint8_t *input_frame,
                                          uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_bgrx16(struct NtscRsEffectParams params,
                                          uintptr_t dimension_x,
                                          uintptr_t dimension_y,
                                          uint8_t *input_frame,
                                          uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_xbgr16(struct NtscRsEffectParams params,
                                          uintptr_t dimension_x,
                                          uintptr_t dimension_y,
                                          uint8_t *input_frame,
                                          uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_rgbx16s(struct NtscRsEffectParams params,
                                           uintptr_t dimension_x,
                                           uintptr_t dimension_y,
                                           uint8_t *input_frame,
                                           uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_xrgb16s(struct NtscRsEffectParams params,
                                           uintptr_t dimension_x,
                                           uintptr_t dimension_y,
                                           uint8_t *input_frame,
                                           uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_bgrx16s(struct NtscRsEffectParams params,
                                           uintptr_t dimension_x,
                                           uintptr_t dimension_y,
                                           uint8_t *input_frame,
                                           uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_xbgr16s(struct NtscRsEffectParams params,
                                           uintptr_t dimension_x,
                                           uintptr_t dimension_y,
                                           uint8_t *input_frame,
                                           uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_rgbx32f(struct NtscRsEffectParams params,
                                           uintptr_t dimension_x,
                                           uintptr_t dimension_y,
                                           uint8_t *input_frame,
                                           uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_xrgb32f(struct NtscRsEffectParams params,
                                           uintptr_t dimension_x,
                                           uintptr_t dimension_y,
                                           uint8_t *input_frame,
                                           uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_bgrx32f(struct NtscRsEffectParams params,
                                           uintptr_t dimension_x,
                                           uintptr_t dimension_y,
                                           uint8_t *input_frame,
                                           uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_xbgr32f(struct NtscRsEffectParams params,
                                           uintptr_t dimension_x,
                                           uintptr_t dimension_y,
                                           uint8_t *input_frame,
                                           uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_rgb8(struct NtscRsEffectParams params,
                                        uintptr_t dimension_x,
                                        uintptr_t dimension_y,
                                        uint8_t *input_frame,
                                        uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_bgr8(struct NtscRsEffectParams params,
                                        uintptr_t dimension_x,
                                        uintptr_t dimension_y,
                                        uint8_t *input_frame,
                                        uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_rgb16(struct NtscRsEffectParams params,
                                         uintptr_t dimension_x,
                                         uintptr_t dimension_y,
                                         uint8_t *input_frame,
                                         uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_bgr16(struct NtscRsEffectParams params,
                                         uintptr_t dimension_x,
                                         uintptr_t dimension_y,
                                         uint8_t *input_frame,
                                         uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_rgb16s(struct NtscRsEffectParams params,
                                          uintptr_t dimension_x,
                                          uintptr_t dimension_y,
                                          uint8_t *input_frame,
                                          uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_bgr16s(struct NtscRsEffectParams params,
                                          uintptr_t dimension_x,
                                          uintptr_t dimension_y,
                                          uint8_t *input_frame,
                                          uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_rgb32f(struct NtscRsEffectParams params,
                                          uintptr_t dimension_x,
                                          uintptr_t dimension_y,
                                          uint8_t *input_frame,
                                          uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer_bgr32f(struct NtscRsEffectParams params,
                                          uintptr_t dimension_x,
                                          uintptr_t dimension_y,
                                          uint8_t *input_frame,
                                          uintptr_t frame_num);

void ntscrs_apply_effect_to_buffer(struct NtscRsEffectParams params,
                                   uintptr_t dimension_x,
                                   uintptr_t dimension_y,
                                   uint8_t *input_frame,
                                   enum NtscRsPixelFormat pix_fmt,
                                   uintptr_t frame_num);
