// Tailwind Play CDN theme shared by every page (ADR 0035). Loaded right after the CDN script, before the page renders.
// Colours are the site's palette; layout steps target 360 px phones, 768 px tablets and 1366-1920 px desktops,
// with content kept in a 1200 px container. The component classes hold the shared page chrome (menu, footer, page
// frame) and `.prose-rv` styles the plain HTML of the text pages.
// A block, so the palette constants stay out of the global scope the pages share.
{
  // Each page's inline <style> repeats ink and fg for the first paint (no white flash): change them there too.
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
    heroTop: '#1a1d29',
  };
  const rgba = (hex, alpha) => `rgba(${hex.slice(1).match(/../g).map((h) => parseInt(h, 16)).join(', ')}, ${alpha})`;

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
  const link = { color: c.link, textDecoration: 'underline', textUnderlineOffset: '3px', '&:hover': { color: c.fg } };

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
        fontFamily: { sans: ['system-ui', '-apple-system', '"Segoe UI"', 'Roboto', 'sans-serif'] },
        backgroundImage: {
          hero:
            `radial-gradient(ellipse 60% 70% at 75% 45%, ${rgba(c.red, 0.18)}, transparent 70%),` +
            `radial-gradient(ellipse 60% 70% at 55% 70%, ${rgba(c.blue, 0.2)}, transparent 70%),` +
            `linear-gradient(180deg, ${c.heroTop} 0%, ${c.ink} 100%)`,
          grid:
            'linear-gradient(rgba(255, 255, 255, 0.035) 1px, transparent 1px),' +
            'linear-gradient(90deg, rgba(255, 255, 255, 0.035) 1px, transparent 1px)',
        },
        backgroundSize: { cell: '48px 48px' },
      },
    },
    plugins: [
      ({ addBase, addComponents, theme }) => {
        const md = `@media (min-width: ${theme('screens.md')})`;
        // The menu folds into the toggled panel below 1024 px: tablets get the panel too.
        const lg = `@media (min-width: ${theme('screens.lg')})`;

        addBase({
          html: { scrollBehavior: 'smooth', colorScheme: 'dark' },
          body: { minHeight: '100vh', background: c.ink, color: c.fg, fontSize: '17px', lineHeight: '1.625', WebkitFontSmoothing: 'antialiased', MozOsxFontSmoothing: 'grayscale' },
          '[id]': { scrollMarginTop: '5rem' },
          code: { overflowWrap: 'anywhere', background: c.panel, border: `1px solid ${c.line}`, borderRadius: '4px', padding: '0 4px', fontSize: '0.9em' },
          pre: { background: c.panel, border: `1px solid ${c.line}`, borderRadius: '10px', padding: '14px 18px', overflowX: 'auto', fontSize: '0.95rem' },
          'pre code': { overflowWrap: 'normal', background: 'none', border: '0', padding: '0' },
          'a:focus-visible, button:focus-visible': { outline: `2px solid ${c.link}`, outlineOffset: '3px', borderRadius: '6px' },
        });
        addComponents({
          // The 1200 px content column with its side gutter, for the chrome and every page section
          '.site-container': {
            maxWidth: '1200px',
            marginLeft: 'auto',
            marginRight: 'auto',
            paddingLeft: '1rem',
            paddingRight: '1rem',
            [md]: { paddingLeft: '1.5rem', paddingRight: '1.5rem' },
          },
          // Page chrome: the sticky menu bar (a toggled panel below 1024 px) and the footer
          '.site-header': { position: 'sticky', top: '0', zIndex: '20', borderBottom: `1px solid ${c.line}`, background: rgba(c.ink, 0.85), backdropFilter: 'blur(8px)' },
          '.site-nav': { position: 'relative', display: 'flex', alignItems: 'center', gap: '2rem', paddingTop: '0.75rem', paddingBottom: '0.75rem' },
          '.site-brand': { marginRight: 'auto', display: 'flex', alignItems: 'center', gap: '0.75rem', fontSize: '1.125rem', fontWeight: '700', letterSpacing: '-0.01em', color: c.fg },
          '.site-brand img': { width: '2rem', height: '2rem', borderRadius: '0.5rem' },
          '.menu-button': {
            display: 'inline-flex',
            alignItems: 'center',
            justifyContent: 'center',
            width: '2.75rem',
            height: '2.75rem',
            borderRadius: '0.5rem',
            border: `1px solid ${c.line}`,
            color: c.fg,
            '&:hover': { borderColor: c.link },
            [lg]: { display: 'none' },
          },
          '.menu': {
            position: 'absolute',
            left: '0',
            right: '0',
            top: '100%',
            display: 'none',
            flexDirection: 'column',
            borderBottom: `1px solid ${c.line}`,
            background: c.ink,
            padding: '0 1rem 0.75rem',
            '&[data-open]': { display: 'flex' },
            [md]: { paddingLeft: '1.5rem', paddingRight: '1.5rem' },
            [lg]: { position: 'static', display: 'flex', flexDirection: 'row', alignItems: 'center', gap: '2rem', border: '0', background: 'transparent', padding: '0' },
          },
          '.nav-link': {
            color: c.muted,
            fontWeight: '500',
            padding: '0.75rem 0',
            transition: 'color 150ms',
            '&:hover, &[aria-current]': { color: c.fg },
            [lg]: { padding: '0' },
          },
          '.site-footer': { borderTop: `1px solid ${c.line}` },
          '.site-footer-inner': {
            display: 'flex',
            flexDirection: 'column',
            gap: '1rem',
            paddingTop: '2.5rem',
            paddingBottom: '2.5rem',
            fontSize: '0.875rem',
            color: c.muted,
            [md]: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between' },
          },
          '.footer-link': { ...link, color: c.muted },
          // Text pages: the grid pattern fading out under the menu, and the content container
          '.page': { position: 'relative' },
          '.page::before': {
            content: '""',
            position: 'absolute',
            inset: '0 0 auto 0',
            height: '20rem',
            backgroundImage: theme('backgroundImage.grid'),
            backgroundSize: theme('backgroundSize.cell'),
            maskImage: 'linear-gradient(180deg, #000, transparent)',
            pointerEvents: 'none',
          },
          '.page-body': { position: 'relative', paddingTop: '3rem', paddingBottom: '3rem', [md]: { paddingTop: '5rem', paddingBottom: '5rem' } },
          '.link': link,
          '.btn-primary': {
            ...button,
            background: c.blue,
            color: '#fff',
            fontSize: '1.1rem',
            boxShadow: `0 0 28px ${rgba(c.blue, 0.45)}`,
            '&:hover': { background: c.blueHover, transform: 'translateY(-1px)', boxShadow: `0 0 36px ${rgba(c.blue, 0.6)}` },
          },
          '.btn-secondary': {
            ...button,
            border: `1px solid ${c.line}`,
            background: rgba(c.panel, 0.7),
            color: c.fg,
            '&:hover': { borderColor: c.link, transform: 'translateY(-1px)' },
          },
          '.card': { background: c.panel, border: `1px solid ${c.line}`, borderRadius: '16px', padding: '1.5rem', boxShadow: '0 18px 40px -20px rgba(0, 0, 0, 0.7)' },
          '.notice': { borderLeft: `4px solid ${c.red}`, background: c.panel, padding: '14px 18px', borderRadius: '0 10px 10px 0' },
          '.prose-rv': { lineHeight: '1.7', maxWidth: '72ch' },
          '.prose-rv h1': { fontSize: 'clamp(2rem, 5vw, 3rem)', lineHeight: '1.1', fontWeight: '800', letterSpacing: '-0.02em', margin: '0 0 1.25rem' },
          '.prose-rv h2': { fontSize: '1.6rem', lineHeight: '1.25', fontWeight: '700', letterSpacing: '-0.01em', margin: '2.75rem 0 0.75rem' },
          '.prose-rv h3': { fontSize: '1.2rem', fontWeight: '700', margin: '1.75rem 0 0.5rem' },
          '.prose-rv p, .prose-rv ul, .prose-rv ol, .prose-rv pre': { margin: '0 0 1rem' },
          '.prose-rv ul': { listStyle: 'disc', paddingLeft: '1.4rem' },
          '.prose-rv ol': { listStyle: 'decimal', paddingLeft: '1.4rem' },
          '.prose-rv li': { margin: '0.3rem 0' },
          '.prose-rv a': link,
          '.prose-rv strong': { color: c.fg },
        });
      },
    ],
  };
}
