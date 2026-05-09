import { initMobileMenu } from './modules/nav.js';
import { initAnchors } from './modules/anchors.js';
import { initReadingMode } from './modules/reading.js';
import { initTelemetry, initTelemetryToggle } from './modules/telemetry.js';

document.addEventListener('DOMContentLoaded', () => {
  initMobileMenu();
  initAnchors();
  initReadingMode();
  initTelemetryToggle();
  initTelemetry();
});
