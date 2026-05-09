import AxeBuilder from '@axe-core/playwright';
import { test, expect } from '@playwright/test';

test('mobile menu opens and closes', async ({ page }) => {
  await page.goto('/index.html');
  const menuButton = page.locator('.menu-toggle');
  await expect(menuButton).toBeVisible();
  await menuButton.click();
  await expect(page.locator('.site-nav')).toHaveClass(/is-open/);
  await menuButton.click();
  await expect(page.locator('.site-nav')).not.toHaveClass(/is-open/);
});

test('back-to-top appears after scroll', async ({ page }) => {
  await page.goto('/doc.html');
  await page.evaluate(() => window.scrollTo(0, 1200));
  await expect(page.locator('.back-to-top')).toHaveClass(/is-visible/);
});

test('basic a11y landmarks and focus targets', async ({ page }) => {
  await page.goto('/index.html');
  await expect(page.locator('header.site-header')).toBeVisible();
  await expect(page.locator('main.site-main')).toBeVisible();
  await expect(page.locator('footer.site-footer')).toBeVisible();
  await page.keyboard.press('Tab');
  await expect(page.locator(':focus')).toBeVisible();
});


test('a11y axe scan key pages', async ({ page }) => {
  for (const p of ['/index.html','/doc.html','/news.html']) {
    await page.goto(p);
    const results = await new AxeBuilder({ page }).analyze();
    expect(results.violations).toEqual([]);
  }
});

test('offline fallback page is reachable and actionable', async ({ page, context }) => {
  await page.goto('/offline.html');
  await context.setOffline(true);
  await expect(page.locator('h1')).toContainText(/offline|hors ligne/i);
  await expect(page.locator('#retry-btn')).toBeVisible();
  await expect(page.locator('#network-status')).toBeVisible();
  await context.setOffline(false);
});
