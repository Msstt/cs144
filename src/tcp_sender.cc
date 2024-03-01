#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

bool TCPSenderTimer::Increase( uint64_t ms_since_last_tick, bool flag ) {
  time_ += ms_since_last_tick;
  if ( time_ >= RTO_ ) {
    time_ = 0;
    if (flag) {
      RTO_ <<= 1;
    }
    count_ += 1;
    return true;
  }
  return false;
}

void TCPSenderTimer::SetZero() {
  RTO_ = initial_RTO_ms_;
  time_ = 0;
  count_ = 0;
}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return now_index_ - ack_index_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return timer_.GetCount();
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if (input_.has_error()) {
    transmit(make_empty_message());
    return;
  }
  TCPSenderMessage msg;
  msg.seqno = isn_;
  if (!started) {
    started = true;
    msg.SYN = true;
    now_index_++;
  } else {
    msg.seqno = isn_ + now_index_;
  }
  auto finish = [&] () {
    buffer_.push_back(msg);
    transmit(msg);
    msg.payload.clear();
    msg.SYN = false;
    msg.FIN = false;
    msg.seqno = isn_ + now_index_;
  };
  while (input_.reader().bytes_buffered() > 0 && ack_index_ + max(1ul, capacity_) - 1 >= now_index_) {
    if (msg.sequence_length() == TCPConfig::MAX_PAYLOAD_SIZE) {
      finish();
    }
    msg.payload.push_back(input_.reader().peek()[0]);
    input_.reader().pop(1);
    now_index_++;
  }
  if (!closed && input_.reader().is_finished() && ack_index_ + max(1ul, capacity_) - 1 >= now_index_) {
    closed = true;
    msg.FIN = true;
    now_index_++;
  }
  if (msg.sequence_length() > 0) {
    finish();
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage result;
  result.seqno = isn_ + now_index_;
  if (input_.has_error()) {
    result.RST = true;
  }
  return result;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if (msg.RST) {
    input_.set_error();
  }
  if (input_.has_error()) {
    return;
  }
  if (msg.ackno != nullopt) {
    auto ack_no = msg.ackno->unwrap(isn_, ack_index_);
    if (ack_no <= now_index_) {
      while (!buffer_.empty() &&
              buffer_.front().seqno.unwrap(isn_, ack_index_) + buffer_.front().sequence_length() <= ack_no) {
        ack_index_ += buffer_.front().sequence_length();
        buffer_.pop_front();
        timer_.SetZero();
      }
    }
  }
  capacity_ = msg.window_size;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if (input_.has_error()) {
    return;
  }
  if (timer_.Increase(ms_since_last_tick, capacity_ > 0 || ack_index_ == 0) && !buffer_.empty()) {
    transmit(buffer_.front());
  }
}
