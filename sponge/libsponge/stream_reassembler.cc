#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {
    // this->_streams[capacity] = "";
    this->_assembled = 0;
    this->_unassembled_size = 0;
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) {
        this->_maxIdx = index;
    }
    // cout<<data<<endl;
    this->_streams[index] = data;
    if (index == this->_assembled) {
        if (index == 0) {
            this->_output.write(data);
        } else {
            size_t temIdx = 0;
            for (auto c:data) {
                size_t len_last_str = this->_streams[index-1].length();
                if (this->_streams[index-1][len_last_str-1-temIdx] == c) {
                    temIdx++;
                }
            }
            string res = data;
            res.erase(0, temIdx+1);
            this->_output.write(res);
        }
        this->_assembled++;
        while (this->_streams[this->_assembled] != "") {
            size_t temIdx = 0;
            for (auto c:data) {
                size_t len_last_str = this->_streams[index-1].length();
                if (this->_streams[index-1][len_last_str-1-temIdx] == c) {
                    temIdx++;
                }
            }
            string res = data;
            res.erase(0, temIdx+1);
            this->_output.write(res);
            // this->_output.write(this->_streams[this->_assembled]);
            this->_unassembled_size -= this->_streams[this->_assembled].length();
            this->_assembled++;
        }
    } else {
        // this->_streams[index] = data;
        this->_unassembled_size += data.length();
        if (this->_output.remaining_capacity() <= unassembled_bytes()) {
            for (auto i = this->_assembled; i <= this->_maxIdx; i++) {
                if (this->_streams[i] != "") {
                    size_t temIdx = 0;
                    for (auto c:this->_streams[i-1]) {
                        size_t len_last_str = this->_streams[i-1].length();
                        if (this->_streams[index-1][len_last_str-1-temIdx] == c) {
                            temIdx++;
                        }
                    }
                    string res = this->_streams[i];
                    res.erase(0, temIdx+1);
                    this->_output.write(res);
                }
            }
        }
    }
    if (this->_assembled-1 == this->_maxIdx) {
        this->_output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return this->_unassembled_size; }

bool StreamReassembler::empty() const { return this->_unassembled_size == 0; }
