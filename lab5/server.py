import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = ('10.10.10.1', 5006)
sock.bind(server_address)
sock.listen(1)
last_ack = 0
while True:
    connection, client_address = sock.accept()
    try:
        last_ack = 0
        while True:
            for i in range(last_ack,last_ack+3):
                connection.sendall("MSG:" + str(i))
            ack = connection.recv(16)
            if ack:
                last_ack = int(ack.split(":")[1])
            else:
                break
    finally:
        connection.close()
