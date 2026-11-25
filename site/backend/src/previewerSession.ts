import {ChildProcessWithoutNullStreams, execSync, spawn} from 'child_process';
import fs from 'fs';
import path from 'path';
import {WebSocket} from 'ws';

const PREVIEWER_DIR = path.resolve(process.cwd(), 'previewer');
const BINARY_PATH = path.join(PREVIEWER_DIR, 'build', 'nature-previewer');
const ASSETS_DIR = path.join(PREVIEWER_DIR, 'assets');

export class NaturePreviewerHost {
  private proc: ChildProcessWithoutNullStreams | null = null;
  private clients = new Set<WebSocket>();
  private rxBuffer = Buffer.alloc(0);
  private lastStats = {fps: 60, ramMb: 312, cpuPct: 1.2};

  public ensureCompiled(): void {
    if (!fs.existsSync(BINARY_PATH)) {
      console.log('Compiling Nature OS C/C++ previewer binary...');
      execSync('make -C previewer -j4', {cwd: process.cwd(), stdio: 'inherit'});
    }
  }

  public start(): void {
    if (this.proc) return;
    this.ensureCompiled();

    this.proc = spawn(BINARY_PATH, [ASSETS_DIR], {
      cwd: PREVIEWER_DIR,
      stdio: ['pipe', 'pipe', 'pipe'],
    });

    this.proc.stdout.on('data', (chunk: Buffer) => {
      this.handleStdoutChunk(chunk);
    });

    this.proc.stderr.on('data', (errChunk: Buffer) => {
      const msg = errChunk.toString('utf-8').trim();
      if (msg) console.error('[nature-os]', msg);
    });

    this.proc.on('exit', (code) => {
      console.warn(`nature-previewer exited with code ${code}`);
      this.proc = null;
      if (this.clients.size > 0) {
        setTimeout(() => this.start(), 600);
      }
    });
  }

  private handleStdoutChunk(chunk: Buffer): void {
    this.rxBuffer = Buffer.concat([this.rxBuffer, chunk]);

    while (this.rxBuffer.length >= 16) {
      // Check for NAT3 (v3 dynamic size + active text-input rects + single-pass zlib)
      if (
        this.rxBuffer[0] === 0x4e &&
        this.rxBuffer[1] === 0x41 &&
        this.rxBuffer[2] === 0x54 &&
        this.rxBuffer[3] === 0x33
      ) {
        if (this.rxBuffer.length < 36) return;
        const inputCount = this.rxBuffer.readUInt8(15);
        const zlen = this.rxBuffer.readUInt32LE(20);
        const totalLen = 36 + inputCount * 8 + zlen;
        if (this.rxBuffer.length < totalLen) return;

        const packet = Buffer.from(this.rxBuffer.subarray(0, totalLen));
        this.rxBuffer = this.rxBuffer.subarray(totalLen);

        this.lastStats = {
          fps: packet.readUInt16LE(8) / 10,
          ramMb: packet.readUInt16LE(10),
          cpuPct: packet.readUInt16LE(12) / 10,
          totalRamMb: packet.readUInt16LE(24),
          osRssMb: packet.readUInt16LE(26),
          frameMs: packet.readUInt16LE(28) / 10,
          cores: [
            packet.readUInt8(30),
            packet.readUInt8(31),
            packet.readUInt8(32),
            packet.readUInt8(33),
          ],
        } as typeof this.lastStats;

        for (const client of this.clients) {
          if (client.readyState === WebSocket.OPEN) {
            client.send(packet, {binary: true});
          }
        }
        continue;
      }

      // Check for NAT2 (v2 single-payload high-speed framing)
      if (
        this.rxBuffer[0] === 0x4e &&
        this.rxBuffer[1] === 0x41 &&
        this.rxBuffer[2] === 0x54 &&
        this.rxBuffer[3] === 0x32
      ) {
        if (this.rxBuffer.length < 20) return;
        const zlen = this.rxBuffer.readUInt32LE(16);
        const totalLen = 20 + zlen;
        if (this.rxBuffer.length < totalLen) return;

        const packet = Buffer.from(this.rxBuffer.subarray(0, totalLen));
        this.rxBuffer = this.rxBuffer.subarray(totalLen);

        this.lastStats = {
          fps: packet.readUInt16LE(8) / 10,
          ramMb: packet.readUInt16LE(10),
          cpuPct: packet.readUInt16LE(12) / 10,
        };

        for (const client of this.clients) {
          if (client.readyState === WebSocket.OPEN) {
            client.send(packet, {binary: true});
          }
        }
        continue;
      }

      if (
        this.rxBuffer[0] !== 0x4e ||
        this.rxBuffer[1] !== 0x41 ||
        this.rxBuffer[2] !== 0x54 ||
        (this.rxBuffer[3] !== 0x52 && this.rxBuffer[3] !== 0x32 && this.rxBuffer[3] !== 0x33)
      ) {
        // Resync if unexpected bytes appear
        const idx = this.rxBuffer.indexOf('NAT');
        if (idx === -1) {
          this.rxBuffer = Buffer.alloc(0);
          return;
        }
        this.rxBuffer = this.rxBuffer.subarray(idx);
        if (this.rxBuffer.length < 16) return;
      }

      const tileCount = this.rxBuffer.readUInt16LE(14);
      let offset = 16;
      let complete = true;

      for (let i = 0; i < tileCount; i++) {
        if (this.rxBuffer.length < offset + 12) {
          complete = false;
          break;
        }
        const zlen = this.rxBuffer.readUInt32LE(offset + 8);
        if (this.rxBuffer.length < offset + 12 + zlen) {
          complete = false;
          break;
        }
        offset += 12 + zlen;
      }

      if (!complete) return;

      const packet = Buffer.from(this.rxBuffer.subarray(0, offset));
      this.rxBuffer = this.rxBuffer.subarray(offset);

      this.lastStats = {
        fps: packet.readUInt16LE(8) / 10,
        ramMb: packet.readUInt16LE(10),
        cpuPct: packet.readUInt16LE(12) / 10,
      };

      for (const client of this.clients) {
        if (client.readyState === WebSocket.OPEN) {
          client.send(packet, {binary: true});
        }
      }
    }
  }

  public addClient(ws: WebSocket): void {
    this.clients.add(ws);
    if (!this.proc) {
      this.start();
    }
    this.sendCommand('F');

    ws.on('message', (data, isBinary) => {
      if (isBinary) return;
      const text = data.toString('utf-8').trim();
      if (text) {
        this.sendCommand(text);
      }
    });

    ws.on('close', () => {
      this.clients.delete(ws);
    });
  }

  public sendCommand(cmd: string): void {
    if (!this.proc || !this.proc.stdin.writable) return;
    this.proc.stdin.write(`${cmd}\n`);
  }

  public getStats() {
    return this.lastStats;
  }

  public shutdown(): void {
    if (this.proc) {
      try {
        this.proc.stdin.write('Q\n');
      } catch {
        // ignore
      }
      this.proc.kill('SIGTERM');
      this.proc = null;
    }
  }
}
