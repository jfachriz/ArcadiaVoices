/**
 * Tiny HTTP bridge for click-to-code.
 * The browser calls this, and it runs `code --goto` on the file.
 */
const http = require('http');
const { exec } = require('child_process');
const path = require('path');
const url = require('url');

const PORT = 5123;
const PROJECT_ROOT = '/Users/jfachriz/Documents/VST_Plugins/ArcadiaVoices';

const server = http.createServer((req, res) => {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET');

  const parsed = url.parse(req.url, true);
  const { file, line } = parsed.query;

  if (!file) {
    res.writeHead(400, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ error: 'Missing "file" param' }));
    return;
  }

  const fullPath = path.resolve(PROJECT_ROOT, file);
  const goto = line ? `${fullPath}:${line}` : fullPath;

  exec(`code --goto "${goto}"`, (err, stdout, stderr) => {
    if (err) {
      console.error(`Error: ${err.message}`);
      res.writeHead(500);
      res.end(JSON.stringify({ error: err.message }));
      return;
    }
    console.log(`Opened: ${goto}`);
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ opened: goto }));
  });
});

server.listen(PORT, () => {
  console.log(`Click-to-code bridge running on http://localhost:${PORT}`);
  console.log(`Project root: ${PROJECT_ROOT}`);
});
