#!/usr/bin/env python

# This TCP server listens for connections on a specified port, converts
# incoming messages by swapping the case of the characters and sends it back
# to the originator.

import socket
import sys

if len(sys.argv) != 2:
	print "USAGE: eg_tcpechoserver.py <port>"
else:
	sock = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
	sock.bind(('',int(sys.argv[1])))
	sock.listen(5)
	print "Listening on port", int(sys.argv[1])
	while 1:    # Run until cancelled
		newsock, client_addr = sock.accept()
		print "Client connected:", client_addr
		data = newsock.recv(512)
		while data:
			response = data.swapcase()
			print "Sending data:", response
			newsock.sendall( response )
			data = newsock.recv(512)
		newsock.close()
