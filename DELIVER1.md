# CS244 Final Project Deliverable 1

## Introduction
TCP provides a mechanism for congestion control that depends on cooperation from the sender and the receiver. The sender uses a sliding window to limit the number of unacked packets it can send at once, only incrementing the window upon receiving an acknowledgement. In TCP Congestion Control with a Misbehaving Receiver, Savage et al. demonstrate exactly what the title suggests, that a malicious receiver can induce the sender to send data at a much higher bandwidth than congestion control would ordinarily permit. The paper outlines three main attacks:

- The receiver exploits discrepancies between segment- and byte-level acknowledgement by converting a single ACK for N bytes into M ACKs for N/M bytes each. Since the congestion window (cwnd) grows by the sender’s maximum segment size (SMSS) on every acknowledgement, slow start can be made to grow the congestion window much quicker than usual through ACK Division.

- TCP uses triple duplicate ACKs as a heuristic for detecting network congestion, since duplicate ACKs often signal dropped packets.

- The receiver sends ACK messages optimistically, before actually receiving segments with the corresponding sequence numbers. Since TCP is self-clocking, optimistically ACKing will trick the sender into estimating an RTT that is lower than reality.

## Goals
As mentioned in the paper, the implementation itself is not too complex. Therefore, we think it's feasible to code up all three attacks and to try to replicate the corresponding graphs -- namely, Figures 4, 5, and 6.

## Progress so far
To change the TCP protocol without recompiling the kernel, we've installed lwip, a lightweight user-level IP library. Thus far we have a simple echo server running on lwip's API, which we plan to extend to a simple web server that serves larger files for the actual experiments. Additionally, we've identified the points in the codebase needed to modify the attacks, and have implemented and verified the ACK division attack that returns multiple ACKs per segment.
