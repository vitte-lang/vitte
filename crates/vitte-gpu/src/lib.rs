//! vitte-gpu — Abstraction wgpu pour Vitte
//!
//! Fournit :
//! - Initialisation Instance/Adapter/Device/Queue
//! - Création Surface + configuration (via `winit`, optionnelle)
//! - Helpers buffers/textures/shaders/pipelines
//! - Passes de rendu minimales
//! - Erreurs unifiées `GpuError` et résultats `GpuResult<T>`
//!
//! Version wgpu ciblée: 0.20

#![forbid(unsafe_code)]
#![deny(missing_docs)]

use std::borrow::Cow;
use std::num::NonZeroU32;

use thiserror::Error;
use wgpu::util::DeviceExt;

#[cfg(feature = "trace")]
const TRACE_DIR: Option<&str> = Some("wgpu-trace");
#[cfg(not(feature = "trace"))]
const TRACE_DIR: Option<&str> = None;

/// Résultat standard GPU.
pub type GpuResult<T> = Result<T, GpuError>;

/// Erreurs unifiées.
#[derive(Error, Debug)]
pub enum GpuError {
    /// Aucune carte/adaptateur compatible.
    #[error("aucun adaptateur GPU compatible")]
    NoAdapter,
    /// Échec lors de la création de device/queue.
    #[error("échec création device/queue: {0}")]
    Device(String),
    /// Erreur wgpu générique.
    #[error("wgpu: {0}")]
    Wgpu(String),
    /// Format de surface non supporté.
    #[error("format de surface non supporté")]
    SurfaceFormat,
    /// Shaders.
    #[error("shader error: {0}")]
    Shader(String),
}

/// Contexte GPU principal.
pub struct GpuContext {
    /// Instance wgpu.
    pub instance: wgpu::Instance,
    /// Adaptateur sélectionné.
    pub adapter: wgpu::Adapter,
    /// Device logique.
    pub device: wgpu::Device,
    /// File d’envoi.
    pub queue: wgpu::Queue,
}

impl GpuContext {
    /// Initialise une instance + choisit un adaptateur + crée device/queue.
    ///
    /// `high_perf = true` préfère une carte performante (discrète si possible).
    pub async fn init(high_perf: bool) -> GpuResult<Self> {
        let backends = wgpu::Backends::PRIMARY;
        let dx12_shader_compiler = wgpu::Dx12Compiler::Fxc;
        let flags = wgpu::InstanceFlags::default();

        let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
            backends,
            dx12_shader_compiler,
            flags,
            gles_minor_version: wgpu::Gles3MinorVersion::Automatic,
        });

        let power = if high_perf {
            wgpu::PowerPreference::HighPerformance
        } else {
            wgpu::PowerPreference::LowPower
        };

        let adapter = instance
            .request_adapter(&wgpu::RequestAdapterOptions {
                power_preference: power,
                force_fallback_adapter: false,
                compatible_surface: None,
            })
            .await
            .ok_or(GpuError::NoAdapter)?;

        let optional_trace = TRACE_DIR.map(|p| std::path::Path::new(p).to_path_buf());

        let trace_path = optional_trace.as_ref().map(|p| Cow::from(p.clone()));
        let desc = wgpu::DeviceDescriptor {
            label: Some("vitte-gpu device"),
            required_features: wgpu::Features::empty(),
            required_limits: wgpu::Limits::downlevel_defaults()
                .using_resolution(adapter.limits()),
            memory_hints: wgpu::MemoryHints::default(),
        };

        let (device, queue) = adapter
            .request_device(
                &desc,
                trace_path, // wgpu capture si "trace" activé
            )
            .await
            .map_err(|e| GpuError::Device(e.to_string()))?;

        Ok(Self {
            instance,
            adapter,
            device,
            queue,
        })
    }

    /// Soumet une commande no-op pour valider le device.
    pub fn submit_noop(&self) {
        let mut encoder =
            self.device
                .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("noop"),
                });
        let cmd = encoder.finish();
        self.queue.submit([cmd]);
        self.device.poll(wgpu::Maintain::Wait);
    }

    /// Crée un module shader WGSL depuis source.
    pub fn create_shader_wgsl(&self, src: &str, label: &str) -> wgpu::ShaderModule {
        self.device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some(label),
            source: wgpu::ShaderSource::Wgsl(Cow::Borrowed(src)),
        })
    }

    /// Crée un buffer initialisé.
    pub fn create_buffer_init(
        &self,
        label: &str,
        usage: wgpu::BufferUsages,
        contents: &[u8],
    ) -> wgpu::Buffer {
        self.device
            .create_buffer_init(&wgpu::util::BufferInitDescriptor {
                label: Some(label),
                contents,
                usage,
            })
    }

    /// Crée un buffer non initialisé de `size` octets.
    pub fn create_buffer(&self, label: &str, usage: wgpu::BufferUsages, size: u64) -> wgpu::Buffer {
        self.device
            .create_buffer(&wgpu::BufferDescriptor {
                label: Some(label),
                size,
                usage,
                mapped_at_creation: false,
            })
    }

    /// Crée une texture 2D RGBA8.
    pub fn create_texture_rgba8(
        &self,
        label: &str,
        width: u32,
        height: u32,
        usage: wgpu::TextureUsages,
        mipmapped: bool,
    ) -> (wgpu::Texture, wgpu::TextureView) {
        let mip_level_count = if mipmapped {
            1 + (width.max(height) as f32).log2().floor() as u32
        } else {
            1
        };

        let tex = self.device.create_texture(&wgpu::TextureDescriptor {
            label: Some(label),
            size: wgpu::Extent3d {
                width,
                height,
                depth_or_array_layers: 1,
            },
            mip_level_count,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8UnormSrgb,
            usage,
            view_formats: &[],
        });
        let view = tex.create_view(&wgpu::TextureViewDescriptor::default());
        (tex, view)
    }

    /// Crée un sampler par défaut.
    pub fn create_sampler_linear(&self, label: &str) -> wgpu::Sampler {
        self.device.create_sampler(&wgpu::SamplerDescriptor {
            label: Some(label),
            address_mode_u: wgpu::AddressMode::ClampToEdge,
            address_mode_v: wgpu::AddressMode::ClampToEdge,
            address_mode_w: wgpu::AddressMode::ClampToEdge,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            mipmap_filter: wgpu::FilterMode::Linear,
            ..Default::default()
        })
    }

    /// Pipeline graphique minimal (vertex/fragment WGSL) + layout auto.
    pub fn create_render_pipeline(
        &self,
        label: &str,
        shader: &wgpu::ShaderModule,
        color_format: wgpu::TextureFormat,
        vertex_layouts: &[wgpu::VertexBufferLayout],
        depth_format: Option<wgpu::TextureFormat>,
        primitive: wgpu::PrimitiveState,
    ) -> wgpu::RenderPipeline {
        let bind_layout = self.device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("empty-bgl"),
            entries: &[],
        });
        let pipeline_layout = self.device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("pipeline-layout"),
            bind_group_layouts: &[&bind_layout],
            push_constant_ranges: &[],
        });

        self.device
            .create_render_pipeline(&wgpu::RenderPipelineDescriptor {
                label: Some(label),
                layout: Some(&pipeline_layout),
                vertex: wgpu::VertexState {
                    module: shader,
                    entry_point: "vs_main",
                    buffers: vertex_layouts,
                    compilation_options: Default::default(),
                },
                primitive,
                depth_stencil: depth_format.map(|fmt| wgpu::DepthStencilState {
                    format: fmt,
                    depth_write_enabled: true,
                    depth_compare: wgpu::CompareFunction::Less,
                    stencil: wgpu::StencilState::default(),
                    bias: wgpu::DepthBiasState::default(),
                }),
                multisample: wgpu::MultisampleState::default(),
                fragment: Some(wgpu::FragmentState {
                    module: shader,
                    entry_point: "fs_main",
                    targets: &[Some(wgpu::ColorTargetState {
                        format: color_format,
                        blend: Some(wgpu::BlendState::ALPHA_BLENDING),
                        write_mask: wgpu::ColorWrites::ALL,
                    })],
                    compilation_options: Default::default(),
                }),
                cache: None,
            })
    }

    /// Effectue un clear de la cible de rendu.
    pub fn clear_view(&self, view: &wgpu::TextureView, color: [f64; 4]) {
        let mut encoder =
            self.device
                .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("clear"),
                });
        {
            let _rp = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("clear-pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color {
                            r: color[0],
                            g: color[1],
                            b: color[2],
                            a: color[3],
                        }),
                        store: true,
                    },
                })],
                depth_stencil_attachment: None,
                occlusion_query_set: None,
                timestamp_writes: None,
            });
            let _ = &_rp;
        }
        self.queue.submit([encoder.finish()]);
    }
}

// ---------------- Surface / Swapchain (winit optionnel) ----------------

/// Wrapper surface + configuration.
#[cfg(feature = "winit")]
pub struct SurfaceState {
    /// Surface liée à une fenêtre.
    pub surface: wgpu::Surface<'static>,
    /// Format choisi.
    pub format: wgpu::TextureFormat,
    /// Configuration active.
    pub config: wgpu::SurfaceConfiguration,
    /// Taille courante.
    pub size: winit::dpi::PhysicalSize<u32>,
}

#[cfg(feature = "winit")]
impl SurfaceState {
    /// Crée et configure une surface pour une fenêtre `winit`.
    pub fn new(ctx: &GpuContext, window: &'static winit::window::Window) -> GpuResult<Self> {
        let size = window.inner_size();
        if size.width == 0 || size.height == 0 {
            // wgpu ne supporte pas 0×0, garder 1×1 minimal
            window.request_redraw();
        }

        // wgpu 0.20: création de surface sûre depuis winit 0.30
        let surface = ctx
            .instance
            .create_surface(window)
            .map_err(|e| GpuError::Wgpu(e.to_string()))?;

        let caps = surface.get_capabilities(&ctx.adapter);
        let format = caps
            .formats
            .iter()
            .copied()
            .find(|f| f.is_srgb())
            .or_else(|| caps.formats.first().copied())
            .ok_or(GpuError::SurfaceFormat)?;

        let present_mode = if caps
            .present_modes
            .contains(&wgpu::PresentMode::Mailbox)
        {
            wgpu::PresentMode::Mailbox
        } else if caps.present_modes.contains(&wgpu::PresentMode::FifoRelaxed) {
            wgpu::PresentMode::FifoRelaxed
        } else {
            wgpu::PresentMode::Fifo
        };

        let alpha_mode = caps
            .alpha_modes
            .first()
            .copied()
            .unwrap_or(wgpu::CompositeAlphaMode::Auto);

        let mut config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            format,
            width: size.width.max(1),
            height: size.height.max(1),
            present_mode,
            alpha_mode,
            view_formats: vec![format],
            desired_maximum_frame_latency: 2,
        };

        surface.configure(&ctx.device, &config);
        Ok(Self {
            surface,
            format,
            config,
            size,
        })
    }

    /// Resize et reconfigure la surface.
    pub fn resize(&mut self, ctx: &GpuContext, new_size: winit::dpi::PhysicalSize<u32>) {
        if new_size.width == 0 || new_size.height == 0 {
            return;
        }
        self.size = new_size;
        self.config.width = new_size.width;
        self.config.height = new_size.height;
        self.surface.configure(&ctx.device, &self.config);
    }

    /// Rendu simple: clear + submit.
    pub fn render_clear(&self, ctx: &GpuContext, color: [f64; 4]) -> GpuResult<()> {
        let frame = self
            .surface
            .get_current_texture()
            .map_err(|e| GpuError::Wgpu(e.to_string()))?;
        let view = frame
            .texture
            .create_view(&wgpu::TextureViewDescriptor::default());
        ctx.clear_view(&view, color);
        frame.present();
        Ok(())
    }
}

// ----------------- Exemple de shader WGSL minimal -----------------

/// WGSL minimal: triangle plein écran.
pub const WGSL_TRIANGLE: &str = r#"
@vertex
fn vs_main(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4<f32> {
    var pos = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -3.0),
        vec2<f32>( 3.0,  1.0),
        vec2<f32>(-1.0,  1.0),
    );
    let xy = pos[vi];
    return vec4<f32>(xy, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4<f32> {
    return vec4<f32>(0.10, 0.45, 0.90, 1.0);
}
"#;

// ----------------- Tests de fumée -----------------

#[cfg(test)]
mod tests {
    use super::*;
    use pollster::block_on;

    #[test]
    fn init_and_noop() {
        let ctx = block_on(GpuContext::init(false)).expect("gpu init");
        ctx.submit_noop();

        // shader compilation smoke
        let shader = ctx.create_shader_wgsl(WGSL_TRIANGLE, "triangle");
        let _pipeline = ctx.create_render_pipeline(
            "p",
            &shader,
            wgpu::TextureFormat::Rgba8UnormSrgb,
            &[],
            None,
            wgpu::PrimitiveState {
                topology: wgpu::PrimitiveTopology::TriangleList,
                strip_index_format: None,
                front_face: wgpu::FrontFace::Ccw,
                cull_mode: Some(wgpu::Face::Back),
                unclipped_depth: false,
                polygon_mode: wgpu::PolygonMode::Fill,
                conservative: false,
            },
        );

        // offscreen texture clear
        let (_tex, view) = ctx.create_texture_rgba8(
            "offscreen",
            64,
            64,
            wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::COPY_SRC,
            false,
        );
        ctx.clear_view(&view, [0.0, 0.0, 0.0, 1.0]);
    }

    #[test]
    fn create_buffers() {
        let ctx = pollster::block_on(GpuContext::init(true)).unwrap();
        let data = [0u32, 1, 2, 2, 3, 0];
        let vb = ctx.create_buffer_init(
            "vb",
            wgpu::BufferUsages::VERTEX | wgpu::BufferUsages::COPY_DST,
            bytemuck::cast_slice(&data),
        );
        assert_eq!(vb.size(), (data.len() * std::mem::size_of::<u32>()) as u64);

        let _dyn = ctx.create_buffer("dyn", wgpu::BufferUsages::UNIFORM, 1024);
    }
}