#include <bits/stdc++.h>
#include <cassert>
using namespace std;

/*
Implement WriteOnlyMap and ReadOnlyMap so that updates applied to a WriteOnlyMap
on computer A are reflected in a ReadOnlyMap on computer B. The network is
- is one-directional (A→B)
- is reliable (packets eventually arrive)
- may deliver packets out of order
*/

class ConnectedSender {
public:
  virtual ~ConnectedSender() = default;
  // Implementation guarantees:
  //  - Non-blocking.
  //  - Either all 'buffer_length' bytes are sent in a single packet, or none
  //  are.
  //  - Returns whether the bytes were sent.
  //  - It is not possible to send an empty packet.
  virtual bool send_packet(const void* source, uint8_t buffer_length) = 0;
};

class ConnectedReceiver {
public:
  virtual ~ConnectedReceiver() = default;
  // Implementation guarantees:
  //  - Non-blocking.
  //  - If a packet is available, this writes the packet to 'destination'
  //    and returns the number of bytes in the packet.
  //  - If no packets are available, this returns 0.
  virtual uint8_t receive_packet(void* destination) = 0;
};


enum class Type : uint8_t {
  SET = 'S',
  ERASE = 'E'
};


struct __attribute__((packed)) SetPacket {
  Type type;
  uint64_t seq;
  int32_t key;
  int val;
};

struct __attribute__((packed)) ErasePacket {
  Type type;
  uint64_t seq;
  int32_t key;
};

using SomePacket = std::variant<SetPacket, ErasePacket>;

// Some general C++ notes I find as a result:
// For working with bytes, used uint8_t == unsigned char (only types that allow arbitrary reinterpretation)
// Only if forwarding without looking at all, use std::byte

class WriteOnlyMap {
public:
  WriteOnlyMap(ConnectedSender& connected_sender) : sender(connected_sender) {}
  void set(int key, int val) {
    enqueue(SetPacket{ Type::SET, seq++, key, val});
  }
  void erase(int key) {
    enqueue(ErasePacket{Type::ERASE, seq++, key});
  }

  // expose separately if client needs to flush
  bool flush() {
    while(!pending.empty()) {
      // Use std::visit to qualify, similar to std::move
      bool sent = std::visit([&](const auto& x){
        return sender.send_packet(&x, sizeof(x));
      }, pending.front());
      if(!sent) return false;
      pending.pop();
    }
    return true;
  }

private:
  ConnectedSender& sender;

  // Our logic might put multiple redundant updates in pending.
  // Common fix is to update entry in-place via a map to index for existing entry. Breaks FIFO
  queue<SomePacket> pending;

  void enqueue(SomePacket p) {
    pending.push(std::move(p));
    flush();
  }
  // Since we don't have negatives, 0 is the sentinel starting seq, so 1 > 0 should be true for the first update
  uint64_t seq = 1;
};

class ReadOnlyMap {
public:
  ReadOnlyMap(ConnectedReceiver& connected_receiver)
      : receiver(connected_receiver) {}
  bool contains(const int key) {
    drain();
    return mp.contains(key) && mp[key].val.has_value();
  }

  int get(const int key) {
    drain();
    assert(mp.contains(key) && mp[key].val.has_value());
    return mp[key].val.value(); 
  }

private:
  struct Info {
    optional<int> val;
    uint64_t seq = 0;
  };
  ConnectedReceiver& receiver;
  map<int, Info> mp;

  void drain() {
    uint8_t buf[256];
    while(uint8_t n = receiver.receive_packet(buf)) {
      // decode [0, n)
      if(buf[0] == 'S') {
        // Note C++ object model of memory AND lifetime. Can't do reinterpret_cast because no SetPacket lifetime exists
        // This is equivalent to declaring a SetPacket and memcpying into it
        SetPacket* p = start_lifetime_as<SetPacket>(buf); 
        int key = p->key;
        auto& [val, seq] = mp[key];
        if(p->seq > seq) {
          val = p->val, seq = p->seq;
        }
      }
      else if(buf[0] == 'E') {
        ErasePacket* p = start_lifetime_as<ErasePacket>(buf);
        int key = p->key;
        auto& [val, seq] = mp[p->key];
        if(p->seq > seq) {
          val = nullopt, seq = p->seq;
        }
      }
    }
  }
};
