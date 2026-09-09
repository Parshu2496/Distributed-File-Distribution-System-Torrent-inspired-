# Distributed File Distribution System (Torrent-inspired)

This project is a **learning-focused, systems-level implementation** of a torrent-inspired
distributed file distribution system, built from scratch using **C++ and low-level TCP sockets**.

The goal of this project is to deeply understand **networking, concurrency, and protocol design**
rather than building a production BitTorrent client.

---

## 🏗️ Architecture

The system follows a classic **tracker-mediated P2P** architecture:

```
File Distribution System/
├── Peer/          ← Peer node implementation
├── Tracker/       ← Central tracker server
└── README.md
```

- **Tracker** acts as a central registry for peer discovery
- **Peers** register with the tracker, send heartbeats, query for other peers, and connect directly to each other

---

## 📦 Components

### `Tracker/` — Central Discovery Server

| File | Role |
|---|---|
| `tracker.cpp` | Main tracker logic — listens on **port 7000** |
| `ConnectionHandler.cpp` | `recvAll()` — reliable byte-stream reader |
| `Protocol.h` | Shared message type enum |

**Tracker capabilities:**
- **`MSG_REGISTER`** — stores a peer's IP:port in an `unordered_map`
- **`MSG_HEARTBEAT`** — updates `lastSeen` timestamp for a peer
- **`MSG_GET_PEERS`** — responds with a serialized binary peer list (`MSG_PEER_LIST`)
- **Cleanup thread** — runs every 10s, evicts peers not seen in >30s

### `Peer/` — Peer Node

| File | Role |
|---|---|
| `main.cpp` | Entry point — takes `<port> [files_dir]` as CLI args |
| `Peer.cpp` | Listener, connection handler, file transfer logic |
| `Peer.h` | `Peer` class declaration |
| `ConnectionHandler.cpp` | `recvAll()` transport utility |
| `FileManager.cpp` | Chunk read/write — splits files into 512 KB blocks |
| `FileManager.h` | `FileManager` class declaration |
| `Protocol.h` | Message type definitions + `CHUNK_SIZE` constant |

**Python test scripts:**
- `register_peer.py` — registers a peer with the tracker
- `get_peers.py` — queries tracker for peer list
- `send_heartbeat.py` — sends heartbeat to tracker
- `send_message.py` — sends test messages to a peer

---

## 🔌 Custom Binary Protocol

Message frame format:

```
[4-byte length (network byte order)] [1-byte type] [payload...]
```

| Code | Name | Direction |
|---|---|---|
| `1` | `MSG_REGISTER` | Peer → Tracker |
| `2` | `MSG_HEARTBEAT` | Peer → Tracker |
| `3` | `MSG_GET_PEERS` | Peer → Tracker |
| `4` | `MSG_PEER_LIST` | Tracker → Peer |
| `5` | `MSG_ACK` | Any → Any |
| `6` | `MSG_PING` | Peer → Peer |
| `7` | `MSG_REQUEST_FILE_INFO` | Downloader → Seeder |
| `8` | `MSG_FILE_INFO` | Seeder → Downloader |
| `9` | `MSG_REQUEST_CHUNK` | Downloader → Seeder |
| `10` | `MSG_CHUNK_DATA` | Seeder → Downloader |

Key design decisions:
- **Length-prefix framing** solves TCP's stream-oriented boundary problem
- **`recvAll()`** loops on `recv()` until the exact byte count is satisfied, handling partial reads transparently
- **Network byte order** (`htonl`/`ntohl`) is used consistently for all multi-byte fields
- **`CHUNK_SIZE = 512 KB`** — files are split into fixed-size blocks; the last chunk is naturally smaller

---

## 🚀 Current Features (Implemented)

| Feature | Status |
|---|---|
| Multithreaded TCP peer server | ✅ Done |
| Custom binary protocol + framing | ✅ Done |
| `recvAll()` partial-read handling | ✅ Done |
| Tracker with peer registry + heartbeats | ✅ Done |
| Peer-to-peer PING/ACK | ✅ Done |
| `FileManager` — chunk-based file read/write | ✅ Done |
| Sequential chunk-based file transfer | ✅ Done |
| Parallel downloads from multiple peers | ⬜ Planned |
| Integrity verification (hashing) | ⬜ Planned |
| Peer failure handling & retries | ⬜ Planned |

---

## 🛠️ Tech Stack

- **Language:** C++
- **Networking:** POSIX TCP sockets
- **Concurrency:** `std::thread` (thread-per-connection model)
- **Protocol:** Custom binary protocol (header + body framing)
- **Testing:** Python-based test scripts

---

## 🚦 Getting Started

> Requires Linux or WSL (uses POSIX sockets). On Windows, run inside **WSL**.

### 1 — Build

```bash
# Tracker
cd Tracker
g++ -std=c++17 -pthread tracker.cpp ConnectionHandler.cpp -o tracker

# Peer
cd Peer
g++ -std=c++17 -pthread main.cpp Peer.cpp ConnectionHandler.cpp FileManager.cpp -o peer
```

### 2 — Start the Tracker

```bash
cd Tracker
./tracker
# [Tracker] Listening on port 7000
```

### 3 — Transfer a file between two peers

```bash
cd Peer

# Create the seeder's file directory and add a test file
mkdir -p files_5002
echo "Hello from peer 5002!" > files_5002/test.txt

# Terminal 1 — Seeder (serves files from files_5002/)
./peer 5002 files_5002

# Terminal 2 — Downloader (saves received_test.txt to current dir)
./peer 5001 .

# Verify the transfer was byte-perfect
diff files_5002/test.txt received_test.txt
```

### 4 — Test tracker interactions (Python)

```bash
cd Peer
python3 register_peer.py   # register a peer
python3 get_peers.py       # list all known peers
python3 send_heartbeat.py  # keep a peer alive
```

---

## ⚠️ Disclaimer

This project is intended for **educational purposes only**.
It does **not** support illegal content or public torrent networks.

---

## 📚 Why this project?

Most high-level applications hide networking details behind frameworks.
This project focuses on **understanding what actually happens underneath**:
TCP behavior, message framing, concurrency, and system design trade-offs.
