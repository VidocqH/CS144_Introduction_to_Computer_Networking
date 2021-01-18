#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();
    const WrappingInt32 seqno = seg.header().seqno;
    bool syn_with_data_sign = false; // To check whether it's a special case: segment with SYN + data
    // syn sign sets, represents receive a connection request
    if (header.syn) {
        if (_syn == true) return; // syn already set, refuse other syn
        _syn = header.syn; // Set syn sign
        _ackno = seqno + 1; // Data index
        _isn = seqno; // Store isn
        syn_with_data_sign = true;
    }
    // If fin sign has set and needs to continue get data, the reassembler must be not empty!
    if ((_fin == false || (_fin == true && _reassembler.empty() == false)) && _syn == true) {
        const size_t checkpoint = _reassembler.stream_out().bytes_written(); // Get checkpoint
        size_t idx = unwrap(seqno, _isn, checkpoint) - 1; // unwrap-1 to get the bytes index
        if (syn_with_data_sign) idx = 0; // The computed index with special case "SYN + data" is a diaster
        const size_t writtenBeforeWrite = _reassembler.stream_out().bytes_written(); // To compute how much should ack adds
        _reassembler.push_substring(seg.payload().copy(), idx, header.fin);
        if (_ackno == seqno || syn_with_data_sign) { // Expected bytes arrived, otherwise it's an advanced arrived bytes.
            const size_t writtenAfterWrite = _reassembler.stream_out().bytes_written(); // To compute how much should ack adds
            _ackno = WrappingInt32(_ackno.raw_value() + (writtenAfterWrite - writtenBeforeWrite)); // Update ack
        }
    }
    // fin sign sets, represents it's the last request
    if (header.fin && _syn) _fin = true;
    // Finish all the bytes
    if (_fin == true && _reassembler.empty()) {
        _ackno = WrappingInt32(_ackno.raw_value() + 1ull); // fin sign occupy one bit, ack+1
        _reassembler.stream_out().end_input(); // End input
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (_ackno == WrappingInt32{0} || _syn == false) return {};
    return _ackno;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
