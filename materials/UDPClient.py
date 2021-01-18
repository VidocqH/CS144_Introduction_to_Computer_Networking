from socket import * # Import socket lib

serverName = gethostname() # set server
serverPort = 12000 # set server port
clientSocket = socket(AF_INET, SOCK_DGRAM) # create client socket

message = input('Input lowercase sentence:') # Get input
clientSocket.sendto(message.encode(), (serverName, serverPort)) # Send message

modifiedMessage, serverAddress = clientSocket.recvfrom(2048) # Receive message from server
print(modifiedMessage.decode()) # Print response

clientSocket.close() # Close client socket
