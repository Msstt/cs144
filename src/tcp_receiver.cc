#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if (message.RST) {
    reassembler_.set_error();
  }
  if (reassembler_.writer().has_error()) {
    return;
  }
  if (message.SYN) {
    started_ = true;
    isn_ = message.seqno;
  }
  if (!started_) {
    return;
  }
  auto index = message.seqno.unwrap(isn_, reassembler_.expect_index());
  if (!message.SYN) {
    index -= 1;
  }
  reassembler_.insert(index, message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage result;
  if (started_) {
    result.ackno = Wrap32::wrap(reassembler_.expect_index(), isn_) + (reassembler_.writer().is_closed() ? 2 : 1);
  } else {
    result.ackno = nullopt;
  }
  result.window_size = min(static_cast<uint64_t>(UINT16_MAX), reassembler_.writer().available_capacity());
  result.RST = reassembler_.writer().has_error();
  return result;
}
