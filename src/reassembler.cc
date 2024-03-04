#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
//  std::cout << first_index << std::endl;
//  std::cout << "data:" << data << std::endl;
//  std::cout << "available_capacity:" << output_.writer().available_capacity() << std::endl;
  if (is_last_substring) {
      eof_ = true;
      eof_index_ = first_index + data.length();
  }
  if (first_index > now_index_ + output_.writer().available_capacity() - 1) {
      return;
  }
  if (first_index + data.length() > now_index_) {
      if (first_index + data.length() > now_index_ + output_.writer().available_capacity()) {
          size_t length = (first_index + data.length()) - (now_index_ + output_.writer().available_capacity());
          data.erase(data.end() - length, data.end());
      }
      if (reassembler_[first_index].length() < data.size()) {
          reassembler_[first_index] = data;
      }
  }
  if (reassembler_.begin()->first <= now_index_) {
      auto it = reassembler_.begin();
      while (it != reassembler_.end() && it->first <= now_index_) {
          if (it->first + it->second.length() > now_index_) {
              int length = it->second.length() - (now_index_ - it->first);
              output_.writer().push(it->second.substr(now_index_ - it->first, length));
              now_index_ += length;
          }
          it++;
      }
      reassembler_.erase(reassembler_.begin(), it);
  }
  if (eof_ && now_index_ >= eof_index_) {
//      std::cout << "first_index:" << first_index << std::endl;
      output_.writer().close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
    if (reassembler_.empty()) {
        return 0;
    }
    uint64_t temp_index = now_index_;
    uint64_t result = 0;
    for (auto [index, data] : reassembler_) {
//        std::cout << index << "--" << data << std::endl;
        if (index + data.length() > max(temp_index, index)) {
            result += index + data.length() - max(temp_index, index);
        }
        temp_index = max(temp_index, index + data.length());
    }
    return result;
}
