from socket import * # Import socket lib

serverName = gethostname() # Server use localhost
serverPort = 12000 # Set server port
clientSocket = socket(AF_INET, SOCK_STREAM) # Create client socket
clientSocket.connect((serverName, serverPort)) # TCP connect

sentence = input('Input lowercase sentence:')
clientSocket.send(sentence.encode()) # Send payload to TCP server

modifiedSentence = clientSocket.recv(1024) # Receive the response
print('From server:', modifiedSentence.decode())

clientSocket.close() # Close client socket
