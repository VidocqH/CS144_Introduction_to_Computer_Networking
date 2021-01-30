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

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    TCPState state = TCPState(_sender, _receiver, _active, _linger_after_streams_finish); // State before receive seg
    TCPHeader header = seg.header();
    if (header.rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _linger_after_streams_finish = false;
        _active = false;
        return;
    }
    _receiver.segment_received(seg); // Receiver received
    _sender.ack_received(header.ackno, header.win); // Sender received
    _time_since_last_segment_received = 0; // Reset Clock
    cout<<state.name()<<endl;
    // State::FIN_WAIT_2 ----(FIN=ACK=1)---> State::TIME_WAIT
    if (header.ack && header.fin && (state == TCPState::State::FIN_WAIT_1)) {
        TCPSegment push_seg;
        push_seg.header().ack = true;
        push_seg.header().ackno = header.seqno + 1;
        push_seg.header().seqno = header.ackno + 1;
        cout<<"ackno---: "<<push_seg.header().ackno.raw_value()<<endl;
        _segments_out.push(push_seg); // Then wait for a while, then close
        return;
    }

    // State::ESTABLISHED ----(FIN=1)---> State::CLOSE_WAIT
    if (header.fin && (state == TCPState::State::ESTABLISHED || state == TCPState::State::CLOSE_WAIT)) {
        TCPSegment push_seg;
        push_seg.header().ack = true;
        push_seg.header().ackno = header.seqno + 1;
        _segments_out.push(push_seg); // Now enter CLOSE_WAIT
        _linger_after_streams_finish = false;
        _is_passive_close = true; // Set sign
        return;
    }

    // State::LAST_ACK ----(ACK=1)---> State::CLOSED
    if (header.ack && header.ackno == _sender.next_seqno() && state == TCPState::State::LAST_ACK) {
        _active = false;
        // _linger_after_streams_finish = false; // Already set in CLOSE_WAIT
        return;
    }

    if (header.ack && state == TCPState::State::ESTABLISHED &&
        (header.seqno.raw_value() < _receiver.ackno().value().raw_value() ||
        header.seqno.raw_value() - header.ackno.raw_value() >= _cfg.recv_capacity
        )) {
        TCPSegment push_seg;
        push_seg.header().ack = true;
        push_seg.header().ackno = _receiver.ackno().value();
        _segments_out.push(seg);
        return;
    }
    if (header.ack && state == TCPState::State::ESTABLISHED &&
    (seg.payload().size() && header.seqno.raw_value() - header.ackno.raw_value() != seg.payload().size())) {
        TCPSegment push_seg;
        push_seg.header().ack = true;
        push_seg.header().ackno = _receiver.ackno().value() + 1;
        _segments_out.push(seg);
        return;
    }

    _sender.fill_window();
    while (_sender.segments_out().size()) {
        TCPSegment push_seg;
        push_seg = _sender.segments_out().front();
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
        cout<<"ackno+++: "<<push_seg.header().ackno.raw_value()<<endl;
        _segments_out.push(push_seg);
        if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
            _receiver.stream_out().set_error();
            _sender.stream_in().set_error();
            push_seg.header().rst = true;
            push_seg.header().ackno = WrappingInt32{0};
            _segments_out.push(push_seg);
            return;
        }
    }
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    size_t res = _sender.stream_in().write(data);
    _sender.fill_window();
    while (_sender.segments_out().size()) {
        TCPSegment seg;
        seg = _sender.segments_out().front();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true; // ACK have set
            seg.header().ackno = _receiver.ackno().value();
        } else {
            seg.header().ack = false; // ACK not set
            seg.header().ackno = WrappingInt32{0};
        }
        seg.header().win = _receiver.window_size();
        _sender.segments_out().pop();
        cout<<"ackno$$$: "<<seg.header().ackno.raw_value()<<endl;
        _segments_out.push(seg);
        if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
            _receiver.stream_out().set_error();
            _sender.stream_in().set_error();
            seg.header().rst = true;
            seg.header().ackno = WrappingInt32{0};
            _segments_out.push(seg);
            return res;
        }
    }
    return res;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (_active == false) // Active return none
        return;
    _time_since_last_segment_received += ms_since_last_tick; // Syn Clock
    _sender.tick(ms_since_last_tick); // Sender Clock
    TCPState state = TCPState(_sender, _receiver, _active, _linger_after_streams_finish);
    // State::TIME_WAIT ----(Timeout)---> State::CLOSED
    if (state == TCPState::State::TIME_WAIT && _time_since_last_segment_received >= _cfg.rt_timeout * 10) {
        _active = false;
        _linger_after_streams_finish = false;
        return;
    }
    // State::FIN_WAIT1,2,CLOSING ----> State::TIME_WAIT
    // State::TIME_WAIT ----(receive ACK)---> State::TIME_WAIT
    if (state == TCPState::State::TIME_WAIT && _time_since_last_segment_received == 1) {
        TCPSegment seg;
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
        if (_resend_in_timewait)
            seg.header().ackno = seg.header().ackno - 1; // Next ackno, -1 to get current
        else 
            _resend_in_timewait = true;
        _segments_out.push(seg);
        return;
    }

    _sender.fill_window();
    while (_sender.segments_out().size()) {
        TCPSegment seg;
        seg = _sender.segments_out().front();
        seg.header().ackno = _receiver.ackno().value();
        cout<<"ackno: "<<seg.header().ackno.raw_value()<<endl;
        if (state == TCPState::State::LAST_ACK && _time_since_last_segment_received < _cfg.rt_timeout)
            return;
        _sender.segments_out().pop();
        _segments_out.push(seg);
        if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
            _receiver.stream_out().set_error();
            _sender.stream_in().set_error();
            seg.header().rst = true;
            _segments_out.push(seg);
            return;
        }
    }
}

void TCPConnection::end_input_stream() {
    // Call CLOSE
    _sender.stream_in().end_input();
    if (_is_passive_close)
        _time_since_last_segment_received = 0;
    _sender.fill_window();
    TCPSegment seg;
    while (_sender.segments_out().size()) {
        seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        seg.header().ackno = _receiver.ackno().value();
        seg.header().ack = true;
        seg.header().fin = true;
        cout<<"ackno///: "<<seg.header().ackno.raw_value()<<endl;
        _segments_out.push(seg);
        if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
            _receiver.stream_out().set_error();
            _sender.stream_in().set_error();
            seg.header().rst = true;
            seg.header().ackno = WrappingInt32{0};
            _segments_out.push(seg);
            return;
        }
    }
}

void TCPConnection::connect() {
    _sender.fill_window();
    TCPSegment seg = _sender.segments_out().front();
    _segments_out.push(seg);
    _active = true;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            unclean_shutdown(true); // send rst seg
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::segment_sends(bool send_syn) {
    if (send_syn || TCPState::state_summary(_receiver) == TCPReceiverStateSummary::SYN_RECV)
        _sender.fill_window();
    TCPSegment seg;
    while (_sender.segments_out().size()) {
        seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true; // ACK have set
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        if (_need_send_rst)
            seg.header().rst = true;
        _segments_out.push(seg);
    }
}

void TCPConnection::unclean_shutdown(bool send_rst) {
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _active = false;
    if (send_rst) {
        _need_send_rst = true;
        if (_sender.segments_out().empty())
            _sender.send_empty_segment();
        segment_sends();
    }
}
