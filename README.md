arp-send
---

`arp-send` is a low-level ARP sender that allows you the specify both ethernet source/destination MAC, ARP source/destination MAC, IP source/destination and ARP type.

`arp-send` can be used for anti-ARP spoofing and ARP spoofing. The current anti-ARP spoofing software and ARP-spoofer are just way too heavy and don't offer user much of control. `arp-send` is created to solve this.

### Install

1. Clone this.
2. install `libnet`.
3. `make`.
4. `sudo make install`.

### Usage

```
Usage: arp_send COMMAND [COMMAND...]
where COMMAND := { request IFN ETHER_MAC_SRC ARP_MAC_SRC IP_SRC ETHER_MAC_DST ARP_MAC_DST IP_DST INTV |
                   reply IFN ETHER_MAC_SRC ARP_MAC_SRC IP_SRC ETHER_MAC_DST ARP_MAC_DST IP_DST INTV |
                   source SOURCEFILE }
where IFN := Interface name.
      ETHER_MAC_SRC := Ethernet source MAC.
      ETHER_MAC_DST := Ethernet destination MAC.
      ARP_MAC_SRC := ARP source MAC.
      ARP_MAC_DST := ARP destination MAC.
      IP_SRC := ARP source IP.
      IP_DST := ARP destination IP.
      INTV := Time in milliseconds to wait between sends.
```


### Examples

For example:

```
$ arp_send eth0 \
    reply \              # ARPOP: Reply
    aa:bb:cc:dd:ee:ff \  # Ethernet source MAC.
    bb:cc:dd:ee:ff:aa \  # ARP source MAC.
    1.0.0.1 \            # ARP source IP.
    cc:dd:ee:ff:aa:bb \  # Ethernet destination MAC.
    dd:ee:ff:aa:bb:cc \  # ARP destination MAC.
    1.0.0.2 \            # ARP destination IP.
    1000                 # 1000 ms wait time.
```

Will send the following ARP reply on eth0 with the rate of 1p/s:

```
Ethernet: type ARP
 \        src aa:bb:cc:dd:ee:ff
  \       dst cc:dd:ee:ff:aa:bb
   \
    - ARP: type reply
           sender bb:cc:dd:ee:ff:aa/1.0.0.1
           target dd:ee:ff:aa:bb:cc/1.0.0.2
```

And you can have multiple commands like:

```
$ arp_send \
    reply aa:bb:cc:dd:ee:ff bb:cc:dd:ee:ff:aa 1.0.0.1 cc:dd:ee:ff:aa:bb dd:ee:ff:aa:bb:cc 1.0.0.2 1000 \
    reply bb:cc:dd:ee:ff:aa cc:dd:ee:ff:aa:bb 1.0.0.3 dd:ee:ff:aa:bb:cc ee:ff:aa:bb:cc:dd 1.0.0.4 1000
```

To load commands from a file, use "source FILENAME":

```
$ cat << FILE > arp.conf
reply aa:bb:cc:dd:ee:ff bb:cc:dd:ee:ff:aa 1.0.0.1 cc:dd:ee:ff:aa:bb dd:ee:ff:aa:bb:cc 1.0.0.2 1000
reply bb:cc:dd:ee:ff:aa cc:dd:ee:ff:aa:bb 1.0.0.3 dd:ee:ff:aa:bb:cc ee:ff:aa:bb:cc:dd 1.0.0.4 1000
FILE
$ arp_send source ./arp.conf
```

Some more real-life type examples will be like:

Assume the network `172.17.0.0/24`, where the router is at `172.17.0.1/01:01:01:01:01:01`, someone else is at `172.17.0.2/02:02:02:02:02:02`, and you are `172.17.0.3/03:03:03:03:03:03`.

```
$ arp-send reply eth0 03:03:03:03:03:03 03:03:03:03:03:03 172.17.0.1 ff:ff:ff:ff:ff:ff ff:ff:ff:ff:ff:ff 255.255.255.255 100
```
(Tell everyone on the network that `172.17.0.1` is at `03:03:03:03:03:03`)

```
$ arp-send \
    reply eth0 03:03:03:03:03:03 03:03:03:03:03:03 172.17.0.1 02:02:02:02:02:02 02:02:02:02:02:02 172.17.0.2 100 \
    request eth0 03:03:03:03:03:03 03:03:03:03:03:03 172.17.0.2 01:01:01:01:01:01 01:01:01:01:01:01 172.17.0.1 100
```
(Tell `172.17.0.2` that `172.17.0.1` is at `03:03:03:03:03:03`, ask router for its MAC address with `172.17.0.2/03:03:03:03:03:03`. (so the router think  `172.17.0.2` is at `03:03:03:03:03:03`))

```
$ arp-send request eth0 02:02:02:02:02:02 02:02:02:02:02:02 172.17.0.2 01:01:01:01:01:01 01:01:01:01:01:01 172.17.0.1 100
```
(Ask router for its MAC address with correct IP/MAC of `172.17.0.2/02:02:02:02:02:02` pair and potentially protect `172.17.0.2/02:02:02:02:02:02` from ARP attacks)
