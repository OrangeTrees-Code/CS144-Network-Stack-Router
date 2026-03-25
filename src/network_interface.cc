#include <iostream>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(string_view name,
  shared_ptr<OutputPort> port,
  const EthernetAddress& ethernet_address,
  const Address& ip_address)
  : name_(name)
  , port_(notnull("OutputPort", move(port)))
  , ethernet_address_(ethernet_address)
  , ip_address_(ip_address)
  , I2E_cache_()
  , pending_()
  , last_arp_time()
  , current_time_(0)
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string(ethernet_address_) << " and IP address "
    << ip_address.ip() << "\n";
}

static EthernetFrame make_frame(const EthernetAddress& src,
  const EthernetAddress& dst,
  const uint16_t type,
  vector<Ref<string>> payload)
{
  EthernetFrame frame;
  frame.header.src = src;
  frame.header.dst = dst;
  frame.header.type = type;
  frame.payload = move(payload);
  return frame;
}

static ARPMessage make_arp(const uint16_t opcode,
  const EthernetAddress sender_ethernet_address,
  const uint32_t& sender_ip_address,
  const EthernetAddress target_ethernet_address,
  const uint32_t& target_ip_address)
{
  ARPMessage arp;
  arp.opcode = opcode;
  arp.sender_ethernet_address = sender_ethernet_address;
  arp.sender_ip_address = sender_ip_address;
  arp.target_ethernet_address = target_ethernet_address;
  arp.target_ip_address = target_ip_address;
  return arp;
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram(const InternetDatagram& dgram, const Address& next_hop)
{
  // debug( "unimplemented send_datagram called" );
  // (void)dgram;
  // (void)next_hop;

  Serializer serializer;
  uint32_t addr = next_hop.ipv4_numeric();

  bool isCached = I2E_cache_.contains(addr);

  if (isCached == true) {
    dgram.serialize(serializer);

    transmit(make_frame(ethernet_address_, I2E_cache_[addr].etherAddr, EthernetHeader::TYPE_IPv4, serializer.finish()));

    return;
  }




  if ((not isCached) and (last_arp_time.contains(addr) == false or current_time_ - last_arp_time[addr] >= 5000)) {

    pending_[addr].clear();

    ARPMessage broadcast = make_arp(ARPMessage::OPCODE_REQUEST, ethernet_address_, ip_address_.ipv4_numeric(), EthernetAddress(), addr);

    broadcast.serialize(serializer);

    EthernetFrame etherFrame = make_frame(ethernet_address_, ETHERNET_BROADCAST, EthernetHeader::TYPE_ARP, serializer.finish());

    transmit(etherFrame);


    last_arp_time[addr] = current_time_;

  }

  pending_[addr].emplace_back(dgram);
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame(EthernetFrame frame)
{
  // debug("unimplemented recv_frame called");
  // (void)frame;
  if (frame.header.dst != ethernet_address_ and frame.header.dst != ETHERNET_BROADCAST) {
    return;
  }
  Parser parser(frame.payload);
  if (frame.header.type == EthernetHeader::TYPE_IPv4) {
    InternetDatagram datagram;
    datagram.parse(parser);

    datagrams_received_.push(datagram);
  }
  else if (frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage msg;
    msg.parse(parser);

    I2E_cache_[msg.sender_ip_address].etherAddr = msg.sender_ethernet_address;
    I2E_cache_[msg.sender_ip_address].expireTime = current_time_ + 30000;

    if (msg.target_ip_address == ip_address_.ipv4_numeric() and msg.opcode == ARPMessage::OPCODE_REQUEST) {
      ARPMessage reply = make_arp(ARPMessage::OPCODE_REPLY, ethernet_address_, ip_address_.ipv4_numeric(), msg.sender_ethernet_address, msg.sender_ip_address);

      Serializer serializer;
      reply.serialize(serializer);

      EthernetFrame replyFrame = make_frame(ethernet_address_, msg.sender_ethernet_address, EthernetHeader::TYPE_ARP, serializer.finish());
      transmit(replyFrame);
    }

    if (pending_.contains(msg.sender_ip_address)) {
      auto& queue = pending_[msg.sender_ip_address];
      for (auto& datagram : queue) {
        Serializer serializer;
        datagram.serialize(serializer);
        transmit(make_frame(
          ethernet_address_,
          I2E_cache_[msg.sender_ip_address].etherAddr,
          EthernetHeader::TYPE_IPv4,
          serializer.finish()
        ));
      }
      pending_.erase(msg.sender_ip_address);
      last_arp_time.erase(msg.sender_ip_address);
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick)
{
  // debug("unimplemented tick({}) called", ms_since_last_tick);
  current_time_ += ms_since_last_tick;

  for (auto it = I2E_cache_.begin();it != I2E_cache_.end();) {
    if (it->second.expireTime <= current_time_) {
      last_arp_time.erase(it->first);
      it = I2E_cache_.erase(it);
    }
    else {
      it++;
    }
  }
}
