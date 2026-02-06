import socket,struct

s = socket.socket()
s.connect(("127.0.0.1",7000))

MSG_REGISTER = 1
peer_port = 5001

payload = struct.pack("!I",peer_port)
length = 1 +len(payload)

s.sendall(struct.pack("!I",length))
s.sendall(bytes([MSG_REGISTER])+payload)

# read ACK
ack_len = struct.unpack("!I",s.recv(4))[0]
ack = s.recv(ack_len)

print("ACK received")

s.close()