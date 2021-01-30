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
    if (_active == false)
        return;
    TCPState state = TCPState(_sender, _receiver, _active, _linger_after_streams_finish);
    _time_since_last_segment_received = 0;
    TCPHeader header = seg.header();

    if (TCPState::state_summary(_sender) == TCPSenderStateSummary::SYN_SENT && header.ack && seg.payload().size())
        return;

    bool send_empty = false;
    if (header.ack && _sender.next_seqno_absolute() > 0) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        //TODO: if ack received failed, send empty
    }
    _receiver.segment_received(seg);
    //TODO: if segment reiceved failed, send empty

    if (header.fin && state == TCPState::State::ESTABLISHED)
        _is_passive_close = true;

    if (header.syn && _sender.next_seqno_absolute() == 0) {
        connect(); // Syn
        return;
    }

    if (header.rst) {
        if (TCPState::state_summary(_sender) == TCPSenderStateSummary::SYN_SENT && header.ack == false)
            return;
        unclean_shutdown(false); // Passive rst
        return;
    }

    if (seg.length_in_sequence_space() > 0)
        send_empty = true;

    // send empty segment
    if (send_empty && _receiver.ackno().has_value() && _sender.segments_out().empty())
        _sender.send_empty_segment();
    
    segment_sends();
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    size_t res = _sender.stream_in().write(data);
    segment_sends();
    return res;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (_active == false) // Active return none
        return;
    _time_since_last_segment_received += ms_since_last_tick; // Syn Clock
    _sender.tick(ms_since_last_tick); // Sender Clock
    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS)
        unclean_shutdown(true); // set and send rst
    segment_sends();
}

void TCPConnection::end_input_stream() {
    // Call CLOSE
    _sender.stream_in().end_input();
    if (_is_passive_close)
        _time_since_last_segment_received = 0;
    segment_sends();
}

void TCPConnection::connect() {
    segment_sends(true); // send syn
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
    auto state = TCPState(_sender, _receiver, _active, _linger_after_streams_finish);
    cout<<state.name()<<endl;
    _sender.fill_window();
    while (_sender.segments_out().size()) {
        TCPSegment seg;
        if (state == TCPState::State::LAST_ACK && _time_since_last_segment_received < _cfg.rt_timeout)
            break;
        seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (send_syn)
            seg.header().syn = true;
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true; // ACK have set
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        if (_need_send_rst)
            seg.header().rst = true;
        if (state == TCPState::State::CLOSE_WAIT)
            seg.header().fin = true;
        _segments_out.push(seg);
    }
    clean_shutdown();
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

void TCPConnection::clean_shutdown() {
    if (_receiver.stream_out().input_ended() && _sender.stream_in().eof() == false)
        _linger_after_streams_finish = false;
    if (_receiver.stream_out().input_ended() && _sender.stream_in().eof() && _sender.bytes_in_flight() == 0)
        if (_linger_after_streams_finish == false || time_since_last_segment_received() >= 10 * _cfg.rt_timeout)
            _active = false;
}
