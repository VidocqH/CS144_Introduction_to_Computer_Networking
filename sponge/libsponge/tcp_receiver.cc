#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    const TCPHeader &header = seg.header();
    _seqno = seg.header().seqno;
    // syn sign sets, represents receive a connection request
    if (header.syn) {
        if (_syn == true) return;
        _syn = header.syn; // Set syn sign
        _ackno = _seqno + 1; // Data index
        _isn = _seqno; // Store isn
    }
    cout<<"Accept Data:"<<seg.payload().copy()<<endl;
    if (_fin == false && _syn == true) {
        size_t checkpoint = _reassembler.stream_out().bytes_written(); // Get checkpoint
        size_t idx = unwrap(_seqno, _isn, checkpoint) - 1; // unwrap-1 to get the bytes index
        size_t writtenBeforeWrite = _reassembler.stream_out().bytes_written();
        _reassembler.push_substring(seg.payload().copy(), idx, seg.header().fin);
        if (_ackno == _seqno) { // Expected bytes arrived, otherwise it's an advanced arrived bytes.
            size_t writtenAfterWrite = _reassembler.stream_out().bytes_written();
            _ackno = WrappingInt32(_ackno.raw_value() + (writtenAfterWrite - writtenBeforeWrite)); // Update ack
        }
    }
    // fin sign sets, represents it's the last request
    if (header.fin) {
        _reassembler.stream_out().end_input(); // End input
        _fin = true; // Set fin sign
        _ackno = WrappingInt32(_ackno.raw_value() + 1); // Accepts fin sign, ack+1
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (_ackno == WrappingInt32{0} || _syn == false) return {};
    return _ackno;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
