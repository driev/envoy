#pragma once

#include <atomic>

#include "envoy/common/time.h"

#include "common/buffer/buffer_impl.h"
#include "common/event/event_impl_base.h"
#include "common/event/file_event_impl.h"
#include "common/network/utility.h"

#include "base_listener_impl.h"

namespace Envoy {
namespace Network {

/**
 * Implementation of Network::Listener for UDP.
 */
class UdpListenerImpl : public BaseListenerImpl,
                        public virtual UdpListener,
                        public UdpPacketProcessor,
                        protected Logger::Loggable<Logger::Id::udp> {
public:
  UdpListenerImpl(Event::DispatcherImpl& dispatcher, SocketSharedPtr socket,
                  UdpListenerCallbacks& cb, TimeSource& time_source, uint64_t max_rx_datagram_size);
  ~UdpListenerImpl() override;
  uint32_t packetsDropped() { return packets_dropped_; }

  // Network::Listener Interface
  void disable() override;
  void enable() override;
  void setRejectFraction(UnitFloat) override {}

  // Network::UdpListener Interface
  Event::Dispatcher& dispatcher() override;
  const Address::InstanceConstSharedPtr& localAddress() const override;
  Api::IoCallUint64Result send(const UdpSendData& data) override;
  Api::IoCallUint64Result flush() override;
  void activateRead() override;

  void processPacket(Address::InstanceConstSharedPtr local_address,
                     Address::InstanceConstSharedPtr peer_address, Buffer::InstancePtr buffer,
                     MonotonicTime receive_time) override;

  uint64_t maxDatagramSize() const override { return max_rx_datagram_size_; }

protected:
  void handleWriteCallback();
  void handleReadCallback();

  UdpListenerCallbacks& cb_;
  uint32_t packets_dropped_{0};

private:
  void onSocketEvent(short flags);
  void disableEvent();

  TimeSource& time_source_;
  const uint64_t max_rx_datagram_size_;
};

class UdpListenerWorkerRouterImpl : public UdpListenerWorkerRouter {
public:
  UdpListenerWorkerRouterImpl(uint32_t concurrency);

  // UdpListenerWorkerRouter
  void registerWorkerForListener(UdpListenerCallbacks& listener) override;
  void unregisterWorkerForListener(UdpListenerCallbacks& listener) override;
  void deliver(uint32_t dest_worker_index, UdpRecvData&& data) override;

private:
  absl::Mutex mutex_;
  std::vector<UdpListenerCallbacks*> workers_ ABSL_GUARDED_BY(mutex_);
};

} // namespace Network
} // namespace Envoy
