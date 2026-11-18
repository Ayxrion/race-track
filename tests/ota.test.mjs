import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
const source = name => readFileSync(new URL(`../${name}`, import.meta.url), 'utf8');

test('telemetry credentials are persisted before GitHub OTA can replace a USB build', () => {
  const sketch = source('race_track/race_track.ino');
  const save = sketch.indexOf('telemetryLoadConfig();');
  const update = sketch.indexOf('checkForOTAUpdate();');
  assert.ok(save >= 0 && update > save);
  assert.ok(!sketch.includes('RT_TELEMETRY_ALLOW_UPSTREAM_OTA'));
  const telemetry = source('race_track/telemetry.h');
  assert.match(telemetry, /putString\("url"/);
  assert.match(telemetry, /putString\("token"/);
  assert.match(telemetry, /getString\("url",telemetryUrl/);
  assert.match(telemetry, /getString\("token",telemetryToken/);
  assert.match(source('.gitignore'), /race_track\/telemetry_config\.h/);
});

test('release assets and OTA still agree, and CI does not embed the publishing token', () => {
  const workflow = source('.github/workflows/release.yml');
  assert.match(workflow, /GITHUB_REF_NAME#v/);
  assert.match(workflow, /gh release create "\$GITHUB_REF_NAME" firmware\.bin/);
  assert.ok(!workflow.includes('RT_TELEMETRY_TOKEN'));
  assert.match(source('race_track/version.h'), /GITHUB_ASSET_NAME "firmware\.bin"/);
  assert.match(source('race_track/ota_update.h'), /releases\/latest/);
});
