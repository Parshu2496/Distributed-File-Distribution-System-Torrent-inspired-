import socket
import struct

SERVER_IP = "127.0.0.1"
SERVER_PORT = 5001

MESSAGE_TYPE = 1
PAYLOAD = b"Hello"

def send_message():
    s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    s.connect((SERVER_IP,SERVER_PORT))

    message_length = 1+len(PAYLOAD)
    s.sendall(struct.pack("!I",1))#MessageLength = 1
    s.sendall(bytes([1]))#MessageType = 1

    s.sendall(struct.pack("!I",1))
    s.sendall(bytes([1]))


    s.close()
    print("Message sent successfully")

if __name__ == "__main__":
    send_message()