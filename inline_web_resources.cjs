const fs = require('fs');
const path = require('path');

const scriptDir = __dirname;
const distDir = path.join(scriptDir, 'dist');
const htmlPath = path.join(distDir, 'index.html');
const cssPath = path.join(distDir, 'assets', 'index.css');
const jsPath = path.join(distDir, 'assets', 'index.js');

if (!fs.existsSync(htmlPath) || !fs.existsSync(jsPath) || !fs.existsSync(cssPath)) {
  console.error('Error: dist files missing before inlining');
  process.exit(1);
}

let html = fs.readFileSync(htmlPath, 'utf8');
const css = fs.readFileSync(cssPath, 'utf8');
const js = fs.readFileSync(jsPath, 'utf8');

// Strip module and link tags targeting assets
html = html.replace(/<script type="module"[^>]*><\/script>/gi, '');
html = html.replace(/<link rel="stylesheet"[^>]*>/gi, '');

// Inject style into head and script before body closing tag
html = html.replace('</head>', `<style>\n${css}\n</style>\n</head>`);
html = html.replace('</body>', `<script>\n${js}\n</script>\n</body>`);

fs.writeFileSync(htmlPath, html, 'utf8');
console.log('Successfully inlined JS and CSS into dist/index.html!');
