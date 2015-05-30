#!/usr/bin/python

from argparse import ArgumentParser
from time import time

import socket

parser = ArgumentParser(description="TCP Source")
parser.add_argument('--ip',
                    help="IP address of receiver",
                    required=True)
parser.add_argument('--port',
                    help="Port of receiver",
                    type=int,
                    required=True)
parser.add_argument('--time',
                    help="How long to send in ms",
                    type=int,
                    default=5000)
# parser.add_argument('--recv',
#                     help="Should recv?",
#                     required=True)
args = parser.parse_args()

message = "A" * 8

def receive():
  r = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  r.bind(("127.0.0.1", 5005))
  r.listen(1)
  conn, addr = r.accept()
  print 'Connection address: ', addr
  while 1:
    data = conn.recv(128)
    if not data: break
    print data
  conn.close()

if __name__ == '__main__':
  # if args.recv == "recv":
  #   receive()
  # else:
  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  s.connect((args.ip, args.port))

  # Send data until timeout
  start_time = time()
  while (time() - start_time < args.time / 1000):
    # print "Time elapsed: ", (time() - start_time)
    s.send(message)

  s.close()
  print "TCPSource Completed."
