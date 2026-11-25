import fs from 'fs';
import path from 'path';

export interface OsTreeNode {
  name: string;
  path: string;
  type: 'dir' | 'file';
  size?: number;
  language?: string;
  children?: OsTreeNode[];
}

const NATURE_OS_ROOT = path.resolve(process.cwd(), 'nature-os');

function detectLanguage(filename: string): string {
  const ext = path.extname(filename).toLowerCase();
  switch (ext) {
    case '.c':
    case '.h':
      return 'c';
    case '.vala':
      return 'vala';
    case '.py':
      return 'python';
    case '.css':
      return 'css';
    case '.xml':
      return 'xml';
    case '.sh':
      return 'bash';
    case '.md':
      return 'markdown';
    case '.service':
    case '.conf':
      return 'ini';
    default:
      if (filename === 'meson.build') return 'meson';
      if (filename === 'control') return 'debian';
      return 'text';
  }
}

function walkTree(dirPath: string, relBase = ''): OsTreeNode[] {
  if (!fs.existsSync(dirPath)) return [];
  const entries = fs.readdirSync(dirPath, {withFileTypes: true});
  const sorted = entries.sort((a, b) => {
    if (a.isDirectory() && !b.isDirectory()) return -1;
    if (!a.isDirectory() && b.isDirectory()) return 1;
    return a.name.localeCompare(b.name);
  });

  const nodes: OsTreeNode[] = [];
  for (const entry of sorted) {
    const relPath = relBase ? `${relBase}/${entry.name}` : entry.name;
    const fullPath = path.join(dirPath, entry.name);
    if (entry.isDirectory()) {
      nodes.push({
        name: entry.name,
        path: relPath,
        type: 'dir',
        children: walkTree(fullPath, relPath),
      });
    } else {
      const stat = fs.statSync(fullPath);
      nodes.push({
        name: entry.name,
        path: relPath,
        type: 'file',
        size: stat.size,
        language: detectLanguage(entry.name),
      });
    }
  }
  return nodes;
}

export function getNatureOsTree(): OsTreeNode[] {
  return walkTree(NATURE_OS_ROOT);
}

export function readNatureOsFile(relativePath: string): {
  path: string;
  content: string;
  language: string;
  size: number;
} {
  if (!relativePath || typeof relativePath !== 'string') {
    throw new Error('Invalid file path');
  }
  const normalized = relativePath.replace(/^\/+/, '');
  const resolved = path.resolve(NATURE_OS_ROOT, normalized);

  if (!resolved.startsWith(NATURE_OS_ROOT + path.sep)) {
    throw new Error('Access denied outside /nature-os');
  }
  if (!fs.existsSync(resolved) || fs.statSync(resolved).isDirectory()) {
    throw new Error('File not found');
  }

  const content = fs.readFileSync(resolved, 'utf-8');
  return {
    path: normalized,
    content,
    language: detectLanguage(path.basename(resolved)),
    size: Buffer.byteLength(content, 'utf-8'),
  };
}
