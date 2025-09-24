use vitte_lsp::start_stdio;

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    start_stdio().await
}
