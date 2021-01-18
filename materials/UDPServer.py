from socket import * # Socket Lib

serverPort = 12000 # Set server port
serverSocket = socket(AF_INET, SOCK_DGRAM) # Create socket with SOCK_DGRAM
serverSocket.bind(('', serverPort)) # Bind socket and port
print('The server is ready to receive.')

# Server needs to continually run
while True:
	message, clientAddress = serverSocket.recvfrom(2048) # Read from socket
	modifiedMessage = message.decode().upper() # Upper the message

	serverSocket.sendto(modifiedMessage.encode(), clientAddress) # Send back to client
