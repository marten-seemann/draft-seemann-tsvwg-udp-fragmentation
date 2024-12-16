---

title: "Controlling IP Fragmentation on Common Platforms"
abbrev: "IP fragmentation on UDP Sockets"
category: info

docname: draft-seemann-tsvwg-udp-fragmentation-latest
submissiontype: IETF  # also: "independent", "editorial", "IAB", or "IRTF"
number:
date:
consensus: true
v: 3
area: "Web and Internet Transport"
workgroup: "Transport and Services Working Group"
keyword:
 - IP fragmentation
 - Don't Fragment
 - Path MTU Discovery
 - DPLPMTUD

author:
 -
    fullname: Marten Seemann
    email: martenseemann@gmail.com

normative:

informative:


--- abstract

When performing Path MTU Discovery (PMTUD) over UDP, applications must prevent
fragmentation of UDP datagrams both by the sender's kernel and during network
transit. This document provides guidance for implementers on configuring socket
options to prevent fragmentation of IPv4 and IPv6 packets across commonly used
platforms.

--- middle

# Introduction

{{!RFC0791}} defines the Don't Fragment (DF) bit in the IPv4 header. When set,
this bit prevents routers from fragmenting IP packets. If a router needs to
fragment a packet with the DF bit set, it will instead drop the packet and send
an ICMP "fragmentation needed" message back to the sender.

The DF bit has historically been most relevant to TCP ({{!RFC9293}}), where the
kernel handles Path MTU Discovery (PMTUD) internally. Applications using TCP
sockets do not need to interact with the DF bit directly.

In IPv6 ({{!RFC8200}}), fragmentation by intermediate nodes is not permitted.
All IPv6 packets effectively have the DF bit set, however, the endpoint's kernel
might still break up UDP datagrams that are too large to fit the MTU of the
interface before sending a packet into the network.

{{!RFC8899}} defines Datagram Packetization Layer Path MTU Discovery (DPLPMTUD),
a mechanism that allows protocols running over UDP to determine the maximum
packet size they can send. Setting the DF bit is crucial for DPLPMTUD, as it
ensures that packets larger than the Path MTU are dropped, allowing the endpoint
to detect the MTU limitation.

QUIC {{!RFC9000}} is one such protocol that runs over UDP and make use of
DPLPMTUD. As QUIC implementations typically run in user space, they need to
configure the DF bit on their UDP sockets to perform DPLPMTUD correctly.

This document provides guidance for implementers on how to set the DF bit on UDP
sockets across different platforms.


# Conventions and Definitions

{::boilerplate bcp14-tagged}


# Setting the DF bit

While routers don't fragment IPv6 packets in transit, the sender's kernel will
still fragment UDP datagrams that are too large to fit the MTU of the interface
before sending a packet into the network. Therefore, operating systems offer
socket options to control the fragmentation behavior for IPv6 packets.

For user-space implementations of DPLPMTUD, applications need to set the DF bit on
IPv4 sockets and prevent fragmentation on IPv6 sockets.

## Linux

PMTUD behavior is controlled via the IP_MTU_DISCOVER and IPV6_MTU_DISCOVER
socket option on level IPPROTO_IP and IPPROTO_IPV6. A value of IP_PMTUDISC_DO
and IPV6_PMTUDISC_DO turns on the DF bit, and enables the processing of ICMP
packets by the kernel. A value of IP_PMTUDISC_PROBE and IPV6_PMTUDISC_PROBE
turns on the DF bit, and disables the processing of ICMP packets by the kernel.

Given that IP_PMTUDISC_DO and IPV6_PMTUDISC_DO prevent sending datagrams larger
than the observed path MTU and are prone to ICMP NEEDFRAG attacks, one should
use IP_PMTUDISC_PROBE and IPV6_PMTUDISC_PROBE.

For dual-stack sockets, both IPv4 and IPv6 socket options can be set
independently.

In addition, for IPv6, to prevent local fragmentation when sending packets
larger than the interface MTU, set the socket option of level IPPROTO_IPV6 with
name IPV6_DONTFRAG.


## Apple

For IPv4, Apple platforms use the socket option of level IPPROTO_IP with name
IP_DONTFRAG with value 1. For IPv6, IPV6_DONTFRAG with value 1 is used for the
IPPROTO_IPV6 level.

However, dual-stack sockets are handled differently: To open a dual-stack socket,
an IPv6 socket needs to be opened and the IPV6_V6ONLY option needs to be set to
0. This enables the socket to send both IPv4 and IPv6 packets. IPv4 packets must
be sent using an IPv4-mapped IPv6 address.

When using a dual-stack socket, it is only necessary (and possible) to set the
IPV6_DONTFRAG socket option. This results in the DF bit being set when sending
IPv4 packets. It is not possible to control the fragmentation behavior of IPv4
and IPv6 separately.


## Windows

For IPv4, Windows uses the socket option of level IPPROTO_IP with name
IP_DONTFRAGMENT with value 1. For IPv6, IPV6_DONTFRAG with value 1 is used for
the IPPROTO_IPV6 level.

Similar to the Apple platforms, dual-stack sockets are IPv6 sockets with the
IPV6_V6ONLY option set to 0. IPv4 packets must be sent using an IPv4-mapped
IPv6 address. However, contractary to Apple platforms, the DF bit on IPv4
packets is controlled by the IP_DONTFRAGMENT socket option.


# Security Considerations

TODO Security


# IANA Considerations

This document has no IANA actions.


--- back

# Acknowledgments
{:numbered="false"}

TODO acknowledge.
