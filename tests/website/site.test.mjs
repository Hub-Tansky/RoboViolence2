// Website download logic and shared page chrome (website/, ADR 0029). Run: node --test tests/website/site.test.mjs
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import test from 'node:test';
import { PLATFORMS, RELEASES_PAGE, detectOS, downloadOffer, newestRelease, pickAsset } from '../../website/site.js';

const root = new URL('../../', import.meta.url);
const read = (path) => readFileSync(new URL(path, root), 'utf8');

// The asset names the release job publishes: the dist/ files of `gh release create` in build.yml
const releaseJob = read('.github/workflows/build.yml');
const assetNames = [...releaseJob.matchAll(/dist\/(roboviolence2-[\w.${}-]+)/g)]
  .map((m) => m[1].replace('${GITHUB_REF_NAME}', 'v0.0.2'))
  .filter((name, i, all) => all.indexOf(name) === i);

const release = {
  tag_name: 'v0.0.2',
  draft: false,
  prerelease: true,
  assets: assetNames.map((name) => ({ name, size: 1048576, browser_download_url: `https://example.invalid/${name}` })),
};

test('the release job publishes exactly one installer per OS', () => {
  assert.ok(assetNames.length >= 4, `asset names from build.yml: ${assetNames}`);
  for (const [os, { suffix }] of Object.entries(PLATFORMS)) {
    assert.equal(assetNames.filter((n) => n.endsWith(suffix)).length, 1, `${os}: ${suffix} in ${assetNames}`);
  }
  assert.equal(pickAsset(release, 'windows').name, 'roboviolence2-win-x64-msvc-setup.exe');
  assert.equal(pickAsset(release, 'macos').name, 'roboviolence2-macos-arm64.dmg');
  assert.equal(pickAsset(release, 'linux').name, 'roboviolence2-linux-x64.tar.gz');
});

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

test('downloadOffer: installer, releases page fallback, or the list', () => {
  assert.deepEqual(downloadOffer(release, 'macos'), {
    label: 'Download for macOS',
    href: 'https://example.invalid/roboviolence2-macos-arm64.dmg',
    note: 'v0.0.2 alpha, roboviolence2-macos-arm64.dmg, 1.0 MB. Apple Silicon (M1 or newer)',
  });
  assert.equal(downloadOffer(release, 'windows').note, 'v0.0.2 alpha, roboviolence2-win-x64-msvc-setup.exe, 1.0 MB');
  // No answer from the API (offline, rate limit) or no matching asset
  assert.equal(downloadOffer(null, 'linux').href, RELEASES_PAGE);
  assert.equal(downloadOffer({ ...release, assets: [] }, 'windows').href, RELEASES_PAGE);
  assert.equal(downloadOffer(release, null).href, '#install');
});

test('the install instructions name the files the release job publishes', () => {
  const index = read('website/index.html');
  for (const name of assetNames.filter((n) => !n.endsWith('-source.tar.gz'))) assert.ok(index.includes(name), name);
});

test('pages without the download button never call the API', async () => {
  let fetched = 0;
  globalThis.document = { getElementById: () => null, querySelectorAll: () => [] };
  globalThis.fetch = async () => fetched++;
  try {
    await import('../../website/site.js?page=about');
    await new Promise((resolve) => setTimeout(resolve, 0));
  } finally {
    delete globalThis.document;
    delete globalThis.fetch;
  }
  assert.equal(fetched, 0);
});

test('every page has the same menu and footer', () => {
  const chrome = (page) => {
    const html = read(`website/${page}`);
    const part = (tag) => html.slice(html.indexOf(`<${tag}>`), html.indexOf(`</${tag}>`));
    return (part('header') + part('footer')).replace(' aria-current="page"', '').replace('href="./#install"', 'href="#install"');
  };
  for (const page of ['about.html', 'server.html']) assert.equal(chrome(page), chrome('index.html'), page);
});
