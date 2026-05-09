import { test, expect } from '@playwright/test';

test('visual home', async ({ page }) => {
  await page.goto('/index.html');
  await expect(page).toHaveScreenshot('home.png', { fullPage: true });
});

test('visual grammar', async ({ page }) => {
  await page.goto('/grammar/index.html');
  await expect(page).toHaveScreenshot('grammar.png', { fullPage: true });
});

test('visual doc', async ({ page }) => {
  await page.goto('/doc.html');
  await expect(page).toHaveScreenshot('doc.png', { fullPage: true });
});
