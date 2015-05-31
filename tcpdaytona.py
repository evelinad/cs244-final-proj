#!/usr/bin/python

"CS244 Spring 2015 Final Project: TCP Daytona"

from mininet.topo import Topo
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.net import Mininet
from mininet.log import lg, info
from mininet.util import dumpNodeConnections
from mininet.cli import CLI

from subprocess import Popen, PIPE
from time import sleep, time
from multiprocessing import Process
from argparse import ArgumentParser

import sys
import os
import pdb

parser = ArgumentParser(description="TCP Daytona tests")
parser.add_argument('--delay',
                    type=float,
                    help="Link propagation delay (ms)",
                    required=True)

parser.add_argument('--dir', '-d',
                    help="Directory to store outputs",
                    required=True)

parser.add_argument('--bw',
                    type=float,
                    help="Max buffer size of network interfaces in packets",
                    required=True)

parser.add_argument('--dumpfile',
                    help="File to store tcpdump output",
                    required=True)

# Invoke the following to see available algorithms:
# sysctl -a | grep cong
parser.add_argument('--cong',
                    help="Congestion control algorithm to use on unmodified TCP stack",
                    default="reno")

args = parser.parse_args()

class TCPDaytonaTopo(Topo):
  "Simple topology for TCP Daytona experiment"

  def build(self):
    # Create two hosts
    host1 = self.addHost('h1')
    host2 = self.addHost('h2')

    # Add links
    self.addLink(host1, host2, bw=args.bw, delay='%sms' % (args.delay))
    return

def run_experiment():
    # Create the output directory if it doesn't exist yet
    if not os.path.exists(args.dir):
        os.makedirs(args.dir)

    # Set the congestion control algorithm for the kernel TCP stack
    os.system("sysctl -w net.ipv4.tcp_congestion_control=%s" % args.cong)

    # Make sure the kernel is configured to do packet forwarding
    os.system("sysctl -w net.ipv4.ip_forward=1")

    # Create the topology and start mininet
    topo = TCPDaytonaTopo()
    net = Mininet(topo=topo, host=CPULimitedHost, link=TCLink)
    net.start()
    dumpNodeConnections(net.hosts)
    net.pingAll()

    sender = net.getNodeByName('h2')
    receiver = net.getNodeByName('h1')

    # Reduce sender's MTU so we get more and smaller packets
    sender.cmd("ifconfig h2-eth0 mtu 128")
    sender.cmd("sysctl net.core.netdev_max_backlog=500000")

    # Start tcpdump on the sending node
    sender.cmd("tcpdump -tt 'tcp port 5001' &> %s/%s &" % (args.dir, args.dumpfile))

    sleep(1)

    # Start echop server and configure sender to reach echop via receiver
    receiver.cmd('./echop > %s/echop.out &' % args.dir)
    sender.cmd('route add default gw %s' % receiver.IP())

    # Send a file to the receiver
    print sender.cmd('python tcpsource.py --ip 192.168.0.2 --port 5001 --time 5000')

    net.stop()

if __name__ == "__main__":
  run_experiment()