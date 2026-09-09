import socket, struct

s = socket.socket()
s.connect(("127.0.0.1", 7000))

MSG_GET_PEERS = 3

# send GET_PEERS
s.sendall(struct.pack("!I", 1))
s.sendall(bytes([MSG_GET_PEERS]))

# receive response
length = struct.unpack("!I", s.recv(4))[0]
body = s.recv(length)

msg_type = body[0]
peer_count = struct.unpack("!I", body[1:5])[0]

print("Peers:", peer_count)

offset = 5
for _ in range(peer_count):
    ip = socket.inet_ntoa(body[offset:offset+4])
    port = struct.unpack("!H", body[offset+4:offset+6])[0]
    offset += 6
    print(ip, port)

s.close()
