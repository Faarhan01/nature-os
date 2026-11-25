import express, {Request, Response} from 'express';
import fs from 'fs';
import http from 'http';
import os from 'os';
import path from 'path';
import {createServer as createViteServer} from 'vite';
import {WebSocketServer} from 'ws';
import {getNatureOsTree, readNatureOsFile} from './osInspector.ts';
import {NaturePreviewerHost} from './previewerSession.ts';

export async function startNatureSiteServer() {
  const app = express();
  const server = http.createServer(app);
  const port = Number(process.env.PORT) || 3000;
  const isProd = process.env.NODE_ENV === 'production';

  const previewer = new NaturePreviewerHost();
  previewer.start();

  const wss = new WebSocketServer({noServer: true});
  wss.on('connection', (ws) => {
    previewer.addClient(ws);
  });

  server.on('upgrade', (request, socket, head) => {
    const reqUrl = request.url || '';
    if (reqUrl.startsWith('/api/os-stream')) {
      wss.handleUpgrade(request, socket, head, (ws) => {
        wss.emit('connection', ws, request);
      });
    } else {
      socket.destroy();
    }
  });

  app.use(express.json());

  app.get('/api/health', (_req: Request, res: Response) => {
    res.json({status: 'ok', distro: 'Nature OS 24.04 LTS', runtime: previewer.getStats()});
  });

  app.get('/api/nature-os/tree', (_req: Request, res: Response) => {
    try {
      const tree = getNatureOsTree();
      res.json({root: '/nature-os', tree});
    } catch (err) {
      res.status(500).json({
        error: err instanceof Error ? err.message : 'Failed to inspect /nature-os',
      });
    }
  });

  app.get('/api/nature-os/file', (req: Request, res: Response) => {
    const filePath = typeof req.query.path === 'string' ? req.query.path : '';
    try {
      const fileData = readNatureOsFile(filePath);
      res.json(fileData);
    } catch (err) {
      const message = err instanceof Error ? err.message : 'File read error';
      const status = message.includes('denied') ? 403 : message.includes('not found') ? 404 : 400;
      res.status(status).json({error: message});
    }
  });

  app.get('/api/system/host-metrics', (_req: Request, res: Response) => {
    res.json({
      hostArch: os.arch(),
      hostCpus: os.cpus().length,
      uptimeSeconds: Math.floor(os.uptime()),
      distroBase: 'Ubuntu 24.04.1 LTS (Noble Numbat)',
      kernelTarget: '6.8.0-45-lowlatency',
      compositor: 'nature-compositor (C17 + Cairo/wlroots)',
      runtime: previewer.getStats(),
    });
  });

  if (!isProd) {
    const vite = await createViteServer({
      server: {middlewareMode: true},
      appType: 'spa',
    });
    app.use(vite.middlewares);
  } else {
    const distPath = path.resolve(process.cwd(), 'dist');
    app.use(express.static(distPath));
    app.get('*', (_req: Request, res: Response) => {
      const indexFile = path.join(distPath, 'index.html');
      if (fs.existsSync(indexFile)) {
        res.sendFile(indexFile);
      } else {
        res.status(404).send('Build output not found. Run npm run build.');
      }
    });
  }

  const cleanup = () => {
    previewer.shutdown();
    process.exit(0);
  };
  process.on('SIGINT', cleanup);
  process.on('SIGTERM', cleanup);

  server.listen(port, '0.0.0.0', () => {
    console.log(`Nature OS viewer server listening on http://0.0.0.0:${port}`);
  });
}
