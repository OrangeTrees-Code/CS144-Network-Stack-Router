#include "router.hh"
#include "debug.hh"

#include <iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
  const uint8_t prefix_length,
  const optional<Address> next_hop,
  const size_t interface_num)
{
  // cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/"
  //   << static_cast<int>(prefix_length) << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)")
  //   << " on interface " << interface_num << "\n";

  // debug("unimplemented add_route() called");

  route_table_.emplace_back(Entry(route_prefix, prefix_length, next_hop, interface_num));

  isSorted = false;
}

bool Router::isMatch(Entry& entry, InternetDatagram& dgram) {
  uint32_t mask = entry.prefix_length == 0 ? 0 : (0xFFFFFFFF << (32 - entry.prefix_length));
  uint32_t route = entry.route_prefix & mask;
  uint32_t dst = dgram.header.dst & mask;
  if (route == dst) {
    return true;
  }
  return false;
}

void Router::doRoute(InternetDatagram& dgram) {
  for (auto& entry : route_table_) {
    if (isMatch(entry, dgram) == true) {
      if (entry.next_hop.has_value()) {
        interface(entry.interface_num)->send_datagram(dgram, entry.next_hop.value());
      }
      else {
        interface(entry.interface_num)->send_datagram(dgram, Address::from_ipv4_numeric(dgram.header.dst));
      }
      return;
    }
  }
}
// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // debug("unimplemented route() called");
  if (isSorted == false) {
    sort(begin(route_table_), end(route_table_), [](Entry& e1, Entry& e2) {return e1.prefix_length > e2.prefix_length;});
    isSorted = true;
  }

  for (auto& interface : interfaces_) {
    auto& que = interface->datagrams_received();
    InternetDatagram dgram;
    while (que.empty() == false) {
      dgram = que.front();
      que.pop();
      if (dgram.header.ttl <= 1) {
        continue;
      }
      dgram.header.ttl--;
      dgram.header.compute_checksum();
      doRoute(dgram);
    }
  }
}
