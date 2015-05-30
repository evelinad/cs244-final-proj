#!/usr/bin/python

import matplotlib.pyplot as plt
from pylab import figure
from argparse import ArgumentParser
import re
import pdb

BASELINE = 'dump.baseline.out'
ACKDIV = 'dump.ackdiv.out'
ACKDUP = 'dump.ackdup.out'

parser = ArgumentParser(description="Plotter")
parser.add_argument('--dir',
                    help="Directory containing tcpdump output",
                    required=True)
parser.add_argument('--baseline',
                    help="Name of baseline tcpdump",
                    default="dump.baseline.out")
parser.add_argument('--ackdiv',
                    help="Name of ack division tcpdump",
                    default="dump.ackdiv.out")
parser.add_argument('--ackdup',
                    help="Name of ack duplication tcpdump",
                    default="dump.ackdup.out")
parser.add_argument('--ackdivout',
                    help="Name of plot output comparing ACK division w/ the baseline",
                    default="AckDiv.png")
parser.add_argument('--ackdupout',
                    help="Name of plot output comparing ACK duplication w/ the baseline",
                    default="AckDup.png")
args = parser.parse_args()

def plot(sq_base, sq, filename):
    plt.clf()
    plt.scatter([x[0] for x in sq_base], [x[1] for x in sq_base], marker='x', label='Data Segments (normal)')
    plt.scatter([x[0] for x in sq], [x[1] for x in sq], marker='s', label='Data Segments')
    # plt.show()
    plt.savefig(filename)

def parse_data(filename):
    seqnos = []

    # Get the sequence numbers
    with open(filename) as f:
        for line in f.readlines():
            # Packet transmission
            if re.search('Flags \[P.\]', line) is not None:
                ts = float(line.split(' ')[0])
                seqno = float(re.search('seq \d+:(\d+)', line).group(1))
                seqnos.append((ts, seqno))

    # Compute relative timestamps
    if len(seqnos) > 0:
        first_ts = seqnos[0][0]
        for i in range(len(seqnos)):
            seqnos[i] = (seqnos[i][0] - first_ts, seqnos[i][1])

    return seqnos

def main():
    sq_base = parse_data("%s/%s" % (args.dir, args.baseline))
    sq_div  = parse_data("%s/%s" % (args.dir, args.ackdiv))
    sq_dup  = parse_data("%s/%s" % (args.dir, args.ackdup))

    plot(sq_base, sq_div, args.ackdivout)
    plot(sq_base, sq_dup, args.ackdupout)

if __name__ == '__main__':
    main()


