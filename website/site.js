// Robo Violence 2 website: download button, download list, Discord link (ADR 0027).
// The pure functions are exported for tests/website/site.test.mjs; the DOM part runs only in a browser.

export const REPO = 'Hub-Tansky/RoboViolence2';
export const RELEASES_PAGE = `https://github.com/${REPO}/releases`;
const RELEASES_API = `https://api.github.com/repos/${REPO}/releases?per_page=10`;

// The invite link, once the Discord server exists ("Discord server setup" item). Empty: the menu entry says "soon".
export const DISCORD_INVITE = '';

// The installer each OS gets from the main button; release asset names come from build.yml (release job).
export const PLATFORMS = {
  windows: { label: 'Windows', suffix: '-setup.exe' },
  macos: { label: 'macOS', suffix: '.dmg' },
  linux: { label: 'Linux', suffix: '-linux-x64.tar.gz' },
};

// 'windows', 'macos', 'linux' or null (phones, tablets, unknown).
export function detectOS(userAgent, platform = '') {
  const ua = `${userAgent} ${platform}`;
  if (/Android|iPhone|iPad|iPod|CrOS/i.test(ua)) return null;
  if (/Windows|Win64|Win32/i.test(ua)) return 'windows';
  if (/Mac/i.test(ua)) return 'macos';
  if (/Linux|X11/i.test(ua)) return 'linux';
  return null;
}

// The API lists releases newest first, pre-releases included.
export function newestRelease(releases) {
  return releases.find((r) => !r.draft) ?? null;
}

export function pickAsset(release, os) {
  const platform = PLATFORMS[os];
  if (!release || !platform) return null;
  return release.assets.find((a) => a.name.endsWith(platform.suffix)) ?? null;
}

export function formatSize(bytes) {
  return `${(bytes / 1048576).toFixed(1)} MB`;
}

function setUpDownload(release) {
  const os = detectOS(navigator.userAgent, navigator.userAgentData?.platform);
  const button = document.getElementById('download-button');
  const note = document.getElementById('download-note');
  if (!button) return;
  if (!os) {
    button.textContent = 'Download';
    button.href = '#install';
    note.textContent = 'Windows, macOS and Linux. Pick a file below.';
    return;
  }
  button.textContent = `Download for ${PLATFORMS[os].label}`;
  const asset = pickAsset(release, os);
  if (!asset) {
    button.href = RELEASES_PAGE;
    note.textContent = 'Opens the GitHub releases page.';
    return;
  }
  button.href = asset.browser_download_url;
  const kind = release.prerelease ? 'alpha' : 'release';
  note.textContent = `${release.tag_name} ${kind}, ${asset.name}, ${formatSize(asset.size)}`;
  if (os === 'macos') note.textContent += '. Apple Silicon (M1 or newer)';
}

function fillDownloadList(release) {
  const list = document.getElementById('download-list');
  if (!list || !release) return;
  list.replaceChildren(
    ...release.assets.map((a) => {
      const item = document.createElement('li');
      const link = document.createElement('a');
      link.href = a.browser_download_url;
      link.textContent = a.name;
      item.append(link, ` (${formatSize(a.size)})`);
      return item;
    }),
  );
  const tag = document.getElementById('release-tag');
  if (tag) tag.textContent = `${release.tag_name}${release.prerelease ? ' (alpha)' : ''}`;
}

function setUpDiscord() {
  for (const link of document.querySelectorAll('[data-discord]')) {
    if (DISCORD_INVITE) {
      link.href = DISCORD_INVITE;
    } else {
      link.removeAttribute('href');
      link.title = 'Coming soon';
      link.classList.add('soon');
    }
  }
}

async function main() {
  setUpDiscord();
  let release = null;
  try {
    const response = await fetch(RELEASES_API, { headers: { Accept: 'application/vnd.github+json' } });
    if (response.ok) release = newestRelease(await response.json());
  } catch {
    // Offline or rate-limited: the links keep pointing at the releases page.
  }
  setUpDownload(release);
  fillDownloadList(release);
}

if (typeof document !== 'undefined') main();
