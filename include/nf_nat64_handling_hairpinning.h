#ifndef _NF_NAT64_HANDLING_HARPINNING_H
#define _NF_NAT64_HANDLING_HARPINNING_H

/**
 * @file
 * Fifth and (officially) last step of the Nat64 translation algorithm: "Handling Hairpinning", as
 * defined in RFC6146 section 3.8.
 */

#include <net/netfilter/nf_conntrack_tuple.h>


void set_hairpinning_active(bool is_active);
/**
 * Checks whether a packet is a hairpin packet.
 *
 * @param outgoing tuple of the packet the NAT64 would send if it's not a hairpin.
 * @return boolean whether the packet's a hairpin packet.
 *
 * A packet is a hairpin if the IPv4 pool contains its destination address.
 */
bool nat64_got_hairpin(struct nf_conntrack_tuple *outgoing);
/**
 * Mirrors the core's behavior by processing skb_in as if it was the incoming packet.
 *
 * @param skb_in the outgoing packet. Except because it's a hairpin, here it's treated as if it was
 *		the one received from the network.
 * @param tuple_in skb_in's tuple.
 * @return whether we managed to U-turn the packet successfully.
 */
bool nat64_handling_hairpinning(struct sk_buff *skb_in, struct nf_conntrack_tuple *tuple_in);


#endif /* _NF_NAT64_HANDLING_HARPINNING_H */