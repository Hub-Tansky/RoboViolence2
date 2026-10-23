// Website download logic and shared page chrome (website/, ADR 0029, 0035). Run: node --test tests/website/site.test.mjs
import assert from 'node:assert/strict';
import { existsSync, readFileSync } from 'node:fs';
import test from 'node:test';
import { pages as htmlPages, toMarkdown } from '../../tools/website-markdown.mjs';
import { DISCORD_INVITE, DOWNLOADS, PLATFORMS, RELEASES_PAGE, detectOS, downloadOffer, mailAddress, movedSection, newestRelease, pickAsset } from '../../website/site.js';

const root = new URL('../../', import.meta.url);
const read = (path) => readFileSync(new URL(path, root), 'utf8');
const pages = htmlPages(new URL('website/', root));

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

test('downloadOffer: installer, releases page fallback, or the install guide list', () => {
  assert.deepEqual(downloadOffer(release, 'macos'), {
    label: 'Download for macOS',
    href: 'https://example.invalid/roboviolence2-macos-arm64.dmg',
    note: 'v0.0.2 alpha, roboviolence2-macos-arm64.dmg, 1.0 MB. Apple Silicon (M1 or newer)',
  });
  assert.equal(downloadOffer(release, 'windows').note, 'v0.0.2 alpha, roboviolence2-win-x64-msvc-setup.exe, 1.0 MB');
  // No answer from the API (offline, rate limit) or no matching asset
  assert.equal(downloadOffer(null, 'linux').href, RELEASES_PAGE);
  assert.equal(downloadOffer({ ...release, assets: [] }, 'windows').href, RELEASES_PAGE);
  assert.equal(downloadOffer(release, null).href, DOWNLOADS);
});

test('the install guide names the files the release job publishes', () => {
  const install = read('website/install.html');
  for (const name of assetNames.filter((n) => !n.endsWith('-source.tar.gz'))) assert.ok(install.includes(name), name);
});

test('old links to the install section of the landing page go to the install guide', () => {
  assert.equal(movedSection('#install'), 'install.html');
  assert.equal(movedSection('#features'), null);
  assert.equal(movedSection(''), null);
});

test('pages without the download button never call the API', async () => {
  let fetched = 0;
  globalThis.document = { getElementById: () => null, querySelector: () => null, querySelectorAll: () => [] };
  globalThis.location = { hash: '' };
  globalThis.fetch = async () => fetched++;
  try {
    await import('../../website/site.js?page=about');
    await new Promise((resolve) => setTimeout(resolve, 0));
  } finally {
    delete globalThis.document;
    delete globalThis.location;
    delete globalThis.fetch;
  }
  assert.equal(fetched, 0);
});

test('every page has the same menu and footer', () => {
  const chrome = (page) => {
    const html = read(`website/${page}`);
    const part = (tag) => html.slice(html.indexOf(`<${tag} `), html.indexOf(`</${tag}>`));
    return (part('header') + part('footer')).replace(' aria-current="page"', '');
  };
  assert.ok(chrome('index.html').includes('<nav '), 'index.html has the menu');
  for (const page of pages) assert.equal(chrome(page), chrome('index.html'), page);
});

test('the privacy contact is assembled in the browser, never written in the page', () => {
  const privacy = read('website/privacy.html');
  const link = privacy.match(/<a data-mail data-user="([^"]+)" data-domain="([^"]+)"/);
  assert.ok(link, 'privacy.html has a data-mail link');
  const address = mailAddress(link[1], link[2]);
  assert.match(address, /^[^@\s]+@[^@\s]+\.[a-z]+$/);
  for (const page of pages) {
    assert.ok(!read(`website/${page}`).includes(address), `${page} holds the address in clear`);
    assert.ok(read(`website/${page}`).includes('href="privacy.html">Privacy policy</a>'), `${page} links the policy`);
  }
  for (const file of ['llms.txt', ...pages.map((p) => p.replace('.html', '.md'))]) {
    assert.ok(!read(`website/${file}`).includes(address), `${file} holds the address in clear`);
  }
  assert.equal(mailAddress('', 'example.org'), null);
});

test('every page loads the Tailwind CDN and the shared theme, and has its own canonical address', () => {
  assert.ok(!existsSync(new URL('website/style.css', root)), 'no stylesheet files: styles come from Tailwind');
  for (const page of pages) {
    const html = read(`website/${page}`);
    // Pinned: theme.js is written for the Tailwind 3 Play CDN (ADR 0035)
    const cdn = html.indexOf('<script src="https://cdn.tailwindcss.com/3.4.17"></script>');
    assert.ok(cdn > 0 && html.indexOf('<script src="theme.js"></script>') > cdn, `${page}: theme.js right after the CDN`);
    const canonical = page === 'index.html' ? 'https://roboviolence.com/' : `https://roboviolence.com/${page}`;
    assert.ok(html.includes(`<link rel="canonical" href="${canonical}">`), `${page}: canonical`);
    assert.ok(html.includes(`<meta property="og:url" content="${canonical}">`), `${page}: og:url`);
    assert.ok(html.includes('<meta property="og:image" content="https://roboviolence.com/img/og.png">'), `${page}: og:image`);
  }
  assert.ok(existsSync(new URL('website/img/og.png', root)));
});

test('the sitemap lists every page and robots.txt points at it', () => {
  const sitemap = read('website/sitemap.xml');
  const listed = [...sitemap.matchAll(/<loc>https:\/\/roboviolence\.com\/([^<]*)<\/loc>/g)].map((m) => m[1] || 'index.html');
  assert.deepEqual(listed.toSorted(), pages.toSorted());
  assert.match(read('website/robots.txt'), /^Sitemap: https:\/\/roboviolence\.com\/sitemap\.xml$/m);
});

test('every robots.txt group declares the content signals and a rule', () => {
  const groups = read('website/robots.txt')
    .split(/\n\s*\n/)
    .map((g) => g.split('\n').filter((l) => !l.startsWith('#')))
    .filter((g) => g.some((l) => l.startsWith('User-agent:')));
  assert.ok(groups.length >= 2);
  for (const g of groups) {
    assert.ok(g.includes('Content-Signal: search=yes, ai-input=yes, ai-train=yes'), g[0]);
    assert.ok(g.some((l) => /^(Allow|Disallow): \//.test(l)), g[0]);
  }
});

test('every page has a current Markdown copy, linked from the page and from llms.txt', () => {
  const llms = read('website/llms.txt');
  for (const page of pages) {
    const md = page.replace('.html', '.md');
    assert.equal(read(`website/${md}`), toMarkdown(read(`website/${page}`)), `${md} is stale: run node tools/website-markdown.mjs`);
    assert.ok(read(`website/${page}`).includes(`<link rel="alternate" type="text/markdown" href="${md}">`), page);
    assert.ok(llms.includes(`(https://roboviolence.com/${md})`), `llms.txt links ${md}`);
  }
  assert.ok(llms.includes(`(${DISCORD_INVITE})`), 'llms.txt has the current Discord invite');
  // Code blocks keep their text exactly: the env-file lines start in column 0, ready to paste
  assert.match(read('website/server.md'), /\n# join password, empty for none\nBV2_SV_PASSWORD=\n# admin login password\nBV2_ADMIN_PASS=\n/);
});

test('toMarkdown maps the pages\' HTML: own pages to .md, other sites unchanged', () => {
  const html = `<main>
  <div aria-hidden="true">decoration</div><section hidden>hook</section>
  <h2>Steps <span>now</span></h2>
  <p>Read <a href="install.html#data">the guide</a>, <a href="./">home</a>, <a href="https://www.gnu.org/licenses/gpl-3.0.html">GPL</a>,
    <a href="#top">top</a>, <a data-discord href="#">Discord</a>, <a>no link</a> and <strong>run</strong> <code>a &amp;&amp; b</code>.</p>
  <ol><li><span aria-hidden="true">1</span><div><h3>Get</h3><p>The file.</p></div></li></ol>
  <pre><code>tar -xzf x.tar.gz
cd x</code></pre>
  <a class="btn" href="server.html">Server</a>
</main>`;
  assert.equal(toMarkdown(html), [
    '## Steps now',
    'Read [the guide](https://roboviolence.com/install.md#data), [home](https://roboviolence.com/index.md), ' +
      '[GPL](https://www.gnu.org/licenses/gpl-3.0.html), [top](#top), ' + `[Discord](${DISCORD_INVITE}), no link and **run** \`a && b\`.`,
    '1. **Get** The file.',
    '```\ntar -xzf x.tar.gz\ncd x\n```',
    '[Server](https://roboviolence.com/server.md)',
  ].join('\n\n') + '\n');
});

test('the landing page describes the game as schema.org VideoGame data', () => {
  const json = read('website/index.html').match(/<script type="application\/ld\+json">([\s\S]*?)<\/script>/);
  assert.ok(json, 'index.html has JSON-LD');
  const data = JSON.parse(json[1]);
  assert.equal(data['@type'], 'VideoGame');
  assert.equal(data.offers.price, '0');
  assert.equal(data.url, 'https://roboviolence.com/');
  assert.ok(data.sameAs.includes(DISCORD_INVITE), 'JSON-LD has the current Discord invite');
});
