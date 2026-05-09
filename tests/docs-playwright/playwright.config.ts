import { defineConfig, devices } from '@playwright/test';

export default defineConfig({
  testDir: '.',
  timeout: 30_000,
  use: {
    baseURL: 'http://127.0.0.1:4173',
  },
  webServer: {
    command: 'python3 -m http.server 4173 --directory docs',
    url: 'http://127.0.0.1:4173',
    reuseExistingServer: true,
    timeout: 20_000,
  },
  projects: [
    { name: 'mobile-chrome', use: { ...devices['Pixel 7'] } },
    { name: 'tablet-safari', use: { ...devices['iPad (gen 7)'] } },
  ],
});
