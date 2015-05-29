#!/usr/bin/python

"CS244 Spring 2015 Final Project: TCP Daytona"

from mininet.topo import Topo
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.net import Mininet
from mininet.log import lg, info
from mininet.util import dumpNodeConnections
from mininet.cli import CLI

from subprocess import Poopen, PIPE
from time import sleep, time
from multiprocessing import Process
from argparse import ArgumentParser

import sys
import os
import math

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

# Invoke the following to see available algorithms:
# sysctl -a | grep cong
# TODO: Migrate setting this out to the wrapper .sh script
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

    # Add a switch
    switch = self.addSwitch('s0')

    # Add links
    self.addLink(host1, switch, bw=args.bw, delay='%sms' % (args.delay))
    self.addLink(switch, host2, bw=args.bw, delay='%sms' % (args.delay))
    return

def run_experiment():
  pass

if __name__ == "__main__":
  run_experiment()