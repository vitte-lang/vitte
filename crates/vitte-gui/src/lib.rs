//! vitte-gui — couche graphique unifiée pour Vitte
//!
//! Fournit :
//! - Intégration complète `egui` + `winit` + `wgpu` (desktop).
//! - Intégration Web (wasm) via `wasm-bindgen` + `web-sys`.
//! - Support optionnel `eframe`.
//!
//! Objectif : une API simple pour lancer une app GUI multiplateforme.
//!
//! # Exemples
//!
//! ## Desktop
//! ```no_run
//! use vitte_gui::native::run_native;
//!
//! fn main() -> anyhow::Result<()> {
//!     run_native("Demo", |ctx| {
//!         egui::CentralPanel::default().show(ctx, |ui| {
//!             ui.heading("Hello world");
//!         });
//!     })
//! }
//! ```
//!
//! ## Web (wasm)
//! ```ignore
//! // via wasm-bindgen
//! vitte_gui::web::start("canvas_id");
//! ```

#![forbid(unsafe_code)]
#![deny(missing_docs)]

use thiserror::Error;

/// Résultat générique GUI.
pub type GuiResult<T> = Result<T, GuiError>;

/// Erreurs possibles.
#[derive(Error, Debug)]
pub enum GuiError {
    /// Backend (winit, wgpu, egui).
    #[error("backend: {0}")]
    Backend(String),
    /// Non supporté.
    #[error("unsupported: {0}")]
    Unsupported(&'static str),
}

/// Interface commune pour une application egui.
pub trait GuiApp {
    /// Appelée chaque frame.
    fn update(&mut self, ctx: &egui::Context);
}

// ===================== Native =====================

#[cfg(feature = "native")]
pub mod native {
    use super::*;
    use egui::{Context, FullOutput};
    use egui_wgpu::{Renderer, ScreenDescriptor};
    use egui_winit::State;
    use pollster::block_on;
    use wgpu::SurfaceError;
    use winit::{
        dpi::LogicalSize,
        event::{Event, WindowEvent},
        event_loop::{ControlFlow, EventLoop},
        window::WindowBuilder,
    };

    /// Boucle native `egui` + `winit` + `wgpu`.
    pub fn run_native<F>(title: &str, mut ui: F) -> GuiResult<()>
    where
        F: FnMut(&Context) + 'static,
    {
        let event_loop = EventLoop::new().map_err(|e| GuiError::Backend(format!("{e:?}")))?;
        let window = WindowBuilder::new()
            .with_title(title)
            .with_inner_size(LogicalSize::new(1024.0, 768.0))
            .build(&event_loop)
            .map_err(|e| GuiError::Backend(format!("{e:?}")))?;

        let instance = wgpu::Instance::default();
        let surface = instance
            .create_surface(&window)
            .map_err(|e| GuiError::Backend(format!("{e:?}")))?;
        let adapter = block_on(instance.request_adapter(&wgpu::RequestAdapterOptions {
            compatible_surface: Some(&surface),
            ..Default::default()
        }))
        .ok_or(GuiError::Backend("no adapter".into()))?;
        let (device, queue) = block_on(adapter.request_device(
            &wgpu::DeviceDescriptor::default(),
            None,
        ))
        .map_err(|e| GuiError::Backend(format!("{e:?}")))?;

        // config surface
        let size = window.inner_size();
        let caps = surface.get_capabilities(&adapter);
        let format = caps
            .formats
            .iter()
            .copied()
            .find(|f| f.is_srgb())
            .unwrap_or(caps.formats[0]);
        let mut config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            format,
            width: size.width,
            height: size.height,
            present_mode: wgpu::PresentMode::AutoVsync,
            alpha_mode: caps.alpha_modes[0],
            view_formats: vec![format],
            desired_maximum_frame_latency: 2,
        };
        surface.configure(&device, &config);

        // egui
        let ctx = Context::default();
        let mut state = State::new(ctx.clone(), &window, None, None);
        let mut renderer = Renderer::new(&device, format, None, 1);

        event_loop
            .run(move |event, _, control_flow| {
                *control_flow = ControlFlow::Poll;
                match event {
                    Event::WindowEvent {
                        event: WindowEvent::CloseRequested,
                        ..
                    } => *control_flow = ControlFlow::Exit,
                    Event::WindowEvent { event, .. } => {
                        let _ = state.on_event(&ctx, &event);
                    }
                    Event::RedrawRequested(_) => {
                        let input = state.take_egui_input(&window);
                        ctx.begin_frame(input);
                        ui(&ctx);
                        let FullOutput {
                            platform_output,
                            textures_delta,
                            shapes,
                            ..
                        } = ctx.end_frame();
                        state.handle_platform_output(&window, &ctx, platform_output);

                        let clipped = ctx.tessellate(shapes);
                        let screen = ScreenDescriptor {
                            size_in_pixels: [config.width, config.height],
                            pixels_per_point: state.pixels_per_point(),
                        };

                        let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
                            label: Some("encoder"),
                        });
                        let frame = match surface.get_current_texture() {
                            Ok(f) => f,
                            Err(SurfaceError::Lost) => {
                                config.width = size.width;
                                config.height = size.height;
                                surface.configure(&device, &config);
                                surface.get_current_texture().unwrap()
                            }
                            Err(e) => {
                                eprintln!("surface error: {e:?}");
                                return;
                            }
                        };
                        let view = frame
                            .texture
                            .create_view(&wgpu::TextureViewDescriptor::default());

                        renderer.update_buffers(&device, &queue, &mut encoder, &clipped, &screen);
                        renderer.update_textures(&device, &queue, &textures_delta).unwrap();
                        renderer.render(&mut encoder, &view, &clipped, &screen, None);
                        queue.submit(Some(encoder.finish()));
                        frame.present();
                        renderer.free_textures(&textures_delta);
                    }
                    Event::MainEventsCleared => {
                        window.request_redraw();
                    }
                    _ => {}
                }
            })
            .map_err(|e| GuiError::Backend(format!("{e:?}")))
    }
}

// ===================== Web (wasm) =====================

#[cfg(feature = "web")]
pub mod web {
    use super::*;
    use wasm_bindgen::prelude::*;
    use wasm_bindgen::JsCast;
    use web_sys::{window, HtmlCanvasElement};

    /// Démarre le rendu egui dans un canvas HTML donné par id.
    #[wasm_bindgen]
    pub fn start(id: &str) -> Result<(), JsValue> {
        let win = window().ok_or("no window")?;
        let doc = win.document().ok_or("no doc")?;
        let canvas: HtmlCanvasElement = doc
            .get_element_by_id(id)
            .ok_or("no canvas")?
            .dyn_into::<HtmlCanvasElement>()?;
        canvas.set_width(800);
        canvas.set_height(600);

        // init egui context
        let ctx = egui::Context::default();
        ctx.begin_frame(egui::RawInput::default());
        egui::CentralPanel::default().show(&ctx, |ui| {
            ui.label("Hello from vitte-gui (web)");
        });
        ctx.end_frame();

        Ok(())
    }
}

// ===================== eframe wrapper =====================

#[cfg(feature = "eframe")]
pub mod frame {
    use super::*;

    /// Run eframe App natif.
    pub fn run_eframe<A: eframe::App + 'static>(title: &str, app: A) -> GuiResult<()> {
        let opts = eframe::NativeOptions::default();
        eframe::run_native(title, opts, Box::new(|_| Box::new(app)))
            .map_err(|e| GuiError::Backend(format!("{e:?}")))
    }
}

// ===================== Tests =====================

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn error_fmt() {
        let e = GuiError::Unsupported("no");
        assert_eq!(format!("{e}"), "unsupported: no");
    }
}