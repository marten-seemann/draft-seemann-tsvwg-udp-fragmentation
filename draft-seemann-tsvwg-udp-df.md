---

title: "Configuring UDP Sockets for Don't Fragment for Common Platforms"
abbrev: "UDP-DF"
category: info

docname: draft-seemann-tsvwg-udp-df-latest
submissiontype: IETF  # also: "independent", "editorial", "IAB", or "IRTF"
number:
date:
consensus: true
v: 3
area: "Web and Internet Transport"
workgroup: "Transport and Services Working Group"
keyword:
 - Don't Fragment

author:
 -
    fullname: Marten Seemann
    email: martenseemann@gmail.com

normative:

informative:


--- abstract

TODO Abstract


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
All IPv6 packets effectively have the DF bit set.

{{!RFC8899}} defines Datagram Packetization Layer Path MTU Discovery (DPLPMTUD),
a mechanism that allows protocols running over UDP to determine the maximum
packet size they can send. Setting the DF bit is crucial for DPLPMTUD, as it
ensures that packets larger than the Path MTU are dropped, allowing the endpoint
to detect the MTU limitation.

QUIC {{!RFC9000}} is one such protocol that runs over UDP and requires DPLPMTUD.
As QUIC implementations typically run in user space, they need to configure the
DF bit on their UDP sockets to perform DPLPMTUD correctly.

This document provides guidance for implementers on how to set the DF bit on UDP
sockets across different platforms.


# Conventions and Definitions

{::boilerplate bcp14-tagged}


# Setting the DF bit

# Linux

IPv4: IP_MTU_DISCOVER, IP_PMTUDISC_DO

IPv6: IPV6_MTU_DISCOVER, IPV6_PMTUDISC_DO

works on dual-stack sockets: yes

# Apple

## On a per-socket basis

IPv4: IP_DONTFRAG, 1

IPv6: IPV6_DONTFRAG, 1

works on dual-stack sockets: no

TODO: describe the failure. Does it apply to IPv4 or IPV6 only, or both?

## Network.framework

TODO: figure out how this works

# Windows

IPv4: IP_DONTFRAG, 1

IPv6: IPV6_DONTFRAG, 1

works on dual-stack sockets: yes. TODO: double check the above

When sending a too large packet, a WSAEMSGSIZE error is returned. TODO: figure
out what exactly triggers this error


# Security Considerations

TODO Security


# IANA Considerations

This document has no IANA actions.


--- back

# Acknowledgments
{:numbered="false"}

TODO acknowledge.
