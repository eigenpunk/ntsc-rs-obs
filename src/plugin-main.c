/*
ntsc-rs-obs
Copyright (C) 2025 eigenpunk

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>

#include "plugin-support.h"
#include "plugin-props.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")


#include <ntscrs.h>

#define OUTPUT_WIDTH (fd->cx)
#define OUTPUT_HEIGHT (fd->cy)

struct ntscrs_filter_data {
    obs_source_t* context;

    gs_texrender_t *texrender;
    gs_stagesurf_t *stagesurf;
    uint8_t *framebuf;
    gs_texture_t *framebuf_tex;

    enum gs_color_space space;
    enum gs_color_format format;

    uint32_t cx, cy;

    bool frame_processed;

    NtscRsEffectParams ntsc;
    size_t frame;
    bool paused;
};

static const char* filter_getname(void* unused) {
    UNUSED_PARAMETER(unused);
    return "ntsc-rs";
}

static inline void make_textures(struct ntscrs_filter_data *fd, enum gs_color_format format) {
    if (!fd) return;

    if (!fd->texrender) {
        obs_enter_graphics();
        fd->texrender = gs_texrender_create(format, GS_ZS_NONE);
        obs_leave_graphics();
    }

    if (!fd->stagesurf) {
        obs_enter_graphics();
        fd->stagesurf = gs_stagesurface_create(OUTPUT_WIDTH, OUTPUT_HEIGHT, format);
        obs_leave_graphics();
    }

    if (!fd->framebuf) {
        const size_t stride = gs_get_format_bpp(format);
        fd->framebuf = bzalloc(stride * OUTPUT_WIDTH * OUTPUT_HEIGHT);
    }

    if (!fd->framebuf_tex) {
        obs_enter_graphics();
        fd->framebuf_tex = gs_texture_create(OUTPUT_WIDTH, OUTPUT_HEIGHT, format, 1, NULL, GS_DYNAMIC);
        obs_leave_graphics();
    }
}

static void* filter_create(obs_data_t* settings, obs_source_t* context) {
    struct ntscrs_filter_data *fd = bzalloc(sizeof(struct ntscrs_filter_data));
    fd->context = context;
    obs_source_update(context, settings);
    return fd;
}

static inline void free_textures(struct ntscrs_filter_data *fd) {
    if (!fd) return;

    if (fd->framebuf_tex) {
        obs_enter_graphics();
        gs_texture_destroy(fd->framebuf_tex);
        obs_leave_graphics();
        fd->framebuf_tex = NULL;
    }

    if (fd->framebuf) {
        bfree(fd->framebuf);
        fd->framebuf = NULL;
    }

    if (fd->stagesurf) {
        obs_enter_graphics();
        gs_stagesurface_destroy(fd->stagesurf);
        obs_leave_graphics();
        fd->stagesurf = NULL;
    }

    if (fd->texrender) {
        obs_enter_graphics();
        gs_texrender_destroy(fd->texrender);
        obs_leave_graphics();
        fd->texrender = NULL;
    }
}

static void filter_destroy(void* data) {
    struct ntscrs_filter_data *fd = data;
    if (fd) {
        free_textures(fd);
        bfree(fd);
    }
}

static void filter_tick(void *data, float t) {
    UNUSED_PARAMETER(t);
    struct ntscrs_filter_data *fd = data;

    fd->frame_processed = false;
}

static const char *
get_tech_name_and_multiplier(enum gs_color_space current_space,
                 enum gs_color_space source_space,
                 float *multiplier)
{
    const char *tech_name = "Draw";
    *multiplier = 1.f;

    switch (source_space) {
    case GS_CS_SRGB:
    case GS_CS_SRGB_16F:
        if (current_space == GS_CS_709_SCRGB) {
            tech_name = "DrawMultiply";
            *multiplier = obs_get_video_sdr_white_level() / 80.0f;
        }
        break;
    case GS_CS_709_EXTENDED:
        switch (current_space) {
        case GS_CS_SRGB:
        case GS_CS_SRGB_16F:
            tech_name = "DrawTonemap";
            break;
        case GS_CS_709_SCRGB:
            tech_name = "DrawMultiply";
            *multiplier = obs_get_video_sdr_white_level() / 80.0f;
            break;
        default:
            break;
        }
        break;
    case GS_CS_709_SCRGB:
        switch (current_space) {
        case GS_CS_SRGB:
        case GS_CS_SRGB_16F:
            tech_name = "DrawMultiplyTonemap";
            *multiplier = 80.0f / obs_get_video_sdr_white_level();
            break;
        case GS_CS_709_EXTENDED:
            tech_name = "DrawMultiply";
            *multiplier = 80.0f / obs_get_video_sdr_white_level();
            break;
        default:
            break;
        }
    }

    return tech_name;
}

static void draw_frame(struct ntscrs_filter_data *fd) {
    if (!fd->framebuf_tex) return;
    gs_texture_t *tex = fd->framebuf_tex;

    const enum gs_color_space current_space = gs_get_color_space();
    float multiplier;
    const char *technique = get_tech_name_and_multiplier(current_space, fd->space, &multiplier);

    gs_effect_t *default_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
    gs_effect_set_texture(gs_effect_get_param_by_name(default_effect, "image"), tex);
    gs_effect_set_float(gs_effect_get_param_by_name(default_effect, "multiplier"), 1.0);
    while (gs_effect_loop(default_effect, technique)) {
        gs_draw_sprite(tex, 0, OUTPUT_WIDTH, OUTPUT_HEIGHT);
    }
}

static void filter_render(void* data, gs_effect_t *effect) {
    UNUSED_PARAMETER(effect);
    struct ntscrs_filter_data *fd = data;

    // get/validate source target/parent
    obs_source_t *target, *parent;
    if ((target = obs_filter_get_target(fd->context)) == NULL) {
        obs_log(LOG_ERROR, "got NULL target from context");
        obs_source_skip_video_filter(fd->context);
        return;
    }
    if ((parent = obs_filter_get_parent(fd->context)) == NULL) {
        obs_log(LOG_ERROR, "got NULL parent from context");
        obs_source_skip_video_filter(fd->context);
        return;
    }

    // validate target dimensions
    uint32_t cx = obs_source_get_width(target),
             cy = obs_source_get_height(target);
    if (cx <= 0 || cy <= 0) {
        obs_log(LOG_ERROR, "target has invalid size %dx%d (one or more dims <= 0)", cx, cy);
        obs_source_skip_video_filter(fd->context);
        return;
    }

    const enum gs_color_space preferred_spaces[] = {
        GS_CS_SRGB,
        GS_CS_SRGB_16F,
        GS_CS_709_EXTENDED,
    };
    const enum gs_color_space space = obs_source_get_color_space(target, OBS_COUNTOF(preferred_spaces), preferred_spaces);
    const enum gs_color_format format = gs_get_format_from_space(space);
    const enum gs_color_format tr_fmt = fd->texrender ? gs_texrender_get_format(fd->texrender) : GS_UNKNOWN;

    if (cx != fd->cx || cy != fd->cy || tr_fmt != format) {
        fd->cx = cx;
        fd->cy = cy;
        fd->space = space;

        free_textures(fd);
        make_textures(fd, format);
        obs_log(LOG_INFO, "created/resized textures, size %ux%u", cx, cy);

        obs_source_skip_video_filter(fd->context);
        return;
    }
    if (fd->framebuf_tex == NULL || fd->framebuf == NULL || fd->texrender == NULL || fd->stagesurf == NULL) {
        obs_source_skip_video_filter(fd->context);
        return;
    }

    // early out if we've already handled this frame
    if (fd->frame_processed) {
        draw_frame(fd);
        return;
    }

    // render frame to texture using texrender
    gs_texrender_reset(fd->texrender);
    gs_blend_state_push();
    gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
    if (gs_texrender_begin_with_color_space(fd->texrender, fd->cx, fd->cy, fd->space)) {
        // reset framebuffer, projection
        struct vec4 clear_color;
        vec4_zero(&clear_color);
        clear_color.w = 1.0f;
        gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
        gs_ortho(0.0f, (float)fd->cx, 0.0f, (float)fd->cy, -100.0f, 100.0f);

        // render
        uint32_t target_flags = obs_source_get_output_flags(target);
        if (target == parent && (target_flags & OBS_SOURCE_CUSTOM_DRAW) == 0 && (target_flags & OBS_SOURCE_ASYNC) == 0) {
            obs_source_default_render(target);
        } else {
            obs_source_video_render(target);
        }
        gs_texrender_end(fd->texrender);
    }
    gs_blend_state_pop();

    {
        uint8_t *texdata;
        uint32_t linesize;

        // stage/map rendered texture, copy to CPU buffer
        gs_texture_t *rendered_tex = gs_texrender_get_texture(fd->texrender);
        gs_stage_texture(fd->stagesurf, rendered_tex);
        if (gs_stagesurface_map(fd->stagesurf, &texdata, &linesize)) {
            size_t h = gs_stagesurface_get_height(fd->stagesurf);
            memcpy(fd->framebuf, texdata, linesize * h);
            gs_stagesurface_unmap(fd->stagesurf);
        } else {
            obs_log(LOG_ERROR, "failed to map stage surface");
        }

        // map empty texture, apply effects pass to CPU buffer, then write back to texture
        if (gs_texture_map(fd->framebuf_tex, &texdata, &linesize)) {
            size_t h = gs_texture_get_height(fd->framebuf_tex);
            ntscrs_apply_effect_to_buffer(
                fd->ntsc,
                OUTPUT_WIDTH,
                OUTPUT_HEIGHT,
                fd->framebuf,
                format == GS_RGBA16F ? Rgbx16 : Rgbx8,
                fd->frame);
            memcpy(texdata, fd->framebuf, linesize * h);

            gs_texture_unmap(fd->framebuf_tex);
        } else {
            obs_log(LOG_ERROR, "failed to map render target");
        }
    }

    // use effect to draw texture
    draw_frame(fd);
    fd->frame_processed = true;

    if (!fd->paused) {
        fd->frame++;
    }
}

static obs_properties_t *filter_properties(void *data) {
    UNUSED_PARAMETER(data);

    obs_properties_t *props = obs_properties_create();
    obs_property_t *paused = obs_properties_add_bool(
        props, PROP_PAUSED, "Pause"
    );
    UNUSED_PARAMETER(paused);
    obs_property_t *random_seed = obs_properties_add_int(
        props, PROP_RANDOM_SEED, "Random seed", INT32_MIN, INT32_MAX, 1
    );
    UNUSED_PARAMETER(random_seed);
    obs_property_t *use_field = obs_properties_add_list(
        props, PROP_USE_FIELD, "Use field", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT
    );
    obs_property_list_add_int(use_field, "Alternating", UseFieldAlternating);
    obs_property_list_add_int(use_field, "Upper only", UseFieldUpper);
    obs_property_list_add_int(use_field, "Lower only", UseFieldLower);
    obs_property_list_add_int(use_field, "Interleaved (upper first)", UseFieldInterleavedUpper);
    obs_property_list_add_int(use_field, "Interleaved (lower first)", UseFieldInterleavedLower);
    obs_property_list_add_int(use_field, "Both", UseFieldBoth);

    obs_property_t *filter_type = obs_properties_add_list(
        props, PROP_FILTER_TYPE, "Lowpass filter type", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT
    );
    obs_property_list_add_int(filter_type, "Constant K (blurry)", FilterTypeConstantK);
    obs_property_list_add_int(filter_type, "Butterworth (sharper)", FilterTypeButterworth);

    obs_property_t *input_luma_filter = obs_properties_add_list(
        props, PROP_INPUT_LUMA_FILTER, "Input luma filter", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT
    );
    obs_property_list_add_int(input_luma_filter, "Notch", LumaLowpassNotch);
    obs_property_list_add_int(input_luma_filter, "Box", LumaLowpassBox);
    obs_property_list_add_int(input_luma_filter, "None", LumaLowpassNone);

    obs_property_t *chroma_lowpass_in = obs_properties_add_list(
        props, PROP_CHROMA_LOWPASS_IN, "Chroma low-pass in", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT
    );
    obs_property_list_add_int(chroma_lowpass_in, "Full", ChromaLowpassFull);
    obs_property_list_add_int(chroma_lowpass_in, "Light", ChromaLowpassLight);
    obs_property_list_add_int(chroma_lowpass_in, "None", ChromaLowpassNone);

    obs_property_t *composite_sharpening = obs_properties_add_float(
        props, PROP_COMPOSITE_SHARPENING, "Composite sharpening", -1.0, 2.0, 0.001
    );
    UNUSED_PARAMETER(composite_sharpening);

    obs_property_t *enable_composite_noise = obs_properties_add_bool(
        props, PROP_COMPOSITE_NOISE, "Enable composite noise"
    );
    obs_property_t *composite_noise_frequency = obs_properties_add_float(
        props, PROP_COMPOSITE_NOISE_FREQUENCY, "Composite noise: Frequency", 0.0, 10000.0, 0.001
    );
    obs_property_t *composite_noise_intensity = obs_properties_add_float(
        props, PROP_COMPOSITE_NOISE_INTENSITY, "Composite noise: Intensity", 0.0, 1.0, 0.001
    );
    obs_property_t *composite_noise_detail = obs_properties_add_int(
        props, PROP_COMPOSITE_NOISE_DETAIL, "Composite noise: Detail", 1, 5, 1
    );
    UNUSED_PARAMETER(enable_composite_noise);
    UNUSED_PARAMETER(composite_noise_frequency);
    UNUSED_PARAMETER(composite_noise_intensity);
    UNUSED_PARAMETER(composite_noise_detail);

    obs_property_t *snow_intensity = obs_properties_add_float(
        props, PROP_SNOW_INTENSITY, "Snow", 0, 100.0, 0.00001
    );
    obs_property_t *snow_anisotropy = obs_properties_add_float(
        props, PROP_SNOW_ANISOTROPY, "Snow anisotropy", 0, 1.0, 0.001
    );
    UNUSED_PARAMETER(snow_intensity);
    UNUSED_PARAMETER(snow_anisotropy);

    obs_property_t *video_scanline_phase_shift = obs_properties_add_list(
        props, PROP_VIDEO_SCANLINE_PHASE_SHIFT, "Scanline phase shift", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT
    );
    obs_property_list_add_int(video_scanline_phase_shift, "0 degrees", PhaseShiftDegrees0);
    obs_property_list_add_int(video_scanline_phase_shift, "90 degrees", PhaseShiftDegrees90);
    obs_property_list_add_int(video_scanline_phase_shift, "180 degrees", PhaseShiftDegrees180);
    obs_property_list_add_int(video_scanline_phase_shift, "270 degrees", PhaseShiftDegrees270);

    obs_property_t *video_scanline_phase_shift_offset = obs_properties_add_int(
        props, PROP_VIDEO_SCANLINE_PHASE_SHIFT_OFFSET, "Scanline phase shift offset", 0, 4, 1
    );
    UNUSED_PARAMETER(video_scanline_phase_shift_offset);

    obs_property_t *chroma_demodulation = obs_properties_add_list(
        props, PROP_CHROMA_DEMODULATION, "Chroma demodulation filter", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT
    );
    obs_property_list_add_int(chroma_demodulation, "Box", ChromaDemodFilterBox);
    obs_property_list_add_int(chroma_demodulation, "Notch", ChromaDemodFilterNotch);
    obs_property_list_add_int(chroma_demodulation, "1-line comb", ChromaDemodFilterOneLineComb);
    obs_property_list_add_int(chroma_demodulation, "2-line comb", ChromaDemodFilterTwoLineComb);

    obs_property_t *luma_smear = obs_properties_add_float(
        props, PROP_LUMA_SMEAR, "Luma smear", 0, 1, 0.001
    );
    UNUSED_PARAMETER(luma_smear);

    /*
    * HEAD SWITCHING
    */
    obs_property_t *enable_head_switching = obs_properties_add_bool(
        props, PROP_HEAD_SWITCHING, "Enable head switching"
    );
    obs_property_t *head_switching_height = obs_properties_add_int(
        props, PROP_HEAD_SWITCHING_HEIGHT, "Head switching: Height", 0, 24, 1
    );
    obs_property_t *head_switching_offset = obs_properties_add_int(
        props, PROP_HEAD_SWITCHING_OFFSET, "Head switching: Offset", 0, 24, 1
    );
    obs_property_t *head_switching_horizontal_shift = obs_properties_add_float(
        props, PROP_HEAD_SWITCHING_HORIZONTAL_SHIFT, "Head switching: Horizontal shift", -100.0, 100.0, 0.001
    );
    obs_property_t *head_switching_enable_mid_line = obs_properties_add_bool(
        props, PROP_HEAD_SWITCHING_START_MID_LINE, "Head switching: Start mid-line"
    );
    obs_property_t *head_switching_mid_line_position = obs_properties_add_float(
        props, PROP_HEAD_SWITCHING_MID_LINE_POSITION, "Head switching: Mid-line position", 0.0, 1.0, 0.001
    );
    obs_property_t *head_switching_mid_line_jitter = obs_properties_add_float(
        props, PROP_HEAD_SWITCHING_MID_LINE_JITTER, "Head switching: Mid-line jitter", 0.0, 1.0, 0.001
    );
    UNUSED_PARAMETER(enable_head_switching);
    UNUSED_PARAMETER(head_switching_height);
    UNUSED_PARAMETER(head_switching_offset);
    UNUSED_PARAMETER(head_switching_horizontal_shift);
    UNUSED_PARAMETER(head_switching_enable_mid_line);
    UNUSED_PARAMETER(head_switching_mid_line_position);
    UNUSED_PARAMETER(head_switching_mid_line_jitter);

    /*
    * TRACKING NOISE
    */
    obs_property_t *enable_tracking_noise = obs_properties_add_bool(
        props, PROP_TRACKING_NOISE, "Enable tracking noise"
    );
    obs_property_t *tracking_noise_height = obs_properties_add_int(
        props, PROP_TRACKING_NOISE_HEIGHT, "Tracking noise: Height", 0, 120, 1
    );
    obs_property_t *tracking_noise_wave_intensity = obs_properties_add_float(
        props, PROP_TRACKING_NOISE_WAVE_INTENSITY, "Tracking noise: Wave intensity", -50.0, 50.0, 0.001
    );
    obs_property_t *tracking_noise_snow_intensity = obs_properties_add_float(
        props, PROP_TRACKING_NOISE_SNOW_INTENSITY, "Tracking noise: Snow", 0.0, 1.0, 0.001
    );
    obs_property_t *tracking_noise_snow_anisotropy = obs_properties_add_float(
        props, PROP_TRACKING_NOISE_SNOW_ANISOTROPY, "Tracking noise: Snow anisotropy", 0.0, 1.0, 0.001
    );
    obs_property_t *tracking_noise_intensity = obs_properties_add_float(
        props, PROP_TRACKING_NOISE_NOISE_INTENSITY, "Tracking noise: Intensity", 0.0, 1.0, 0.001
    );
    UNUSED_PARAMETER(enable_tracking_noise);
    UNUSED_PARAMETER(tracking_noise_height);
    UNUSED_PARAMETER(tracking_noise_wave_intensity);
    UNUSED_PARAMETER(tracking_noise_snow_intensity);
    UNUSED_PARAMETER(tracking_noise_snow_anisotropy);
    UNUSED_PARAMETER(tracking_noise_intensity);

    /*
    * RINGING
    */
    obs_property_t *enable_ringing = obs_properties_add_bool(
        props, PROP_RINGING, "Enable ringing"
    );
    obs_property_t *ringing_frequency = obs_properties_add_float(
        props, PROP_RINGING_FREQUENCY, "Ringing: Frequency", 0.0, 1.0, 0.001
    );
    obs_property_t *ringing_power = obs_properties_add_float(
        props, PROP_RINGING_POWER, "Ringing: Power", 1.0, 10.0, 0.001
    );
    obs_property_t *ringing_intensity = obs_properties_add_float(
        props, PROP_RINGING_SCALE, "Ringing: Intensity", 0.0, 10.0, 0.001
    );
    UNUSED_PARAMETER(enable_ringing);
    UNUSED_PARAMETER(ringing_frequency);
    UNUSED_PARAMETER(ringing_power);
    UNUSED_PARAMETER(ringing_intensity);

    /*
    * LUMA NOISE
    */
    obs_property_t *enable_luma_noise = obs_properties_add_bool(
        props, PROP_LUMA_NOISE, "Enable luma noise"
    );
    obs_property_t *luma_noise_intensity = obs_properties_add_float(
        props, PROP_LUMA_NOISE_INTENSITY, "Luma noise: Intensity", 0.0, 1.0, 0.001
    );
    obs_property_t *luma_noise_frequency = obs_properties_add_float(
        props, PROP_LUMA_NOISE_FREQUENCY, "Luma noise: Frequency", 0.0, 1.0, 0.0001
    );
    obs_property_t *luma_noise_detail = obs_properties_add_int(
        props, PROP_LUMA_NOISE_DETAIL, "Luma noise: Detail", 1, 5, 1
    );
    UNUSED_PARAMETER(enable_luma_noise);
    UNUSED_PARAMETER(luma_noise_frequency);
    UNUSED_PARAMETER(luma_noise_intensity);
    UNUSED_PARAMETER(luma_noise_detail);

    /*
    * CHROMA NOISE
    */
    obs_property_t *enable_chroma_noise = obs_properties_add_bool(
        props, PROP_CHROMA_NOISE, "Enable chroma noise"
    );
    obs_property_t *chroma_noise_intensity = obs_properties_add_float(
        props, PROP_CHROMA_NOISE_INTENSITY, "Chroma noise: Intensity", 0.0, 1.0, 0.001
    );
    obs_property_t *chroma_noise_frequency = obs_properties_add_float(
        props, PROP_CHROMA_NOISE_FREQUENCY, "Chroma noise: Frequency", 0.0, 0.5, 0.001
    );
    obs_property_t *chroma_noise_detail = obs_properties_add_int(
        props, PROP_CHROMA_NOISE_DETAIL, "Chroma noise: Detail", 1, 5, 1
    );
    UNUSED_PARAMETER(enable_chroma_noise);
    UNUSED_PARAMETER(chroma_noise_frequency);
    UNUSED_PARAMETER(chroma_noise_intensity);
    UNUSED_PARAMETER(chroma_noise_detail);

    obs_property_t *chroma_phase_error = obs_properties_add_float(
        props, PROP_CHROMA_PHASE_ERROR, "Chroma phase error", 0.0, 1.0, 0.001
    );
    obs_property_t *chroma_phase_noise_intensity = obs_properties_add_float(
        props, PROP_CHROMA_PHASE_NOISE_INTENSITY, "Chroma phase noise", 0.0, 1.0, 0.001
    );
    obs_property_t *chroma_delay_horizontal = obs_properties_add_float(
        props, PROP_CHROMA_DELAY_HORIZONTAL, "Chroma delay (horizontal)", -40.0, 40.0, 0.001
    );
    obs_property_t *chroma_delay_vertical = obs_properties_add_int(
        props, PROP_CHROMA_DELAY_VERTICAL, "Chroma delay (vertical)", -20, 20, 1
    );
    UNUSED_PARAMETER(chroma_phase_error);
    UNUSED_PARAMETER(chroma_phase_noise_intensity);
    UNUSED_PARAMETER(chroma_delay_horizontal);
    UNUSED_PARAMETER(chroma_delay_vertical);

    obs_property_t *enable_vhs = obs_properties_add_bool(
        props, PROP_VHS_SETTINGS, "Enable VHS"
    );
    obs_property_t *vhs_tape_speed = obs_properties_add_list(
        props, PROP_VHS_TAPE_SPEED, "VHS: Tape speed", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT
    );
    obs_property_list_add_int(vhs_tape_speed, "SP (Standard Play)", TapeSpeedSP);
    obs_property_list_add_int(vhs_tape_speed, "LP (Long Play)", TapeSpeedLP);
    obs_property_list_add_int(vhs_tape_speed, "EP (Extended Play)", TapeSpeedEP);
    obs_property_list_add_int(vhs_tape_speed, "None", TapeSpeedNONE);
    obs_property_t *vhs_chroma_loss = obs_properties_add_float(
        props, PROP_VHS_CHROMA_LOSS, "VHS: Chroma loss", 0.0, 1.0, 0.000001
    );
    obs_property_t *vhs_enable_sharpen = obs_properties_add_bool(
        props, PROP_VHS_SHARPEN_ENABLED, "VHS: Sharpen"
    );
    obs_property_t *vhs_sharpen_intensity = obs_properties_add_float(
        props, PROP_VHS_SHARPEN_INTENSITY, "VHS: Sharpen: Intensity", 0.0, 5.0, 0.001
    );
    obs_property_t *vhs_sharpen_frequency = obs_properties_add_float(
        props, PROP_VHS_SHARPEN_FREQUENCY, "VHS: Sharpen: Frequency", 0.5, 4.0, 0.001
    );
    obs_property_t *vhs_enable_edge_wave = obs_properties_add_bool(
        props, PROP_VHS_EDGE_WAVE_ENABLED, "VHS: Edge wave"
    );
    obs_property_t *vhs_edge_wave_intensity = obs_properties_add_float(
        props, PROP_VHS_EDGE_WAVE_INTENSITY, "VHS: Edge wave: Intensity", 0.0, 20.0, 0.001
    );
    obs_property_t *vhs_edge_wave_speed = obs_properties_add_float(
        props, PROP_VHS_EDGE_WAVE_SPEED, "VHS: Edge wave: Speed", 0.0, 10.0, 0.001
    );
    obs_property_t *vhs_edge_wave_frequency = obs_properties_add_float(
        props, PROP_VHS_EDGE_WAVE_FREQUENCY, "VHS: Edge wave: Frequency", 0.0, 0.5, 0.001
    );
    obs_property_t *vhs_edge_wave_detail = obs_properties_add_int(
        props, PROP_VHS_EDGE_WAVE_DETAIL, "VHS: Edge wave: Detail", 0, 5, 1
    );
    UNUSED_PARAMETER(enable_vhs);
    UNUSED_PARAMETER(vhs_chroma_loss);
    UNUSED_PARAMETER(vhs_enable_sharpen);
    UNUSED_PARAMETER(vhs_sharpen_frequency);
    UNUSED_PARAMETER(vhs_sharpen_intensity);
    UNUSED_PARAMETER(vhs_enable_edge_wave);
    UNUSED_PARAMETER(vhs_edge_wave_frequency);
    UNUSED_PARAMETER(vhs_edge_wave_speed);
    UNUSED_PARAMETER(vhs_edge_wave_intensity);
    UNUSED_PARAMETER(vhs_edge_wave_detail);

    obs_property_t *chroma_vert_blend = obs_properties_add_bool(
        props, PROP_CHROMA_VERT_BLEND, "Vertically blend chroma"
    );
    obs_property_t *chroma_lowpass_out = obs_properties_add_list(
        props, PROP_CHROMA_LOWPASS_OUT, "Chroma low-pass out", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT
    );
    obs_property_list_add_int(chroma_lowpass_out, "Full", ChromaLowpassFull);
    obs_property_list_add_int(chroma_lowpass_out, "Light", ChromaLowpassLight);
    obs_property_list_add_int(chroma_lowpass_out, "None", ChromaLowpassNone);
    UNUSED_PARAMETER(chroma_vert_blend);

    obs_property_t *horizontal_scale = obs_properties_add_float(
        props, PROP_HORIZONTAL_SCALE, "Horizontal scale", 0.125, 8.0, 0.001
    );
    obs_property_t *vertical_scale = obs_properties_add_float(
        props, PROP_VERTICAL_SCALE, "Vertical scale", 0.125, 8.0, 0.001
    );
    obs_property_t *scale_with_video_size = obs_properties_add_bool(
        props, PROP_SCALE_WITH_VIDEO_SIZE, "Scale with video size"
    );
    UNUSED_PARAMETER(horizontal_scale);
    UNUSED_PARAMETER(vertical_scale);
    UNUSED_PARAMETER(scale_with_video_size);

    return props;
}

static uint32_t filter_get_height(void *data) {
    struct ntscrs_filter_data *const fd = data;
    return OUTPUT_HEIGHT;
}

static uint32_t filter_get_width(void *data) {
    struct ntscrs_filter_data *fd = data;
    return OUTPUT_WIDTH;
}

void filter_get_defaults(void *data, obs_data_t *settings)
{
    UNUSED_PARAMETER(data);
    struct NtscRsEffectParams p = {0};
    ntscrs_default_effect_params(&p);

    obs_data_t *s = settings;
    obs_data_set_default_bool(s, PROP_HEAD_SWITCHING, p.enable_head_switching);
    obs_data_set_default_bool(s, PROP_TRACKING_NOISE, p.enable_tracking_noise);
    obs_data_set_default_bool(s, PROP_COMPOSITE_NOISE, p.enable_composite_noise);
    obs_data_set_default_bool(s, PROP_RINGING, p.enable_ringing);
    obs_data_set_default_bool(s, PROP_LUMA_NOISE, p.enable_luma_noise);
    obs_data_set_default_bool(s, PROP_CHROMA_NOISE, p.enable_chroma_noise);
    obs_data_set_default_bool(s, PROP_VHS_SETTINGS, p.enable_vhs);

    obs_data_set_default_int(s, PROP_RANDOM_SEED, p.random_seed);
    obs_data_set_default_int(s, PROP_USE_FIELD, p.use_field);
    obs_data_set_default_int(s, PROP_FILTER_TYPE, p.filter_type);
    obs_data_set_default_int(s, PROP_INPUT_LUMA_FILTER, p.input_luma_filter);
    obs_data_set_default_int(s, PROP_CHROMA_LOWPASS_IN, p.chroma_lowpass_in);
    obs_data_set_default_int(s, PROP_CHROMA_DEMODULATION, p.chroma_demodulation);
    obs_data_set_default_double(s, PROP_LUMA_SMEAR, p.luma_smear);
    obs_data_set_default_double(s, PROP_COMPOSITE_SHARPENING, p.composite_sharpening);
    obs_data_set_default_int(s, PROP_VIDEO_SCANLINE_PHASE_SHIFT, p.video_scanline_phase_shift);
    obs_data_set_default_int(s, PROP_VIDEO_SCANLINE_PHASE_SHIFT_OFFSET, p.video_scanline_phase_shift_offset);
    obs_data_set_default_bool(s, PROP_HEAD_SWITCHING_START_MID_LINE, p.head_switching.enable_mid_line);
    obs_data_set_default_int(s, PROP_HEAD_SWITCHING_HEIGHT, p.head_switching.height);
    obs_data_set_default_int(s, PROP_HEAD_SWITCHING_OFFSET, p.head_switching.offset);
    obs_data_set_default_double(s, PROP_HEAD_SWITCHING_HORIZONTAL_SHIFT, p.head_switching.horiz_shift);
    obs_data_set_default_double(s, PROP_HEAD_SWITCHING_MID_LINE_POSITION, p.head_switching.mid_line_position);
    obs_data_set_default_double(s, PROP_HEAD_SWITCHING_MID_LINE_JITTER, p.head_switching.mid_line_jitter);
    obs_data_set_default_int(s, PROP_TRACKING_NOISE_HEIGHT, p.tracking_noise.height);
    obs_data_set_default_double(s, PROP_TRACKING_NOISE_WAVE_INTENSITY, p.tracking_noise.wave_intensity);
    obs_data_set_default_double(s, PROP_TRACKING_NOISE_SNOW_INTENSITY, p.tracking_noise.snow_intensity);
    obs_data_set_default_double(s, PROP_TRACKING_NOISE_SNOW_ANISOTROPY, p.tracking_noise.snow_anisotropy);
    obs_data_set_default_double(s, PROP_TRACKING_NOISE_NOISE_INTENSITY, p.tracking_noise.noise_intensity);
    obs_data_set_default_double(s, PROP_COMPOSITE_NOISE_FREQUENCY, p.composite_noise.frequency);
    obs_data_set_default_double(s, PROP_COMPOSITE_NOISE_INTENSITY, p.composite_noise.intensity);
    obs_data_set_default_int(s, PROP_COMPOSITE_NOISE_DETAIL, p.composite_noise.detail);
    obs_data_set_default_double(s, PROP_RINGING_FREQUENCY, p.ringing.frequency);
    obs_data_set_default_double(s, PROP_RINGING_POWER, p.ringing.power);
    obs_data_set_default_double(s, PROP_RINGING_SCALE, p.ringing.intensity);
    obs_data_set_default_double(s, PROP_LUMA_NOISE_FREQUENCY, p.luma_noise.frequency);
    obs_data_set_default_double(s, PROP_LUMA_NOISE_INTENSITY, p.luma_noise.intensity);
    obs_data_set_default_int(s, PROP_LUMA_NOISE_DETAIL, p.luma_noise.detail);
    obs_data_set_default_double(s, PROP_CHROMA_NOISE_FREQUENCY, p.chroma_noise.frequency);
    obs_data_set_default_double(s, PROP_CHROMA_NOISE_INTENSITY, p.chroma_noise.intensity);
    obs_data_set_default_int(s, PROP_CHROMA_NOISE_DETAIL, p.chroma_noise.detail);
    obs_data_set_default_double(s, PROP_SNOW_ANISOTROPY, p.snow_anisotropy);
    obs_data_set_default_double(s, PROP_SNOW_INTENSITY, p.snow_intensity);
    obs_data_set_default_double(s, PROP_CHROMA_PHASE_NOISE_INTENSITY, p.chroma_phase_noise_intensity);
    obs_data_set_default_double(s, PROP_CHROMA_PHASE_ERROR, p.chroma_phase_error);
    obs_data_set_default_double(s, PROP_CHROMA_DELAY_HORIZONTAL, p.chroma_delay_horizontal);
    obs_data_set_default_int(s, PROP_CHROMA_DELAY_VERTICAL, p.chroma_delay_vertical);
    obs_data_set_default_double(s, PROP_VHS_SHARPEN_INTENSITY, p.vhs_settings.sharpen_intensity);
    obs_data_set_default_double(s, PROP_VHS_SHARPEN_FREQUENCY, p.vhs_settings.sharpen_frequency);
    obs_data_set_default_double(s, PROP_VHS_EDGE_WAVE_INTENSITY, p.vhs_settings.edge_wave_intensity);
    obs_data_set_default_double(s, PROP_VHS_EDGE_WAVE_FREQUENCY, p.vhs_settings.edge_wave_frequency);
    obs_data_set_default_bool(s, PROP_VHS_EDGE_WAVE_ENABLED, p.vhs_settings.enable_edge_wave);
    obs_data_set_default_int(s, PROP_VHS_EDGE_WAVE_DETAIL, p.vhs_settings.edge_wave_detail);
    obs_data_set_default_double(s, PROP_VHS_EDGE_WAVE_SPEED, p.vhs_settings.edge_wave_speed);
    obs_data_set_default_bool(s, PROP_VHS_SHARPEN_ENABLED, p.vhs_settings.enable_sharpen);
    obs_data_set_default_double(s, PROP_VHS_CHROMA_LOSS, p.vhs_settings.chroma_loss);
    obs_data_set_default_int(s, PROP_VHS_TAPE_SPEED, p.vhs_settings.tape_speed);
    obs_data_set_default_bool(s, PROP_CHROMA_VERT_BLEND, p.chroma_vert_blend);
    obs_data_set_default_int(s, PROP_CHROMA_LOWPASS_OUT, p.chroma_lowpass_out);
    obs_data_set_default_double(s, PROP_HORIZONTAL_SCALE, p.scale.horizontal_scale);
    obs_data_set_default_double(s, PROP_VERTICAL_SCALE, p.scale.vertical_scale);
    obs_data_set_default_bool(s, PROP_SCALE_WITH_VIDEO_SIZE, p.scale.scale_with_video_size);

    obs_data_set_default_bool(s, PROP_PAUSED, false);
}

static void filter_update(void *data, obs_data_t *s) {
    struct ntscrs_filter_data *fd = data;
    struct NtscRsEffectParams *p = &fd->ntsc;

    p->enable_head_switching = obs_data_get_bool(s, PROP_HEAD_SWITCHING);
    p->enable_tracking_noise = obs_data_get_bool(s, PROP_TRACKING_NOISE);
    p->enable_composite_noise = obs_data_get_bool(s, PROP_COMPOSITE_NOISE);
    p->enable_ringing = obs_data_get_bool(s, PROP_RINGING);
    p->enable_luma_noise = obs_data_get_bool(s, PROP_LUMA_NOISE);
    p->enable_chroma_noise = obs_data_get_bool(s, PROP_CHROMA_NOISE);
    p->enable_vhs = obs_data_get_bool(s, PROP_VHS_SETTINGS);

    p->random_seed = obs_data_get_int(s, PROP_RANDOM_SEED);
    p->use_field = obs_data_get_int(s, PROP_USE_FIELD);
    p->filter_type = obs_data_get_int(s, PROP_FILTER_TYPE);
    p->input_luma_filter = obs_data_get_int(s, PROP_INPUT_LUMA_FILTER);
    p->chroma_lowpass_in = obs_data_get_int(s, PROP_CHROMA_LOWPASS_IN);
    p->chroma_demodulation = obs_data_get_int(s, PROP_CHROMA_DEMODULATION);
    p->luma_smear = obs_data_get_double(s, PROP_LUMA_SMEAR);
    p->composite_sharpening = obs_data_get_double(s, PROP_COMPOSITE_SHARPENING);
    p->video_scanline_phase_shift = obs_data_get_int(s, PROP_VIDEO_SCANLINE_PHASE_SHIFT);
    p->video_scanline_phase_shift_offset = obs_data_get_int(s, PROP_VIDEO_SCANLINE_PHASE_SHIFT_OFFSET);
    if (p->enable_head_switching) {
        p->head_switching.enable_mid_line = obs_data_get_bool(s, PROP_HEAD_SWITCHING_START_MID_LINE);
        p->head_switching.height = obs_data_get_int(s, PROP_HEAD_SWITCHING_HEIGHT);
        p->head_switching.offset = obs_data_get_int(s, PROP_HEAD_SWITCHING_OFFSET);
        p->head_switching.horiz_shift = obs_data_get_double(s, PROP_HEAD_SWITCHING_HORIZONTAL_SHIFT);
        p->head_switching.mid_line_position = obs_data_get_double(s, PROP_HEAD_SWITCHING_MID_LINE_POSITION);
        p->head_switching.mid_line_jitter = obs_data_get_double(s, PROP_HEAD_SWITCHING_MID_LINE_JITTER);
    }
    if (p->enable_tracking_noise) {
        p->tracking_noise.height = obs_data_get_int(s, PROP_TRACKING_NOISE_HEIGHT);
        p->tracking_noise.wave_intensity = obs_data_get_double(s, PROP_TRACKING_NOISE_WAVE_INTENSITY);
        p->tracking_noise.snow_intensity = obs_data_get_double(s, PROP_TRACKING_NOISE_SNOW_INTENSITY);
        p->tracking_noise.snow_anisotropy = obs_data_get_double(s, PROP_TRACKING_NOISE_SNOW_ANISOTROPY);
        p->tracking_noise.noise_intensity = obs_data_get_double(s, PROP_TRACKING_NOISE_NOISE_INTENSITY);
    }
    if (p->enable_composite_noise) {
        p->composite_noise.frequency = obs_data_get_double(s, PROP_COMPOSITE_NOISE_FREQUENCY);
        p->composite_noise.intensity = obs_data_get_double(s, PROP_COMPOSITE_NOISE_INTENSITY);
        p->composite_noise.detail = obs_data_get_int(s, PROP_COMPOSITE_NOISE_DETAIL);
    }
    if (p->enable_ringing) {
        p->ringing.frequency = obs_data_get_double(s, PROP_RINGING_FREQUENCY);
        p->ringing.power = obs_data_get_double(s, PROP_RINGING_POWER);
        p->ringing.intensity = obs_data_get_double(s, PROP_RINGING_SCALE);
    }
    if (p->enable_luma_noise) {
        p->luma_noise.frequency = obs_data_get_double(s, PROP_LUMA_NOISE_FREQUENCY);
        p->luma_noise.intensity = obs_data_get_double(s, PROP_LUMA_NOISE_INTENSITY);
        p->luma_noise.detail = obs_data_get_int(s, PROP_LUMA_NOISE_DETAIL);
    }
    if (p->enable_chroma_noise) {
        p->chroma_noise.frequency = obs_data_get_double(s, PROP_CHROMA_NOISE_FREQUENCY);
        p->chroma_noise.intensity = obs_data_get_double(s, PROP_CHROMA_NOISE_INTENSITY);
        p->chroma_noise.detail = obs_data_get_int(s, PROP_CHROMA_NOISE_DETAIL);
    }
    p->snow_intensity = obs_data_get_double(s, PROP_SNOW_INTENSITY);
    p->snow_anisotropy = obs_data_get_double(s, PROP_SNOW_ANISOTROPY);
    p->chroma_phase_noise_intensity = obs_data_get_double(s, PROP_CHROMA_PHASE_NOISE_INTENSITY);
    p->chroma_phase_error = obs_data_get_double(s, PROP_CHROMA_PHASE_ERROR);
    p->chroma_delay_horizontal = obs_data_get_double(s, PROP_CHROMA_DELAY_HORIZONTAL);
    p->chroma_delay_vertical = obs_data_get_int(s, PROP_CHROMA_DELAY_VERTICAL);
    if (p->enable_vhs) {
        p->vhs_settings.tape_speed = obs_data_get_int(s, PROP_VHS_TAPE_SPEED);
        p->vhs_settings.chroma_loss = obs_data_get_double(s, PROP_VHS_CHROMA_LOSS);
        p->vhs_settings.enable_sharpen = obs_data_get_bool(s, PROP_VHS_SHARPEN_ENABLED);
        p->vhs_settings.sharpen_intensity = obs_data_get_double(s, PROP_VHS_SHARPEN_INTENSITY);
        p->vhs_settings.sharpen_frequency = obs_data_get_double(s, PROP_VHS_SHARPEN_FREQUENCY);
        p->vhs_settings.enable_edge_wave = obs_data_get_bool(s, PROP_VHS_EDGE_WAVE_ENABLED);
        p->vhs_settings.edge_wave_intensity = obs_data_get_double(s, PROP_VHS_EDGE_WAVE_INTENSITY);
        p->vhs_settings.edge_wave_speed = obs_data_get_double(s, PROP_VHS_EDGE_WAVE_SPEED);
        p->vhs_settings.edge_wave_frequency = obs_data_get_double(s, PROP_VHS_EDGE_WAVE_FREQUENCY);
        p->vhs_settings.edge_wave_detail = obs_data_get_int(s, PROP_VHS_EDGE_WAVE_DETAIL);
    }
    p->chroma_vert_blend = obs_data_get_bool(s, PROP_CHROMA_VERT_BLEND);
    p->chroma_lowpass_out = obs_data_get_int(s, PROP_CHROMA_LOWPASS_OUT);
    p->scale.horizontal_scale = obs_data_get_double(s, PROP_HORIZONTAL_SCALE);
    p->scale.vertical_scale = obs_data_get_double(s, PROP_VERTICAL_SCALE);
    p->scale.scale_with_video_size = obs_data_get_bool(s, PROP_SCALE_WITH_VIDEO_SIZE);

    fd->paused = obs_data_get_bool(s, PROP_PAUSED);
}

static enum gs_color_space filter_get_color_space(void *data, size_t count, const enum gs_color_space *preferred_spaces) {
    struct ntscrs_filter_data *const fd = data;
    obs_source_t *target = obs_filter_get_target(fd->context);

    if (!target || !fd->texrender) {
        return (count > 0) ? preferred_spaces[0] : GS_CS_SRGB;
    }

    enum gs_color_space space = fd->space;
    for (size_t i = 0; i < count; ++i) {
        space = preferred_spaces[i];
        if (space == fd->space) break;
    }

    return space;
}

struct obs_source_info ntscrs_filter = {
    .id = "ntsc_rs_filter",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
    .get_name = filter_getname,
    .create = filter_create,
    .destroy = filter_destroy,
    .get_width = filter_get_width,
    .get_height = filter_get_height,
    .get_defaults2 = filter_get_defaults,
    .get_properties = filter_properties,
    .update = filter_update,
    .video_tick = filter_tick,
    .video_render = filter_render,
    .video_get_color_space = filter_get_color_space,
};

bool obs_module_load(void) {
    obs_register_source(&ntscrs_filter);

    obs_log(LOG_INFO, "ntsc-rs-obs loaded successfully (version %s)",
         PLUGIN_VERSION);
    return true;
}

void obs_module_unload()
{
    obs_log(LOG_INFO, "plugin unloaded");
}
