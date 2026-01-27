# Distributed File Distribution System (Torrent-inspired)

This project is a **learning-focused, systems-level implementation** of a torrent-inspired
distributed file distribution system, built from scratch using **C++ and low-level TCP sockets**.

The goal of this project is to deeply understand **networking, concurrency, and protocol design**
rather than building a production BitTorrent client.

---

## 🚀 Current Features (Implemented)

- Multithreaded TCP peer server (thread-per-connection model)
- Custom binary protocol with **length-prefixed message framing**
- Safe and reliable message parsing over TCP byte streams
- Handling of partial reads using a reusable transport-layer utility
- Clear separation between **transport-layer logic** and **application logic**
- Test client for end-to-end protocol validation

---

## 🛠️ Tech Stack

- **Language:** C++
- **Networking:** POSIX TCP sockets
- **Concurrency:** std::thread
- **Protocol:** Custom binary protocol (header + body framing)
- **Testing:** Python-based test client

---

## 📌 Planned Features

- Tracker service for peer discovery
- Chunk-based file transfer
- Parallel downloads from multiple peers
- Integrity verification using hashing
- Peer failure handling and retries

---

## ⚠️ Disclaimer

This project is intended for **educational purposes only**.
It does **not** support illegal content or public torrent networks.

---

## 📚 Why this project?

Most high-level applications hide networking details behind frameworks.
This project focuses on **understanding what actually happens underneath**:
TCP behavior, message framing, concurrency, and system design trade-offs.
