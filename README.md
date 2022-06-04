An ns-3 simulation of a home network consisting of two personal computers (PC) connected to a
wired Ethernet network, and 5 other devices connected over WiFi.
The devices at home are connected through a router, which in turn is connected over a wired network to an ISP server. 
An application is run on each of the node to generate saturated traffic.

The following situations were simulated-

1. Only one PC is running and no other device is running
2. Both PC’s are running, and no other devices are running
3. Both PC’s are running, and 3 other devices are running
4. All devices are running

The correponding PCAP traces were collected and analyzed for the change in throughput at the nodes.
