#!/bin/bash

# 1000 Mb/s bandwidth
bw=1000
# 100 ms RTT
delay=50
dir=output

# Run the baseline
make clean
make echop
python tcpdaytona.py --delay $delay --dir $dir --bw $bw --dumpfile dump.baseline.out

# Run with ACK division
make clean
make echop EXTRA_OPTS=-DTCP_ACK_DIV
python tcpdaytona.py --delay $delay --dir $dir --bw $bw --dumpfile dump.ackdiv.out

# Run with DupACK spoofing
make clean
make echop EXTRA_OPTS=-DTCP_ACK_DUP
python tcpdaytona.py --delay $delay --dir $dir --bw $bw --dumpfile dump.ackdup.out

# Plot the figures
# python tcpplot.py --dir $dir