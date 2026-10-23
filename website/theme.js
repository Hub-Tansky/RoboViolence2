// Tailwind Play CDN theme shared by every page (ADR 0029). Loaded right after the CDN script, before the page renders.
// Colours are the site's palette; layout steps target 360 px phones, 768 px tablets and 1366-1920 px desktops,
// with content kept in a 1200 px container. `.prose-rv` styles the plain HTML of the text pages.
// A block, so the palette constant stays out of the global scope the pages share.
{
  const c = {
    ink: '#12141b',
    panel: '#1b1e28',
    line: '#2c3040',
    fg: '#e6e8ef',
    muted: '#9aa0b4',
    link: '#7fb0ff',
    red: '#e5383b',
    blue: '#2f7df6',
    blueHover: '#4b90ff',
  };

  tailwind.config = {
    theme: {
      screens: { sm: '480px', md: '768px', lg: '1024px', xl: '1366px' },
      extend: {
        colors: {
          ink: c.ink,
          panel: c.panel,
          line: c.line,
          fg: c.fg,
          muted: c.muted,
          link: c.link,
          brand: { red: c.red, blue: c.blue },
        },
        maxWidth: { site: '1200px' },
        fontFamily: { sans: ['system-ui', '-apple-system', '"Segoe UI"', 'Roboto', 'sans-serif'] },
        backgroundImage: {
          hero:
            'radial-gradient(ellipse 60% 70% at 75% 45%, rgba(229, 56, 59, 0.18), transparent 70%),' +
            'radial-gradient(ellipse 60% 70% at 55% 70%, rgba(47, 125, 246, 0.2), transparent 70%),' +
            `linear-gradient(180deg, #1a1d29 0%, ${c.ink} 100%)`,
          grid:
            'linear-gradient(rgba(255, 255, 255, 0.035) 1px, transparent 1px),' +
            'linear-gradient(90deg, rgba(255, 255, 255, 0.035) 1px, transparent 1px)',
        },
        backgroundSize: { cell: '48px 48px' },
      },
    },
    plugins: [
      ({ addBase, addComponents }) => {
        addBase({
          html: { scrollBehavior: 'smooth', colorScheme: 'dark' },
          '[id]': { scrollMarginTop: '5rem' },
          code: {
            overflowWrap: 'anywhere',
            background: c.panel,
            border: `1px solid ${c.line}`,
            borderRadius: '4px',
            padding: '0 4px',
            fontSize: '0.9em',
          },
          pre: {
            background: c.panel,
            border: `1px solid ${c.line}`,
            borderRadius: '10px',
            padding: '14px 18px',
            overflowX: 'auto',
            fontSize: '0.95rem',
          },
          'pre code': { overflowWrap: 'normal', background: 'none', border: '0', padding: '0' },
          'a:focus-visible, button:focus-visible': { outline: `2px solid ${c.link}`, outlineOffset: '3px', borderRadius: '6px' },
        });
        const button = {
          display: 'inline-flex',
          alignItems: 'center',
          justifyContent: 'center',
          gap: '0.6rem',
          fontWeight: '700',
          borderRadius: '12px',
          padding: '0.9rem 1.6rem',
          transition: 'background-color 150ms, border-color 150ms, transform 150ms, box-shadow 150ms',
        };
        addComponents({
          '.nav-link': { color: c.muted, fontWeight: '500', transition: 'color 150ms' },
          '.nav-link:hover, .nav-link[aria-current]': { color: c.fg },
          '.footer-link': { color: c.muted, textDecoration: 'underline', textUnderlineOffset: '3px' },
          '.footer-link:hover': { color: c.fg },
          '.btn-primary': { ...button, background: c.blue, color: '#fff', fontSize: '1.1rem', boxShadow: '0 0 28px rgba(47, 125, 246, 0.45)' },
          '.btn-primary:hover': { background: c.blueHover, transform: 'translateY(-1px)', boxShadow: '0 0 36px rgba(47, 125, 246, 0.6)' },
          '.btn-secondary': { ...button, border: `1px solid ${c.line}`, background: 'rgba(27, 30, 40, 0.7)', color: c.fg },
          '.btn-secondary:hover': { borderColor: c.link, transform: 'translateY(-1px)' },
          '.card': {
            background: c.panel,
            border: `1px solid ${c.line}`,
            borderRadius: '16px',
            padding: '1.5rem',
            boxShadow: '0 18px 40px -20px rgba(0, 0, 0, 0.7)',
          },
          '.prose-rv': { lineHeight: '1.7', maxWidth: '72ch' },
          '.prose-rv h1': { fontSize: 'clamp(2rem, 5vw, 3rem)', lineHeight: '1.1', fontWeight: '800', letterSpacing: '-0.02em', margin: '0 0 1.25rem' },
          '.prose-rv h2': { fontSize: '1.6rem', lineHeight: '1.25', fontWeight: '700', letterSpacing: '-0.01em', margin: '2.75rem 0 0.75rem' },
          '.prose-rv h3': { fontSize: '1.2rem', fontWeight: '700', margin: '1.75rem 0 0.5rem' },
          '.prose-rv p, .prose-rv ul, .prose-rv ol, .prose-rv pre': { margin: '0 0 1rem' },
          '.prose-rv ul': { listStyle: 'disc', paddingLeft: '1.4rem' },
          '.prose-rv ol': { listStyle: 'decimal', paddingLeft: '1.4rem' },
          '.prose-rv li': { margin: '0.3rem 0' },
          '.prose-rv a': { color: c.link, textDecoration: 'underline', textUnderlineOffset: '3px' },
          '.prose-rv a:hover': { color: c.fg },
          '.prose-rv strong': { color: c.fg },
          '.notice': {
            borderLeft: `4px solid ${c.red}`,
            background: c.panel,
            padding: '14px 18px',
            borderRadius: '0 10px 10px 0',
          },
        });
      },
    ],
  };
}
