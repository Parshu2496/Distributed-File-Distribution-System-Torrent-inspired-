import socket, struct, time

s = socket.socket()
s.connect(("127.0.0.1", 7000))

MSG_HEARTBEAT = 2

while True:
    s.sendall(struct.pack("!I", 1))
    s.sendall(bytes([MSG_HEARTBEAT]))
    time.sleep(5)
