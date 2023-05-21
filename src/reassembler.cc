#include "reassembler.hh"
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.
  available_capacity_ = output.available_capacity();
  first_unacceptable_index_ = first_unassembled_index_ + available_capacity_;

  /* special case: data is already pushed or beyond the scope or no more capacity to hold it. */
  if ( available_capacity_ == 0 || first_index >= first_unacceptable_index_
       || first_index + data.size() < first_unassembled_index_ ) {
    return;
  }
  if ( is_last_substring ) {
    is_end_ = true;
    end_index_ = first_index + data.size();
  }
  /* left side of data is already pushed */
  if ( first_index <= first_unassembled_index_ && first_index + data.size() > first_unassembled_index_ ) {
    data = data.substr( first_unassembled_index_ - first_index );
    first_index = first_unassembled_index_;
  }
  /*right side of data is out of scope */
  if ( first_index + data.size() > first_unacceptable_index_ ) {
    data = data.substr( 0, first_unacceptable_index_ - first_index );
  }

  store( first_index, data );

  auto it = unassembled_data_.begin();
  while ( it != unassembled_data_.end() && it->first == first_unassembled_index_ ) {
    output.push( it->second );
    first_unassembled_index_ += it->second.size();
    it = unassembled_data_.erase( it );
  }
  /* close the writer */
  if ( is_end_ && first_unassembled_index_ == end_index_ && unassembled_data_.empty()) {
    output.close();
  }
}

/* deal with left and right substring merge */
void Reassembler::store( uint64_t first_index, string data )
{
  auto it = unassembled_data_.lower_bound( first_index );
  /* merge previous substring */
  if ( it != unassembled_data_.begin() ) {
    auto prev_it = std::prev( it );
    const uint64_t prev_end_idx = prev_it->first + prev_it->second.size();
    if ( prev_end_idx >= first_index ) {
      if ( prev_end_idx >= first_index + data.size() ) { 
        data = prev_it->second;
      } else {
        string prev_str = prev_it->second.substr( 0, first_index - prev_it->first );
        data = prev_str + data;
      }
      first_index = prev_it->first;
      it = unassembled_data_.erase( prev_it );
    }
  }
  /* find the next substr could be merged */
  while (it != unassembled_data_.end() && first_index + data.size() >= it->first) {
    if ( first_index + data.size() < it->first + it->second.size() ) {
      data += it->second.substr( first_index + data.size() - it->first );
    }
    it = unassembled_data_.erase( it );
  }

  unassembled_data_[first_index] = data;

}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  uint64_t bytes = 0;
  for ( const auto& elem : unassembled_data_ ) {
    bytes += elem.second.size();
  }
  return bytes;
}
