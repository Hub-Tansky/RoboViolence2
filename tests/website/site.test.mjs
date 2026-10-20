// Website download logic (website/site.js, ADR 0027). Run: node --test tests/website/site.test.mjs
import assert from 'node:assert/strict';
import test from 'node:test';
import { detectOS, newestRelease, pickAsset } from '../../website/site.js';

// Asset names as the release job in .github/workflows/build.yml publishes them
const release = {
  tag_name: 'v0.0.2',
  draft: false,
  prerelease: true,
  assets: [
    'roboviolence2-linux-x64.tar.gz',
    'roboviolence2-macos-arm64.dmg',
    'roboviolence2-v0.0.2-source.tar.gz',
    'roboviolence2-win-x64-msvc-setup.exe',
  ].map((name) => ({ name, size: 1, browser_download_url: `https://example.invalid/${name}` })),
};

test('detectOS maps desktop browsers and leaves phones to the list', () => {
  assert.equal(detectOS('Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/130.0'), 'windows');
  assert.equal(detectOS('Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) Safari/605.1.15'), 'macos');
  assert.equal(detectOS('Mozilla/5.0 (X11; Linux x86_64; rv:131.0) Firefox/131.0'), 'linux');
  assert.equal(detectOS('Mozilla/5.0 (X11; Ubuntu; Linux x86_64) Chrome/130.0', 'Linux'), 'linux');
  assert.equal(detectOS('Mozilla/5.0 (Linux; Android 14; Pixel 8) Chrome/130.0 Mobile'), null);
  assert.equal(detectOS('Mozilla/5.0 (iPhone; CPU iPhone OS 18_0 like Mac OS X) Safari/604.1'), null);
});

test('newestRelease takes the first non-draft release, pre-releases included', () => {
  const draft = { ...release, tag_name: 'v0.0.3', draft: true };
  assert.equal(newestRelease([draft, release]).tag_name, 'v0.0.2');
  assert.equal(newestRelease([]), null);
});

test('pickAsset gives each OS its installer, not the source tarball', () => {
  assert.equal(pickAsset(release, 'windows').name, 'roboviolence2-win-x64-msvc-setup.exe');
  assert.equal(pickAsset(release, 'macos').name, 'roboviolence2-macos-arm64.dmg');
  assert.equal(pickAsset(release, 'linux').name, 'roboviolence2-linux-x64.tar.gz');
  assert.equal(pickAsset(release, null), null);
  assert.equal(pickAsset(null, 'linux'), null);
});
