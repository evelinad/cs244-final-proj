# CS 244 Final Project: Reproducing "TCP Congestion Control with a Misbehaving Receiver"

To run this code, simply:

1. Create a c3.2xlarge instance on EC2 based on the "CS244-Spr15-Mininet" AMI
2. Clone this repo and cd into it
3. sudo ./run.sh

tcpdump outputs should be in the directory specified in run.sh.  To generate plots, mount the filesystem with sshfs and run 'python tcpplot.py --dir output' on your local machine.
