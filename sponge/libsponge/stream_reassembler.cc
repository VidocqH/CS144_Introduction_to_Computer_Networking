#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _unassembled_chars(_capacity), _exist_data(_capacity, false) {
    _unassembled_size = 0;
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) {
        _end_idx = index + data.size();
        _eof = true;
    }
    if (_maxIdx > index + data.size())
        return;
    size_t offset = max(index, _maxIdx) - index;
    // TODO: the mod operation can be optimized out, with begin/end index
    for (size_t i = offset; i < data.size(); ++i) {
        // make sure not to overwrite the previous data
        if (_exist_data[(i + index) % _capacity] && _unassembled_chars[(i + index) % _capacity] != data[i])
            break;
        _unassembled_chars[(i + index) % _capacity] = data[i];
        if (!_exist_data[(i + index) % _capacity]) {
            _exist_data[(i + index) % _capacity] = true;
            _unassembled_size++;
        }
    }
    assemble_data();
}

void StreamReassembler::assemble_data() {
    string data_to_assemble = "";
    // Currently cannot make sure how many bytes can write, so use a temp index
    size_t temp_cur_index = _maxIdx;
    while (_exist_data[temp_cur_index % _capacity]) {
        _exist_data[temp_cur_index % _capacity] = false;
        data_to_assemble += _unassembled_chars[temp_cur_index++ % _capacity];
    }
    // now get the exact number written
    size_t nwritten = _output.write(data_to_assemble);
    for (size_t i = nwritten; i < data_to_assemble.size(); ++i) {
        // these data are not actually written
        _exist_data[(_maxIdx + i) % _capacity] = true;
    }
    _unassembled_size -= nwritten;
    _maxIdx += nwritten;
    if (_eof && _maxIdx == _end_idx) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return this->_unassembled_size; }

bool StreamReassembler::empty() const { return this->_unassembled_size == 0; }
