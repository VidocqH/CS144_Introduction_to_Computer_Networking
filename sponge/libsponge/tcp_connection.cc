#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return {}; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _receiver.segment_received(seg);
    TCPHeader header = seg.header();
    _sender.ack_received(header.ackno, header.win);
    _sender.fill_window();
    if (header.ack && header.fin) { // flag=AF
        TCPSegment push_seg;
        push_seg.header().ack = true;
        push_seg.header().ackno = _receiver.ackno().value();
        _segments_out.push(push_seg);
    }
    while (_sender.segments_out().size()) {
        TCPSegment push_seg;
        push_seg = _sender.segments_out().front();
        if (_sender.segments_out().empty())
            push_seg = seg;
        if (header.ack && header.syn) {  // flag=AS
            push_seg.header().ack = true;
            push_seg.header().syn = false;
        }
        if (_receiver.ackno().has_value()) {
            push_seg.header().ack = true; // ACK have set
            push_seg.header().ackno = _receiver.ackno().value();
        } else {
            push_seg.header().ack = false; // ACK not set
            push_seg.header().ackno = WrappingInt32{0};
        }
        push_seg.header().win = _receiver.window_size();
        _sender.segments_out().pop();
        _segments_out.push(push_seg);
    }
}

bool TCPConnection::active() const { return _linger_after_streams_finish; }

size_t TCPConnection::write(const string &data) {
    return _sender.stream_in().write(data);
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { _sender.tick(ms_since_last_tick); }

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    TCPSegment seg;
    if (_sender.segments_out().size()) {
        seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        seg.header().ackno = _receiver.ackno().value();
        seg.header().ack = true;
        _segments_out.push(seg);
    }
}

void TCPConnection::connect() {
    _sender.fill_window();
    TCPSegment seg = _sender.segments_out().front();
    _segments_out.push(seg);
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _receiver.stream_out().set_error();
            _sender.stream_in().set_error();
            TCPSegment seg;
            seg.header().rst = true;
            seg.header().ackno = WrappingInt32{0};
            _segments_out.push(seg);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
