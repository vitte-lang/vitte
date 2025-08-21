# Fonts

Recommandations :
- **Inter** (titres/corps) : https://fonts.google.com/specimen/Inter
- **JetBrains Mono** (code) : https://fonts.google.com/specimen/JetBrains+Mono

## CSS prêt à coller
```html
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&family=JetBrains+Mono:wght@400;600&display=swap" rel="stylesheet">
<style>
  :root {
    --vitte-primary:#6C5CE7; --vitte-secondary:#00D1B2; --vitte-accent:#FF6B6B;
    --vitte-neutral-900:#0B0B0C; --vitte-neutral-100:#F2F4F8;
  }
  body { font-family: Inter, system-ui, -apple-system, Segoe UI, Roboto, sans-serif; }
  code, pre { font-family: "JetBrains Mono", ui-monospace, SFMono-Regular, Menlo, monospace; }
</style>
```
