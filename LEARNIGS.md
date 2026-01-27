# LEARNINGS.md

This document captures **concepts learned**, **confusions faced**, **mistakes corrected**, and **design decisions** made while building the tracker-based P2P file distribution system. It is intended for **revision before interviews** and to clearly articulate learning progress.

---

## Day 1 — Architecture & Core Concepts

### Key Concepts Learned

* **Peer-to-Peer (P2P) Architecture**: Files are shared directly between peers instead of through a central server.
* **Tracker**: A metadata service that knows which peers have which files/chunks. It never stores or transfers file data.
* **Peer**: A process that uploads and downloads file chunks and reassembles files locally.
* **Client CLI**: A thin control interface used to issue commands like upload and download.
* **Control Plane vs Data Plane**:

  * Tracker and Client CLI operate in the **control plane** (coordination, metadata).
  * Peers operate in the **data plane** (actual file chunk transfer).

### Architecture Understanding

* One machine can simulate multiple peers by running **multiple processes on different ports**.
* A peer is identified by **IP + port**, not by machine.
* Files are split into **fixed-size chunks** to enable parallel downloads and fault tolerance.

### Important Clarifications

* Seeder is **not a server**; it is just a peer that currently has the full file.
* Seeder role is **temporary and dynamic**, not permanent.
* File reassembly happens **inside the peer**, never in the tracker.

### Confusions Faced & Resolved

* Initially confused client vs peer → learned that client only issues commands, peer does the real work.
* Initially thought peers might collaboratively assemble a file → learned that **each downloading peer assembles its own file locally**.

### Interview-Ready Takeaway

> “I designed a tracker-based P2P system where the tracker manages metadata in the control plane and peers exchange file chunks directly in the data plane.”

---

## Day 2 — Tracker APIs & Failure Handling

### Key Concepts Learned

* **Heartbeat Mechanism**: Peers periodically notify the tracker they are alive.
* **Peer Churn**: Peers can join and leave at any time; the system must adapt.
* **Chunk-Level Availability**: Tracker tracks which peer has which chunk, not just which file.

### Why Heartbeats Are Needed

* Peers can crash or disconnect without warning.
* Heartbeats allow the tracker to remove **dead peers** and avoid giving invalid peer info to downloaders.

### Tracker State (Metadata Only)

* Peer info: IP, port, last-seen timestamp
* File info: fileId, total chunks, chunk hashes
* Availability mapping: chunk index → list of peerIds

### Tracker API Responsibilities

* Register peers
* Accept chunk announcements
* Respond with peer lists for a requested file
* Remove peers that stop sending heartbeats

### Important Design Decisions

* Tracker returns **multiple peers** to allow retries and parallel downloads.
* Tracker returns **chunk-level data** to:

  * Reduce unnecessary requests
  * Enable parallel downloads
  * Support resume functionality

### Mistakes & Corrections

* Initially thought SHA-256 was used for secrecy → learned it is for **data integrity verification**.
* Initially underestimated importance of chunk-level metadata → learned it is critical for efficiency and resilience.

### Failure Handling Learned

* If a peer sends a corrupted chunk → re-download from another peer.
* If a peer crashes mid-transfer → retry from another peer.
* If the tracker crashes after discovery → downloads continue because data plane is independent.

### Interview-Ready Takeaway

> “The tracker acts purely as a control-plane service. Once peers discover each other, file transfer continues even if the tracker goes down, making the system resilient.”

---

## Day 3 — Peer ↔ Peer TCP Protocol (Design)

### Key Concepts Learned

* **TCP is a byte stream**, not message-based; explicit framing is required.
* **Two-phase receive model**: first read a fixed-size header, then read a variable-size payload.
* **Partial reads are normal**; `recv()` may return fewer bytes than requested.

### Protocol Messages Designed

* `REQUEST_CHUNK`: Requests a specific chunk by index for a file.
* `CHUNK_DATA`: Sends chunk bytes along with metadata (fileId, chunkIndex, chunkSize).
* `CHUNK_ACK`: Confirms successful receipt of a chunk.
* `ERROR`: Signals issues like missing chunks or overload.

### Why Length-Prefixed Framing

* Header contains `messageLength`, which tells the receiver exactly how many bytes to read for one complete message.
* Prevents partial reads and message coalescing issues inherent to TCP streams.

### Read Loop Semantics (Critical)

* Always loop on `recv()` until the required number of bytes is collected.
* `recv() == 0` indicates the peer has gracefully disconnected.
* Processing data before a full message arrives leads to corruption and hard-to-debug bugs.

### Backpressure & Flow Control

* **ACK-based flow control** ensures only one chunk is in flight per connection.
* Prevents receiver overload, unbounded buffering, and crashes.

### Common Pitfalls Identified

* Assuming one `send()` equals one `recv()`.
* Ignoring the return value of `recv()`.
* Omitting `chunkIndex`, which breaks reassembly when chunks arrive out of order.

---

## Day 4 — Peer Implementation (Foundations)

### Key Concepts Learned

* A peer is a **long-running process** that simultaneously acts as a server (upload) and a client (download).
* The peer must handle **incoming and outgoing connections concurrently**.

### Threading Model

* **Listener thread** runs the `accept()` loop to continuously accept new incoming connections.
* **Handler threads** process each accepted connection independently.
* Separate threads are used for outgoing connections to download chunks from other peers.

### Why Listener Must Be Separate

* Blocking `accept()` would prevent the peer from performing other tasks.
* Separate thread avoids bottlenecks and allows concurrent uploads.

### File & Chunk Safety

* Multiple threads writing to the same file can cause:

  * Data corruption
  * Overwritten chunks
  * Inconsistent file state
* A dedicated **ChunkManager** coordinates disk I/O to ensure thread safety.

### Minimum Working Peer (MWP)

* Start listening on a port.
* Accept multiple connections concurrently.
* Send and receive basic protocol messages safely.

---

## Open Questions / To Be Explored

* Safe synchronization strategies for ChunkManager
* Thread pool vs one-thread-per-connection trade-offs
* Graceful shutdown handling
