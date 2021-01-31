#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {
    _retrans_timeout = retx_timeout;
}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    // Send syn before any other segments
    if (_syn == false) {
        TCPSegment seg;
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.header().syn = true;
        _seqno = _next_seqno;
        _next_seqno += 1ull;       // Expected ackno
        _bytes_in_flight += 1ull;  // Bytes are flighting
        _segments_out.push(seg);
        _segments_in_flight.push(seg);
        _syn = true;
        return;
    }

    while (_window_size && _stream.buffer_size() && _fin == false) {
        size_t len = _window_size < _stream.buffer_size() ? _window_size : _stream.buffer_size();
        len = len < TCPConfig::MAX_PAYLOAD_SIZE ? len : TCPConfig::MAX_PAYLOAD_SIZE;
        string data = _stream.read(len);
        _window_size -= data.length();
        TCPSegment seg;
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.payload() = Buffer(move(data));
        if (_stream.eof() && _stream.buffer_empty() && _window_size) {
            seg.header().fin = true;
            _fin = true;
        }
        _next_seqno += seg.length_in_sequence_space();
        _bytes_in_flight += seg.length_in_sequence_space();
        _segments_out.push(seg);
        _segments_in_flight.push(seg);
    }

    if (_stream.eof() && _stream.buffer_empty() && _window_size && _fin == false) {
        if (_window_size + _receive_ack <= _next_seqno)
            return;
        TCPSegment seg;
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.header().fin = true;
        _next_seqno += 1ull;
        _bytes_in_flight += 1ull;
        _segments_out.push(seg);
        _segments_in_flight.push(seg);
        _fin = true;
        return;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // The structure of TCP Sender:
    // ______________________________________________________________________
    // Already Acked | Ackno | Waiting for Ackno | Next seqno | Wait to send
    // ----------------------------------------------------------------------
    // If ackno > next_seqno, outbound, discard
    // If ackno <= _receive_ack, already received, sync window size and discard it
    size_t abs_ackno = unwrap(ackno, _isn, _checkpoint);
    if (abs_ackno > _next_seqno)  // Outbounded ackno
        return;
    if (abs_ackno <= _receive_ack) {
        _window_size = window_size;  // Only sync window size
        return;
    }
    if (_fin && (abs_ackno == _next_seqno)) {  // The last ack, client has received fin segment
        _window_size = _bytes_in_flight = 0;
        while (_segments_in_flight.size())
            _segments_in_flight.pop();
        return;
    }
    _window_size = window_size == 0 ? 1 : window_size;   // Sync window size
    _window_is_zero = window_size == 0 ? true : false;   // Reset win sign
    _clock = 0;                                          // Reset clock
    _checkpoint = abs_ackno;                             // Sync checkpoint
    _receive_ack = abs_ackno;                            // Sync receive_ack
    _bytes_in_flight -= (abs_ackno - _seqno);            // Sync bytes_in_flight
    _seqno = abs_ackno;                                  // Sync seqno
    _retrans_timeout = _initial_retransmission_timeout;  // Reset timeout clock
    _consecutive_retrans = 0;                            // Reset consecutive_retransmissions
    while (_segments_in_flight.size() &&
        _segments_in_flight.front().header().seqno.raw_value() +
        _segments_in_flight.front().length_in_sequence_space() <=
        ackno.raw_value())
        _segments_in_flight.pop();  // Sync segment_in_flight
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (_segments_in_flight.size())
        _clock += ms_since_last_tick;
    if (_clock >= _retrans_timeout && _segments_in_flight.size()) {
        _segments_out.push(_segments_in_flight.front());
        _consecutive_retrans++;
        _clock = 0;
        if (_window_is_zero)
            return;
        _retrans_timeout *= 2;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retrans; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
