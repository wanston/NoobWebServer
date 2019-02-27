#!/usr/bin/python

from socket import *
import sys
import random
import os
import time

if len(sys.argv) < 3:
    sys.stderr.write('Usage: %s <ip> <port> <connection>\n' % (sys.argv[0]))
    sys.exit(1)

serverHost = gethostbyname(sys.argv[1])
serverPort = int(sys.argv[2])
numConnections = int(sys.argv[3])


socketList = []

RECV_TOTAL_TIMEOUT = 0.1
RECV_EACH_TIMEOUT = 0.01

for i in range(numConnections):
    s = socket(AF_INET, SOCK_STREAM)
    s.connect((serverHost, serverPort))
    socketList.append(s)


r = 'GET / HTTP/1.1\r\nUser-Agent: Chrome\r\n\r\n'


for s in socketList:
    s.send(r.encode())

# for s in socketList:
#     data = s.recv(100)
#     print(data)
#
# for s in socketList:
#     s.close()


input()


print("Success!")
