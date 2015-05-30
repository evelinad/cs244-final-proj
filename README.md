# CS 244 Final Project: Reproducing "TCP Congestion Control with a Misbehaving Receiver"

To run this code, simply:

1. Create a c3.2xlarge instance on EC2 based on the "CS244-Spr15-Mininet" AMI
2. Clone this repo and cd into it
3. sudo ./run.sh

Outputs should be in the directory specified in run.sh


Original project proposal below.  The top of this README will preference technical minutiae.

## Project Implementation Strategy

Instead of building a custom kernel and/or loadable kernel modules, we will base our TCP Daytona implementation off of the [LwIP](http://savannah.nongnu.org/projects/lwip/) project, which is a lightweight, user-space TCP/IP stack.

### Execution Notes

**echop** now works like an iperf client.  Simply run "sudo ./echop" in one terminal and "iperf -c 192.168.0.2" in another.  I was showing 296 MBits/sec of throuput before ACK division.

### Compilation

For starters, I've copied the contents of $(CONTRIBDIR)/ports/unix/proj/minimal into the root directory and modified the Makefile accordingly.  Running 'sudo ./echop' starts a TCP echo server on port 7 (you can interact with it by running 'nc [IP address from echop output] 7').  It's (relatively) simple to see how echop is formulated in the Makefile - all LwIP and LwIP sources are compiled to their respective .a libraries and then linked against.  I hope our project code will be able to leverage this boilerplate nicely.

## Project Proposal
Original paper [linked here](http://www.google.com/url?q=http%3A%2F%2Fcseweb.ucsd.edu%2F%257Esavage%2Fpapers%2FCCR99.pdf&sa=D&sntz=1&usg=AFQjCNFIfy1P5RgDYmguNgWUhmgd_3o0Bw).

In “TCP Congestion Control with a Misbehaving Receiver,” Savage, Wetherall, and Anderson demonstrate a set of attacks on TCP by which a receiver (the attacker) might induce a sender (the victim) to transmit data at a much higher rate than congestion control would allow normally.  In addition to demonstrating these attacks, Savage et al. also propose incremental changes to TCP that foreclose the possibility of throughput-boosting attacks.  The attacks are as follows:

### ACK Division
The receiver exploits discrepancies between segment- and byte-level acknowledgement by converting a single ACK for N bytes into M ACKs for N/M bytes each.  Since the congestion window (cwnd) grows by the sender’s maximum segment size (SMSS) on every acknowledgement, slow start can be made to grow the congestion window much quicker than usual through ACK Division.

### DupACK Spoofing
The receiver sends multiple “empty” ACK messages (ACKs that acknowledge zero of the bytes sent in the previous segment) to put the sender into the fast start state and to grow its congestion window arbitrarily large.

### Optimistic ACKing
The receiver sends ACK messages optimistically, before actually receiving segments with the corresponding sequence numbers.  Since TCP is self-clocking, optimistically ACKing will trick the sender into estimating an RTT that is lower than reality.

### Replicating the Experiment
For our project, we would like to demonstrate the three attacks by replicating figures 4, 5, and 6 of the paper.  To do so, we will need to (1) implement TCP with the three attacks, (2) identify a server (local or remote) to issue an HTTP request to, and (3) log the sequence numbers received and acknowledged over time as our hacked receiver communicates with the server.  Tasks 2 and 3 should be relatively simple (we might request www.stanford.edu and timestamp packets using Wireshark), but task 1 might require substantial effort.  The paper’s authors were able to build their attacks directly into Linux 2.2.10 with very few lines of code (<25), and we would seek to do the same.  Alternately, we might build our own TCP implementation on top of basic sockets (as we did in CS 144).

