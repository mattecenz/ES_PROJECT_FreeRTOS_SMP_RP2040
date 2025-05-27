import socket

SERVER_IP = '127.0.0.1'  # Change to your server's IP
SERVER_PORT = 4242      # Change to your server's port


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((SERVER_IP, SERVER_PORT))
    print(f"Connected to {SERVER_IP}:{SERVER_PORT}")
    # Send a string
    message = "Hello, server!"
    s.sendall(message.encode('utf-8'))
    # Receive a string
    data = s.recv(1024)
    print('Received from server:', data.decode('utf-8'))
