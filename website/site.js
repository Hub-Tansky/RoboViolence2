// Robo Violence 2 website: download button, download list, Discord link, mobile menu, icons (ADR 0029), contact address.
// The exports are for tests/website/site.test.mjs; the DOM part runs only in a browser.

const REPO = 'Hub-Tansky/RoboViolence2';
export const RELEASES_PAGE = `https://github.com/${REPO}/releases`;
const RELEASES_API = `https://api.github.com/repos/${REPO}/releases?per_page=10`;

// The Discord invite (never expires, no use limit). Empty: the menu entry says "soon".
export const DISCORD_INVITE = 'https://discord.gg/gRehxx43z';

// The installer each OS gets from the main button; release asset names come from build.yml (release job).
export const PLATFORMS = {
  windows: { label: 'Windows', suffix: '-setup.exe' },
  macos: { label: 'macOS', suffix: '.dmg', note: 'Apple Silicon (M1 or newer)' },
  linux: { label: 'Linux', suffix: '-linux-x64.tar.gz' },
};

// 'windows', 'macos', 'linux' or null (phones, Android tablets, unknown). iPadOS Safari reports a Mac and gets the .dmg.
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

function formatSize(bytes) {
  return `${(bytes / 1048576).toFixed(1)} MB`;
}

// The install guide's file list, where phones and unknown systems are sent.
export const DOWNLOADS = 'install.html#downloads';

// What the download button offers: the installer for this OS, the releases page, or the install guide's list.
// The note (version, file, size) is shown on the install guide only.
export function downloadOffer(release, os) {
  const platform = PLATFORMS[os];
  if (!platform) return { label: 'Download', href: DOWNLOADS, note: 'Windows, macOS and Linux. Pick a file below.' };
  const label = `Download for ${platform.label}`;
  const asset = pickAsset(release, os);
  if (!asset) return { label, href: RELEASES_PAGE, note: 'Opens the GitHub releases page.' };
  const kind = release.prerelease ? 'alpha' : 'release';
  const note = [`${release.tag_name} ${kind}, ${asset.name}, ${formatSize(asset.size)}`, platform.note].filter(Boolean).join('. ');
  return { label, href: asset.browser_download_url, note };
}

function setUpDownload(button, release) {
  const offer = downloadOffer(release, detectOS(navigator.userAgent, navigator.userAgentData?.platform));
  button.querySelector('[data-label]').textContent = offer.label;
  button.href = offer.href;
  const note = document.getElementById('download-note');
  if (note) note.textContent = offer.note;
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

// The menu button on phones opens and closes the menu; a chosen link or Escape closes it.
function setUpMenu() {
  const button = document.querySelector('[data-menu-button]');
  const menu = document.getElementById('menu');
  if (!button || !menu) return;
  const show = (open) => {
    menu.toggleAttribute('data-open', open);
    button.setAttribute('aria-expanded', String(open));
  };
  button.addEventListener('click', () => show(!menu.hasAttribute('data-open')));
  menu.addEventListener('click', (event) => event.target.closest('a') && show(false));
  document.addEventListener('keydown', (event) => event.key === 'Escape' && show(false));
}

// Links to the old install section of the landing page (roboviolence.com/#install) go to the install guide.
export function movedSection(hash) {
  return hash === '#install' ? 'install.html' : null;
}

// A mail address assembled in the browser from data-user and data-domain: the page source never holds it, so
// address-harvesting bots that read HTML don't find it.
export function mailAddress(user, domain) {
  return user && domain ? `${user}@${domain}` : null;
}

function setUpMail() {
  for (const link of document.querySelectorAll('[data-mail]')) {
    const address = mailAddress(link.dataset.user, link.dataset.domain);
    if (!address) continue;
    link.href = `mailto:${address}`;
    link.textContent = address;
  }
}

async function main() {
  const moved = document.body.dataset.page === 'home' && movedSection(location.hash);
  if (moved) {
    location.replace(moved);
    return;
  }
  globalThis.lucide?.createIcons();
  setUpMenu();
  setUpDiscord();
  setUpMail();
  // Only the landing page and the install guide ask the API: 60 requests per hour per visitor IP.
  const button = document.getElementById('download-button');
  if (!button) return;
  let release = null;
  try {
    const response = await fetch(RELEASES_API, { headers: { Accept: 'application/vnd.github+json' } });
    if (response.ok) release = newestRelease(await response.json());
  } catch {
    // Offline or rate-limited: the links keep pointing at the releases page.
  }
  setUpDownload(button, release);
  fillDownloadList(release);
}

if (typeof document !== 'undefined') main();
