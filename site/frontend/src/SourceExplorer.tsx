import React, {useEffect, useMemo, useState} from 'react';
import {ChevronDown, ChevronRight, FileCode2, Folder, Search} from 'lucide-react';

interface OsTreeNode {
  name: string;
  path: string;
  type: 'dir' | 'file';
  size?: number;
  language?: string;
  children?: OsTreeNode[];
}

interface LoadedFile {
  path: string;
  content: string;
  language: string;
  size: number;
}

interface SourceExplorerProps {
  initialFile?: string;
}

export function SourceExplorer({initialFile}: SourceExplorerProps) {
  const [tree, setTree] = useState<OsTreeNode[]>([]);
  const [search, setSearch] = useState('');
  const [expanded, setExpanded] = useState<Record<string, boolean>>({
    compositor: true,
    'compositor/src': true,
    shell: true,
    'shell/src': true,
    apps: true,
    'apps/nature-files': true,
    'apps/nature-monitor': true,
    'apps/nature-store': true,
    'apps/nature-terminal': true,
    'apps/nature-settings': true,
    'apps/nature-browser': true,
    'apps/nature-media': true,
    include: true,
    packaging: true,
    'packaging/debian': true,
    systemd: true,
    scripts: true,
  });
  const [selectedFile, setSelectedFile] = useState<LoadedFile | null>(null);
  const [loadingTree, setLoadingTree] = useState(true);
  const [loadingFile, setLoadingFile] = useState(false);

  useEffect(() => {
    fetch('/api/nature-os/tree')
      .then((res) => res.json())
      .then((data) => {
        setTree(data.tree || []);
        setLoadingTree(false);
        const startPath = initialFile || 'shell/src/shell.c';
        openFile(startPath);
      })
      .catch(() => setLoadingTree(false));
  }, [initialFile]);

  const openFile = (relPath: string) => {
    setLoadingFile(true);
    fetch(`/api/nature-os/file?path=${encodeURIComponent(relPath)}`)
      .then((res) => res.json())
      .then((data) => {
        if (data && typeof data.content === 'string') {
          setSelectedFile(data);
        }
        setLoadingFile(false);
      })
      .catch(() => setLoadingFile(false));
  };

  const toggleDir = (dirPath: string) => {
    setExpanded((prev) => ({...prev, [dirPath]: !prev[dirPath]}));
  };

  const filterTree = (nodes: OsTreeNode[], query: string): OsTreeNode[] => {
    if (!query) return nodes;
    const q = query.toLowerCase();
    const result: OsTreeNode[] = [];

    for (const node of nodes) {
      if (node.type === 'file') {
        if (node.name.toLowerCase().includes(q) || node.path.toLowerCase().includes(q)) {
          result.push(node);
        }
      } else if (node.children) {
        const filteredChildren = filterTree(node.children, query);
        if (filteredChildren.length > 0 || node.name.toLowerCase().includes(q)) {
          result.push({...node, children: filteredChildren});
        }
      }
    }
    return result;
  };

  const visibleTree = useMemo(() => filterTree(tree, search), [tree, search]);

  const renderNodes = (nodes: OsTreeNode[], depth = 0) => {
    return nodes.map((node) => {
      if (node.type === 'dir') {
        const isOpen = search ? true : !!expanded[node.path];
        return (
          <div key={node.path}>
            <button
              type="button"
              onClick={() => toggleDir(node.path)}
              style={{paddingLeft: `${12 + depth * 14}px`}}
              className="flex w-full items-center gap-1.5 py-1.5 pr-3 text-left text-xs font-medium text-neutral-300 hover:bg-white/5 transition-colors"
            >
              {isOpen ? (
                <ChevronDown className="h-3.5 w-3.5 shrink-0 text-neutral-500" />
              ) : (
                <ChevronRight className="h-3.5 w-3.5 shrink-0 text-neutral-500" />
              )}
              <Folder className="h-3.5 w-3.5 shrink-0 text-emerald-400" />
              <span className="truncate">{node.name}</span>
            </button>
            {isOpen && node.children && renderNodes(node.children, depth + 1)}
          </div>
        );
      }

      const active = selectedFile?.path === node.path;
      return (
        <button
          key={node.path}
          type="button"
          onClick={() => openFile(node.path)}
          style={{paddingLeft: `${26 + depth * 14}px`}}
          className={`flex w-full items-center justify-between gap-2 py-1.5 pr-3 text-left text-xs transition-colors ${
            active
              ? 'bg-emerald-500/15 text-emerald-300 font-medium'
              : 'text-neutral-400 hover:bg-white/5 hover:text-neutral-200'
          }`}
        >
          <span className="flex items-center gap-1.5 min-w-0">
            <FileCode2 className="h-3.5 w-3.5 shrink-0 opacity-75" />
            <span className="truncate">{node.name}</span>
          </span>
          {node.language && (
            <span className="shrink-0 font-mono text-[10px] text-neutral-500">{node.language}</span>
          )}
        </button>
      );
    });
  };

  const lineCount = selectedFile?.content ? selectedFile.content.split('\n').length : 0;

  return (
    <div className="flex h-full w-full flex-col md:flex-row bg-[#070B09] text-[#E8EFEA] overflow-hidden">
      {/* Tree Sidebar */}
      <aside className="h-56 md:h-auto w-full md:w-72 lg:w-80 shrink-0 border-b md:border-b-0 md:border-r border-white/10 bg-[#0B100E] flex flex-col">
        <div className="border-b border-white/10 px-4 py-3">
          <div className="flex items-center justify-between">
            <div className="text-xs font-semibold text-neutral-200">/nature-os Source Tree</div>
            <span className="text-[10px] font-mono text-emerald-400">
              Ubuntu Noble
            </span>
          </div>
          <div className="mt-2 relative">
            <Search className="absolute left-2.5 top-2.5 h-3.5 w-3.5 text-neutral-500" />
            <input
              type="text"
              value={search}
              onChange={(e) => setSearch(e.target.value)}
              placeholder="Search files (e.g. scheduler, shell)..."
              className="w-full rounded-lg border border-white/10 bg-black/40 pl-8 pr-3 py-1.5 text-xs text-neutral-200 placeholder:text-neutral-500 focus:outline-none focus:border-emerald-500/50"
            />
          </div>
        </div>
        <div className="flex-1 overflow-y-auto py-2 nature-scroll">
          {loadingTree ? (
            <div className="px-4 py-3 text-xs text-neutral-500">Scanning /nature-os...</div>
          ) : visibleTree.length === 0 ? (
            <div className="px-4 py-3 text-xs text-neutral-500">No matching files found.</div>
          ) : (
            renderNodes(visibleTree)
          )}
        </div>
      </aside>

      {/* Code Viewer */}
      <main className="flex flex-1 flex-col min-w-0 min-h-0 bg-[#060A08]">
        {selectedFile ? (
          <>
            <div className="flex items-center justify-between border-b border-white/10 px-4 sm:px-5 py-2.5 sm:py-3 bg-[#0A0F0D] overflow-x-auto">
              <div className="flex items-center gap-2 sm:gap-3 text-xs whitespace-nowrap">
                <span className="font-mono font-medium text-emerald-300">
                  /nature-os/{selectedFile.path}
                </span>
                <span className="text-neutral-600">·</span>
                <span className="text-neutral-400">{selectedFile.language}</span>
                <span className="text-neutral-600">·</span>
                <span className="font-mono text-neutral-400 tabular-nums">
                  {lineCount} lines
                </span>
                <span className="text-neutral-600">·</span>
                <span className="font-mono text-neutral-400 tabular-nums">
                  {selectedFile.size.toLocaleString()} bytes
                </span>
              </div>
            </div>
            <div className="flex-1 overflow-auto p-4 nature-scroll">
              {loadingFile ? (
                <div className="text-xs text-neutral-500 p-4">Loading source...</div>
              ) : (
                <div className="flex font-mono text-[12.5px] leading-relaxed">
                  {/* Line numbers column */}
                  <div className="select-none pr-4 text-right text-neutral-600 border-r border-white/5 mr-4 font-mono text-xs tabular-nums">
                    {Array.from({length: lineCount}, (_, i) => (
                      <div key={i + 1}>{i + 1}</div>
                    ))}
                  </div>
                  {/* Code text */}
                  <pre className="text-[#D7E4DC] whitespace-pre flex-1 overflow-x-auto">
                    {selectedFile.content}
                  </pre>
                </div>
              )}
            </div>
          </>
        ) : (
          <div className="flex flex-1 items-center justify-center text-xs text-neutral-500">
            Select a file from /nature-os to inspect its C/C++ implementation.
          </div>
        )}
      </main>
    </div>
  );
}
