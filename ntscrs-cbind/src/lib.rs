use ntscrs::{
    ntsc::NtscEffect,
    settings::standard::*,
    yiq_fielding::*,
};

#[repr(C)]
pub enum NtscRsPixelFormat {
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
}

#[repr(C)]
pub enum NtscRsUseField {
    UseFieldAlternating,
    UseFieldUpper,
    UseFieldLower,
    UseFieldInterleavedUpper,
    UseFieldInterleavedLower,
    UseFieldBoth
}

#[repr(C)]
pub enum NtscRsFilterType {
    FilterTypeConstantK,
    FilterTypeButterworth,
}

#[repr(C)]
pub enum NtscRsLumaLowpass {
    LumaLowpassNone,
    LumaLowpassBox,
    LumaLowpassNotch,
}

#[repr(C)]
pub enum NtscRsChromaLowpass {
    ChromaLowpassNone,
    ChromaLowpassLight,
    ChromaLowpassFull
}

#[repr(C)]
pub enum NtscRsChromaDemodulationFilter {
    ChromaDemodFilterBox,
    ChromaDemodFilterNotch,
    ChromaDemodFilterOneLineComb,
    ChromaDemodFilterTwoLineComb,
}

#[repr(C)]
pub enum NtscRsPhaseShift {
    PhaseShiftDegrees0,
    PhaseShiftDegrees90,
    PhaseShiftDegrees180,
    PhaseShiftDegrees270,
}

#[repr(C)]
pub enum NtscRsTapeSpeed {
    TapeSpeedNONE,
    TapeSpeedSP,
    TapeSpeedLP,
    TapeSpeedEP,
}

#[repr(C)]
pub struct NtscRsHeadSwitchingSettings {
    pub height: u32,
    pub offset: u32,
    pub horiz_shift: f32,
    pub mid_line_position: f32,
    pub mid_line_jitter: f32,
    pub enable_mid_line: bool,
}

#[repr(C)]
pub struct NtscRsTrackingNoiseSettings {
    pub height: u32,
    pub wave_intensity: f32,
    pub snow_intensity: f32,
    pub snow_anisotropy: f32,
    pub noise_intensity: f32,
}

#[repr(C)]
pub struct NtscRsRingingSettings {
    pub frequency: f32,
    pub power: f32,
    pub intensity: f32
}

#[repr(C)]
pub struct NtscRsFbmNoiseSettings {
    pub frequency: f32,
    pub intensity: f32,
    pub detail: u32,
}

#[repr(C)]
pub struct NtscRsVHSSettings {
    pub tape_speed: NtscRsTapeSpeed,
    pub chroma_loss: f32,
    pub sharpen_intensity: f32,
    pub sharpen_frequency: f32,
    pub edge_wave_intensity: f32,
    pub edge_wave_speed: f32,
    pub edge_wave_frequency: f32,
    pub edge_wave_detail: i32,
    pub enable_sharpen: bool,
    pub enable_edge_wave: bool,
}

#[repr(C)]
pub struct NtscRsScaleSettings {
    pub horizontal_scale: f32,
    pub vertical_scale: f32,
    pub scale_with_video_size: bool,
}

#[repr(C)]
pub struct NtscRsEffectParams {
    pub random_seed: i32,
    pub use_field: NtscRsUseField,
    pub filter_type: NtscRsFilterType,
    pub input_luma_filter: NtscRsLumaLowpass,
    pub chroma_lowpass_in: NtscRsChromaLowpass,
    pub chroma_demodulation: NtscRsChromaDemodulationFilter,
    pub luma_smear: f32,
    pub composite_sharpening: f32,
    pub video_scanline_phase_shift: NtscRsPhaseShift,
    pub video_scanline_phase_shift_offset: i32,
    pub head_switching: NtscRsHeadSwitchingSettings,
    pub tracking_noise: NtscRsTrackingNoiseSettings,
    pub composite_noise: NtscRsFbmNoiseSettings,
    pub ringing: NtscRsRingingSettings,
    pub luma_noise: NtscRsFbmNoiseSettings,
    pub chroma_noise: NtscRsFbmNoiseSettings,
    pub snow_intensity: f32,
    pub snow_anisotropy: f32,
    pub chroma_phase_noise_intensity: f32,
    pub chroma_phase_error: f32,
    pub chroma_delay_horizontal: f32,
    pub chroma_delay_vertical: i32,
    pub vhs_settings: NtscRsVHSSettings,
    pub chroma_vert_blend: bool,
    pub chroma_lowpass_out: NtscRsChromaLowpass,
    pub scale: NtscRsScaleSettings,

    pub enable_head_switching: bool,
    pub enable_tracking_noise: bool,
    pub enable_composite_noise: bool,
    pub enable_ringing: bool,
    pub enable_luma_noise: bool,
    pub enable_chroma_noise: bool,
    pub enable_vhs: bool,
}

impl Default for NtscRsEffectParams {
    fn default() -> Self {
        ntscrs_effect_to_params(NtscEffect::default())
    }
}

pub fn ntscrs_effect_from_params(params: NtscRsEffectParams) -> NtscEffect {
    let mut effect = NtscEffect::default();
    effect.random_seed = params.random_seed;
    effect.use_field = match params.use_field {
        NtscRsUseField::UseFieldAlternating => UseField::Alternating,
        NtscRsUseField::UseFieldUpper => UseField::Upper,
        NtscRsUseField::UseFieldLower => UseField::Lower,
        NtscRsUseField::UseFieldBoth => UseField::Both,
        NtscRsUseField::UseFieldInterleavedUpper => UseField::InterleavedUpper,
        NtscRsUseField::UseFieldInterleavedLower => UseField::InterleavedLower,
    };
    effect.filter_type = match params.filter_type {
        NtscRsFilterType::FilterTypeButterworth => FilterType::Butterworth,
        NtscRsFilterType::FilterTypeConstantK => FilterType::ConstantK,
    };
    effect.input_luma_filter = match params.input_luma_filter {
        NtscRsLumaLowpass::LumaLowpassNone => LumaLowpass::None,
        NtscRsLumaLowpass::LumaLowpassBox => LumaLowpass::Box,
        NtscRsLumaLowpass::LumaLowpassNotch => LumaLowpass::Notch,
    };
    effect.chroma_lowpass_in = match params.chroma_lowpass_in {
        NtscRsChromaLowpass::ChromaLowpassNone => ChromaLowpass::None,
        NtscRsChromaLowpass::ChromaLowpassLight => ChromaLowpass::Light,
        NtscRsChromaLowpass::ChromaLowpassFull => ChromaLowpass::Full,
    };
    effect.chroma_demodulation = match params.chroma_demodulation {
        NtscRsChromaDemodulationFilter::ChromaDemodFilterBox => ChromaDemodulationFilter::Box,
        NtscRsChromaDemodulationFilter::ChromaDemodFilterNotch => ChromaDemodulationFilter::Notch,
        NtscRsChromaDemodulationFilter::ChromaDemodFilterOneLineComb => ChromaDemodulationFilter::OneLineComb,
        NtscRsChromaDemodulationFilter::ChromaDemodFilterTwoLineComb => ChromaDemodulationFilter::TwoLineComb,
    };
    effect.luma_smear =  params.luma_smear;
    effect.chroma_lowpass_out = match params.chroma_lowpass_out {
        NtscRsChromaLowpass::ChromaLowpassNone => ChromaLowpass::None,
        NtscRsChromaLowpass::ChromaLowpassLight => ChromaLowpass::Light,
        NtscRsChromaLowpass::ChromaLowpassFull => ChromaLowpass::Full,
    };
    effect.composite_sharpening = params.composite_sharpening;
    effect.video_scanline_phase_shift = match params.video_scanline_phase_shift {
        NtscRsPhaseShift::PhaseShiftDegrees0 => PhaseShift::Degrees0,
        NtscRsPhaseShift::PhaseShiftDegrees90 => PhaseShift::Degrees90,
        NtscRsPhaseShift::PhaseShiftDegrees180 => PhaseShift::Degrees180,
        NtscRsPhaseShift::PhaseShiftDegrees270 => PhaseShift::Degrees270,
    };
    effect.video_scanline_phase_shift_offset = params.video_scanline_phase_shift_offset;
    effect.head_switching = if params.enable_head_switching {
        Some(
            HeadSwitchingSettings {
                height: params.head_switching.height,
                offset: params.head_switching.offset,
                horiz_shift: params.head_switching.horiz_shift,
                mid_line: if params.head_switching.enable_mid_line {
                    Some(HeadSwitchingMidLineSettings {
                        position: params.head_switching.mid_line_position,
                        jitter: params.head_switching.mid_line_jitter,
                    })
                } else { None }
            }
        )
    } else { None };
    effect.tracking_noise = if params.enable_tracking_noise { 
        Some(
            TrackingNoiseSettings {
                height: params.tracking_noise.height,
                wave_intensity: params.tracking_noise.wave_intensity,
                snow_intensity: params.tracking_noise.snow_intensity,
                snow_anisotropy: params.tracking_noise.snow_anisotropy,
                noise_intensity: params.tracking_noise.noise_intensity,
            }
        )
    } else { None };
    effect.ringing = if params.enable_ringing {
        Some(
            RingingSettings {
                frequency: params.ringing.frequency,
                power: params.ringing.power,
                intensity: params.ringing.intensity,
            }
        )
    } else { None };
    effect.snow_intensity = params.snow_intensity;
    effect.snow_anisotropy = params.snow_anisotropy;
    effect.composite_noise = if params.enable_composite_noise {
        Some(
            FbmNoiseSettings {
                frequency: params.composite_noise.frequency,
                intensity: params.composite_noise.intensity,
                detail: params.composite_noise.detail,
            }
        )
    } else { None };
    effect.luma_noise = if params.enable_luma_noise {
        Some(
            FbmNoiseSettings {
                frequency: params.luma_noise.frequency,
                intensity: params.luma_noise.intensity,
                detail: params.luma_noise.detail,
            }
        )
    } else { None };
    effect.chroma_noise = if params.enable_chroma_noise {
        Some(
            FbmNoiseSettings {
                frequency: params.chroma_noise.frequency,
                intensity: params.chroma_noise.intensity,
                detail: params.chroma_noise.detail,
            }
        )
    } else { None };
    effect.chroma_phase_noise_intensity = params.chroma_phase_noise_intensity;
    effect.chroma_phase_error = params.chroma_phase_error;
    effect.chroma_delay_horizontal = params.chroma_delay_horizontal;
    effect.chroma_delay_vertical = params.chroma_delay_vertical;
    effect.vhs_settings = if params.enable_vhs {
        Some(
            VHSSettings {
                tape_speed: match params.vhs_settings.tape_speed {
                    NtscRsTapeSpeed::TapeSpeedNONE => VHSTapeSpeed::NONE,
                    NtscRsTapeSpeed::TapeSpeedLP => VHSTapeSpeed::LP,
                    NtscRsTapeSpeed::TapeSpeedEP => VHSTapeSpeed::EP,
                    NtscRsTapeSpeed::TapeSpeedSP => VHSTapeSpeed::SP,
                },
                chroma_loss: params.vhs_settings.chroma_loss,
                sharpen: if params.vhs_settings.enable_sharpen {
                    Some(
                        VHSSharpenSettings {
                            intensity: params.vhs_settings.sharpen_intensity,
                            frequency: params.vhs_settings.sharpen_frequency,
                        }
                    )
                } else { None },
                edge_wave: if params.vhs_settings.enable_edge_wave {
                    Some(
                        VHSEdgeWaveSettings {
                            intensity: params.vhs_settings.edge_wave_intensity,
                            speed: params.vhs_settings.edge_wave_speed,
                            frequency: params.vhs_settings.edge_wave_frequency,
                            detail: params.vhs_settings.edge_wave_detail,
                        }
                    )
                } else { None },
            }
        )
    } else { None };
    effect.chroma_vert_blend = params.chroma_vert_blend;
    effect.scale = Some(
        ScaleSettings {
            horizontal_scale: params.scale.horizontal_scale,
            vertical_scale: params.scale.vertical_scale,
            scale_with_video_size: params.scale.scale_with_video_size,
        }
    );

    effect
}

pub fn ntscrs_effect_to_params(effect: NtscEffect) -> NtscRsEffectParams {
    let head_switching = effect.head_switching.clone().unwrap_or_default();
    let head_switching_mid_line = head_switching.mid_line.clone().unwrap_or_default();
    let tracking_noise = effect.tracking_noise.clone().unwrap_or_default();
    let composite_noise = effect.composite_noise.clone().unwrap_or_default();
    let luma_noise = effect.luma_noise.clone().unwrap_or_default();
    let chroma_noise = effect.chroma_noise.clone().unwrap_or_default();
    let ringing = effect.ringing.clone().unwrap_or_default();
    let scale = effect.scale.clone().unwrap_or_default();
    let vhs = effect.vhs_settings.clone().unwrap_or_default();
    let vhs_sharpen = vhs.sharpen.clone().unwrap_or_default();
    let vhs_edge_wave = vhs.edge_wave.clone().unwrap_or_default();

    NtscRsEffectParams{
        random_seed: effect.random_seed,
        use_field: match effect.use_field {
            UseField::Alternating => NtscRsUseField::UseFieldAlternating,
            UseField::Upper => NtscRsUseField::UseFieldUpper,
            UseField::Lower => NtscRsUseField::UseFieldLower,
            UseField::Both => NtscRsUseField::UseFieldBoth,
            UseField::InterleavedUpper => NtscRsUseField::UseFieldInterleavedUpper,
            UseField::InterleavedLower => NtscRsUseField::UseFieldInterleavedLower,
        },
        filter_type: match effect.filter_type {
            FilterType::Butterworth => NtscRsFilterType::FilterTypeButterworth,
            FilterType::ConstantK => NtscRsFilterType::FilterTypeConstantK,
        },
        input_luma_filter: match effect.input_luma_filter {
            LumaLowpass::None => NtscRsLumaLowpass::LumaLowpassNone,
            LumaLowpass::Box => NtscRsLumaLowpass::LumaLowpassBox,
            LumaLowpass::Notch => NtscRsLumaLowpass::LumaLowpassNotch,
        },
        chroma_lowpass_in: match effect.chroma_lowpass_in {
            ChromaLowpass::None => NtscRsChromaLowpass::ChromaLowpassNone,
            ChromaLowpass::Light => NtscRsChromaLowpass::ChromaLowpassLight,
            ChromaLowpass::Full => NtscRsChromaLowpass::ChromaLowpassFull,
        },
        chroma_demodulation: match effect.chroma_demodulation {
            ChromaDemodulationFilter::Box => NtscRsChromaDemodulationFilter::ChromaDemodFilterBox,
            ChromaDemodulationFilter::Notch => NtscRsChromaDemodulationFilter::ChromaDemodFilterNotch,
            ChromaDemodulationFilter::OneLineComb => NtscRsChromaDemodulationFilter::ChromaDemodFilterOneLineComb,
            ChromaDemodulationFilter::TwoLineComb => NtscRsChromaDemodulationFilter::ChromaDemodFilterTwoLineComb,
        },
        luma_smear: effect.luma_smear,
        composite_sharpening: effect.composite_sharpening,
        video_scanline_phase_shift: match effect.video_scanline_phase_shift {
            PhaseShift::Degrees0 => NtscRsPhaseShift::PhaseShiftDegrees0,
            PhaseShift::Degrees90 => NtscRsPhaseShift::PhaseShiftDegrees90,
            PhaseShift::Degrees180 => NtscRsPhaseShift::PhaseShiftDegrees180,
            PhaseShift::Degrees270 => NtscRsPhaseShift::PhaseShiftDegrees270,
        },
        video_scanline_phase_shift_offset: effect.video_scanline_phase_shift_offset,
        head_switching: NtscRsHeadSwitchingSettings {
            height: head_switching.height,
            offset: head_switching.offset,
            horiz_shift: head_switching.horiz_shift,
            mid_line_position: head_switching_mid_line.position,
            mid_line_jitter: head_switching_mid_line.jitter,
            enable_mid_line: head_switching.mid_line.is_some(),
        },
        tracking_noise: NtscRsTrackingNoiseSettings {
            snow_anisotropy: tracking_noise.snow_anisotropy,
            height: tracking_noise.height,
            wave_intensity: tracking_noise.wave_intensity,
            snow_intensity: tracking_noise.snow_intensity,
            noise_intensity: tracking_noise.noise_intensity,
        },
        composite_noise: NtscRsFbmNoiseSettings {
            frequency: composite_noise.frequency,
            intensity: composite_noise.intensity,
            detail: composite_noise.detail,
        },
        ringing: NtscRsRingingSettings {
            intensity: ringing.intensity,
            frequency: ringing.frequency,
            power: ringing.power,
        },
        luma_noise: NtscRsFbmNoiseSettings {
            frequency: luma_noise.frequency,
            intensity: luma_noise.intensity,
            detail: luma_noise.detail,
        },
        chroma_noise: NtscRsFbmNoiseSettings {
            frequency: chroma_noise.frequency,
            intensity: chroma_noise.intensity,
            detail: chroma_noise.detail,
        },
        snow_intensity: effect.snow_intensity,
        snow_anisotropy: effect.snow_anisotropy,
        chroma_phase_noise_intensity: effect.chroma_phase_noise_intensity,
        chroma_phase_error: effect.chroma_phase_error,
        chroma_delay_horizontal: effect.chroma_delay_horizontal,
        chroma_delay_vertical: effect.chroma_delay_vertical,
        vhs_settings: NtscRsVHSSettings {
            tape_speed: match vhs.tape_speed {
                VHSTapeSpeed::NONE => NtscRsTapeSpeed::TapeSpeedNONE,
                VHSTapeSpeed::SP => NtscRsTapeSpeed::TapeSpeedSP,
                VHSTapeSpeed::LP => NtscRsTapeSpeed::TapeSpeedLP,
                VHSTapeSpeed::EP => NtscRsTapeSpeed::TapeSpeedEP,
            },
            chroma_loss: vhs.chroma_loss,
            sharpen_intensity: vhs_sharpen.intensity,
            sharpen_frequency: vhs_sharpen.frequency,
            edge_wave_intensity: vhs_edge_wave.intensity,
            edge_wave_speed: vhs_edge_wave.speed,
            edge_wave_frequency: vhs_edge_wave.frequency,
            edge_wave_detail: vhs_edge_wave.detail,
            enable_sharpen: vhs.sharpen != None,
            enable_edge_wave: vhs.edge_wave.is_some(),
        },
        chroma_vert_blend: effect.chroma_vert_blend,
        chroma_lowpass_out: match effect.chroma_lowpass_out {
            ChromaLowpass::None => NtscRsChromaLowpass::ChromaLowpassNone,
            ChromaLowpass::Light => NtscRsChromaLowpass::ChromaLowpassLight,
            ChromaLowpass::Full => NtscRsChromaLowpass::ChromaLowpassFull,
        },
        scale: NtscRsScaleSettings {
            horizontal_scale: scale.horizontal_scale,
            vertical_scale: scale.vertical_scale,
            scale_with_video_size: scale.scale_with_video_size,
        },
        enable_head_switching: effect.head_switching.is_some(),
        enable_tracking_noise: effect.tracking_noise.is_some(),
        enable_composite_noise: effect.composite_noise.is_some(),
        enable_ringing: effect.ringing.is_some(),
        enable_luma_noise: effect.luma_noise.is_some(),
        enable_chroma_noise: effect.chroma_noise.is_some(),
        enable_vhs: effect.vhs_settings.is_some(),
    }
}

#[no_mangle]
pub extern "C" fn ntscrs_default_effect_params(params: *mut NtscRsEffectParams) {
    unsafe { *params = NtscRsEffectParams::default() };
}

#[macro_export]
macro_rules! impl_pix_fmt_fn {
    ($fn_name: ident, $pix_fmt: ident) => {
        #[no_mangle]
        pub extern "C" fn $fn_name(
            params: NtscRsEffectParams,
            dimension_x: usize,
            dimension_y: usize,
            input_frame: *mut u8,
            frame_num: usize,
        ) {
            let buf_size = dimension_x * dimension_y * $pix_fmt::pixel_bytes();
            let input_frame = unsafe { std::slice::from_raw_parts_mut(input_frame as *mut <$pix_fmt as PixelFormat>::DataFormat, buf_size) };
            let effect = ntscrs_effect_from_params(params);
            effect.apply_effect_to_buffer::<$pix_fmt>(
                (dimension_x, dimension_y),
                input_frame,
                frame_num,
                [1.0, 1.0],
            );
        }
    };
}

impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_rgbx8, Rgbx8);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_xrgb8, Xrgb8);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_bgrx8, Bgrx8);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_xbgr8, Xbgr8);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_rgbx16, Rgbx16);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_xrgb16, Xrgb16);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_bgrx16, Bgrx16);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_xbgr16, Xbgr16);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_rgbx16s, Rgbx16s);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_xrgb16s, Xrgb16s);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_bgrx16s, Bgrx16s);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_xbgr16s, Xbgr16s);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_rgbx32f, Rgbx32f);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_xrgb32f, Xrgb32f);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_bgrx32f, Bgrx32f);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_xbgr32f, Xbgr32f);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_rgb8, Rgb8);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_bgr8, Bgr8);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_rgb16, Rgb16);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_bgr16, Bgr16);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_rgb16s, Rgb16s);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_bgr16s, Bgr16s);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_rgb32f, Rgb32f);
impl_pix_fmt_fn!(ntscrs_apply_effect_to_buffer_bgr32f, Bgr32f);

#[no_mangle]
pub extern "C" fn ntscrs_apply_effect_to_buffer(
    params: NtscRsEffectParams,
    dimension_x: usize,
    dimension_y: usize,
    input_frame: *mut u8,
    pix_fmt: NtscRsPixelFormat,
    frame_num: usize,
) {
    macro_rules! call_with_args {
        ($x: ident) => { $x(params, dimension_x, dimension_y, input_frame, frame_num) };
    }

    match pix_fmt {
        NtscRsPixelFormat::Rgbx8 => call_with_args!(ntscrs_apply_effect_to_buffer_rgbx8),
        NtscRsPixelFormat::Xrgb8 => call_with_args!(ntscrs_apply_effect_to_buffer_xrgb8),
        NtscRsPixelFormat::Bgrx8 => call_with_args!(ntscrs_apply_effect_to_buffer_bgrx8),
        NtscRsPixelFormat::Xbgr8 => call_with_args!(ntscrs_apply_effect_to_buffer_xbgr8),
        NtscRsPixelFormat::Rgbx16 => call_with_args!(ntscrs_apply_effect_to_buffer_rgbx16),
        NtscRsPixelFormat::Xrgb16 => call_with_args!(ntscrs_apply_effect_to_buffer_xrgb16),
        NtscRsPixelFormat::Bgrx16 => call_with_args!(ntscrs_apply_effect_to_buffer_bgrx16),
        NtscRsPixelFormat::Xbgr16 => call_with_args!(ntscrs_apply_effect_to_buffer_xbgr16),
        NtscRsPixelFormat::Rgbx16s => call_with_args!(ntscrs_apply_effect_to_buffer_rgbx16s),
        NtscRsPixelFormat::Xrgb16s => call_with_args!(ntscrs_apply_effect_to_buffer_xrgb16s),
        NtscRsPixelFormat::Bgrx16s => call_with_args!(ntscrs_apply_effect_to_buffer_bgrx16s),
        NtscRsPixelFormat::Xbgr16s => call_with_args!(ntscrs_apply_effect_to_buffer_xbgr16s),
        NtscRsPixelFormat::Rgbx32f => call_with_args!(ntscrs_apply_effect_to_buffer_rgbx32f),
        NtscRsPixelFormat::Xrgb32f => call_with_args!(ntscrs_apply_effect_to_buffer_xrgb32f),
        NtscRsPixelFormat::Bgrx32f => call_with_args!(ntscrs_apply_effect_to_buffer_bgrx32f),
        NtscRsPixelFormat::Xbgr32f => call_with_args!(ntscrs_apply_effect_to_buffer_xbgr32f),
        NtscRsPixelFormat::Rgb8 => call_with_args!(ntscrs_apply_effect_to_buffer_rgb8),
        NtscRsPixelFormat::Bgr8 => call_with_args!(ntscrs_apply_effect_to_buffer_bgr8),
        NtscRsPixelFormat::Rgb16 => call_with_args!(ntscrs_apply_effect_to_buffer_rgb16),
        NtscRsPixelFormat::Bgr16 => call_with_args!(ntscrs_apply_effect_to_buffer_bgr16),
        NtscRsPixelFormat::Rgb16s => call_with_args!(ntscrs_apply_effect_to_buffer_rgb16s),
        NtscRsPixelFormat::Bgr16s => call_with_args!(ntscrs_apply_effect_to_buffer_bgr16s),
        NtscRsPixelFormat::Rgb32f => call_with_args!(ntscrs_apply_effect_to_buffer_rgb32f),
        NtscRsPixelFormat::Bgr32f => call_with_args!(ntscrs_apply_effect_to_buffer_bgr32f),
    }
}
