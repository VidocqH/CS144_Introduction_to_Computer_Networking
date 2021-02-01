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
        seg.header().syn = true;
        send_segment(seg);
        _syn = true;
        return;
    }
    _window_is_zero = _window_size == 0;
    const size_t win = _window_size > 0 ? _window_size : 1;  // See win_size as 1 when it's 0, special case
    size_t remain;                                           // Remaining win_size
    while ((remain = win - (_next_seqno - _receive_ack)) != 0 && _fin == false) {
        size_t len = min(remain, TCPConfig::MAX_PAYLOAD_SIZE);
        TCPSegment seg;
        string data = _stream.read(len);
        seg.payload() = Buffer(move(data));
        // EOF, and win_size enough, send fin
        if (_stream.eof() && seg.length_in_sequence_space() < win) {
            seg.header().fin = true;
            _fin = true;
        }
        // seg empty
        if (seg.length_in_sequence_space() == 0)
            return;
        send_segment(seg);
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
    const size_t abs_ackno = unwrap(ackno, _isn, _checkpoint);
    if (abs_ackno > _next_seqno)  // Outbounded ackno, those segs did not send.
        return;
    _window_size = window_size;     // Sync window size
    if (abs_ackno <= _receive_ack)  // Ackno has been acked
        return;
    _receive_ack = abs_ackno;  // Sync receive_ack
    _checkpoint = abs_ackno;   // Set checkpoint

    while (_segments_in_flight.size()) {
        TCPSegment seg = _segments_in_flight.front();
        if (unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() <= abs_ackno) {
            _bytes_in_flight -= seg.length_in_sequence_space();
            _segments_in_flight.pop();
        } else
            break;
    }
    fill_window();
    _retrans_timeout = _initial_retransmission_timeout;  // Reset timeout clock
    _consecutive_retrans = 0;                            // Reset consecutive_retransmissions
    if (_segments_in_flight.size()) {
        _clock_is_counting = true;
        _clock = 0;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _clock += ms_since_last_tick;
    if (_clock >= _retrans_timeout && _segments_in_flight.size()) {
        _segments_out.push(_segments_in_flight.front());
        _consecutive_retrans++;
        _clock_is_counting = true;
        _clock = 0;
        if (_window_is_zero == false)
            _retrans_timeout *= 2;
    }
    if (_segments_in_flight.empty())
        _clock_is_counting = false;
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retrans; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}

void TCPSender::send_segment(TCPSegment seg) {
    seg.header().seqno = wrap(_next_seqno, _isn);        // Set seqno
    _next_seqno += seg.length_in_sequence_space();       // Update seqno
    _bytes_in_flight += seg.length_in_sequence_space();  // Update in flight
    _segments_in_flight.push(seg);                       // Backup seg
    _segments_out.push(seg);                             // Send segment
    if (_clock_is_counting == false) {
        _clock_is_counting = true;  // Start clock
        _clock = 0;                 // Reset clock
    }
}
