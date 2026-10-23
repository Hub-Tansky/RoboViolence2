// Markdown copies of the website pages for AI agents (website/<page>.md beside <page>.html, ADR 0029).
// Run after changing a page: node tools/website-markdown.mjs. tests/website/site.test.mjs fails when a copy is stale.
// Handles the HTML the pages use: headings, paragraphs, flat lists, code blocks, links and inline emphasis inside <main>.
import { readdirSync, readFileSync, writeFileSync } from 'node:fs';
import { DISCORD_INVITE } from '../website/site.js';

const SITE = 'https://roboviolence.com/';
const VOID = new Set(['br', 'img', 'meta', 'link', 'input', 'hr']);
const BLOCKS = new Set(['h1', 'h2', 'h3', 'p', 'ul', 'ol', 'pre']);
const CONTAINERS = new Set(['main', 'section', 'div']);

const decode = (text) =>
  text.replace(/&lt;/g, '<').replace(/&gt;/g, '>').replace(/&quot;/g, '"').replace(/&#39;/g, "'").replace(/&amp;/g, '&');

// A tree of { tag, attrs, children } and strings, from well-formed HTML without comments.
function parse(html) {
  const root = { tag: 'root', attrs: {}, children: [] };
  const stack = [root];
  for (const [, text, close, open, rawAttrs] of html.matchAll(/([^<]+)|<\/(\w+)>|<(\w+)([^>]*)>/g)) {
    const node = stack.at(-1);
    if (text) node.children.push(decode(text));
    else if (close) stack.pop();
    else {
      const attrs = Object.fromEntries([...rawAttrs.matchAll(/([\w-]+)(?:="([^"]*)")?/g)].map(([, k, v]) => [k, v ?? '']));
      const child = { tag: open, attrs, children: [] };
      node.children.push(child);
      if (!VOID.has(open)) stack.push(child);
    }
  }
  return root;
}

function href(attrs) {
  if ('data-discord' in attrs) return DISCORD_INVITE;
  if (attrs.href.startsWith('#')) return attrs.href; // in-page
  return new URL(attrs.href.replace(/\.html(#|$)/, '.md$1').replace(/^\.\/$/, 'index.md'), SITE).href;
}

function inline(node) {
  if (typeof node === 'string') return node.replace(/\s+/g, ' ');
  const text = node.children.map(inline).join('');
  switch (node.tag) {
    case 'a': return `[${text.trim()}](${href(node.attrs)})`;
    case 'strong': return `**${text}**`;
    case 'code': case 'kbd': return `\`${text}\``;
    case 'br': return '  \n';
    case 'i': case 'img': return '';
    default: return text;
  }
}

const tidy = (text) => text.replace(/ *\n */g, '\n').replace(/ {2,}/g, ' ').trim();

const codeText = (node) => (typeof node === 'string' ? node : node.children.map(codeText).join(''));
const HEADINGS = { h1: '# ', h2: '## ', h3: '### ' };

// Markdown blocks of a container: block elements map one to one, loose inline content becomes a paragraph.
function blocks(node, out = []) {
  let run = [];
  const add = (text) => text && out.push(text);
  const flush = () => {
    add(tidy(run.map(inline).join('')));
    run = [];
  };
  for (const child of node.children) {
    if (typeof child !== 'string' && ('hidden' in child.attrs || child.attrs['aria-hidden'] === 'true')) continue;
    if (typeof child === 'string' || !(BLOCKS.has(child.tag) || CONTAINERS.has(child.tag))) {
      run.push(child);
      continue;
    }
    flush();
    if (child.tag in HEADINGS) add(HEADINGS[child.tag] + tidy(inline(child)));
    else if (child.tag === 'p') add(tidy(inline(child)));
    else if (child.tag === 'pre') add('```\n' + codeText(child).replace(/\n$/, '') + '\n```');
    else if (child.tag === 'ul' || child.tag === 'ol') {
      const items = child.children.filter((c) => typeof c !== 'string');
      const item = (li) => blocks(li).map((b) => b.replace(/^#+ (.*)/, '**$1**')).join(' '); // a heading in an item: bold
      add(items.map((li, i) => `${child.tag === 'ol' ? `${i + 1}.` : '-'} ${tidy(item(li))}`).join('\n'));
    } else blocks(child, out);
  }
  flush();
  return out;
}

export function toMarkdown(html) {
  const main = html.slice(html.indexOf('<main'), html.indexOf('</main>') + '</main>'.length).replace(/<!--[\s\S]*?-->/g, '');
  return blocks(parse(main)).join('\n\n') + '\n';
}

export const pages = (dir) => readdirSync(dir).filter((f) => f.endsWith('.html'));

if (import.meta.url === `file://${process.argv[1]}`) {
  const dir = new URL('../website/', import.meta.url);
  for (const page of pages(dir)) {
    writeFileSync(new URL(page.replace(/\.html$/, '.md'), dir), toMarkdown(readFileSync(new URL(page, dir), 'utf8')));
  }
}
