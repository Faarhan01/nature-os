import 'dotenv/config';
import {startNatureSiteServer} from './site/backend/src/server.ts';

startNatureSiteServer().catch((err) => {
  console.error('Failed to start Nature OS server:', err);
  process.exit(1);
});
