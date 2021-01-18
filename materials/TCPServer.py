from socket import * # Import socket lib

serverPort = 12000 # Set server port
serverSocket = socket(AF_INET, SOCK_STREAM) # Create server socket
serverSocket.bind(('', serverPort)) # Bind socket and port
serverSocket.lisetn(1) # Socket listen
print('The server is ready to receive.')

# Server needs to run continually
while True:
	connectionSocket, address = serverSocket.accept() # Create connection socket after server socket receive request

	sentence = connectionSocket.recv(1024),decode() # Decode the message
	capitalizedSentence = sentece.upper() # Upper the message

	connectionSocket.send(capitalizedSentence.encode()) # Return the response to client

	connectionSocket.close() # Close connection socket
