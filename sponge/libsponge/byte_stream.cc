#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) { this->MemoryCapacity = capacity; }

size_t ByteStream::write(const string &data) {
    size_t count = 0;
    this->input_end = true;
    for (auto eachChar : data) {
        if (this->Data.length() < this->MemoryCapacity) {
            this->Data.push_back(eachChar);
            this->written++;
            count++;
        } else {
            break;
        }
    }
    this->input_end = false;
    return count;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t length = min(this->Data.length(), len);
    return this->Data.substr(0, length);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t length = min(this->Data.length(), len);
    this->Data.erase(0, length);
    this->readden += length;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t length = min(this->Data.length(), len);
    std::string res = peek_output(length);
    pop_output(length);
    return res;
}

void ByteStream::end_input() { this->input_end = true; }

bool ByteStream::input_ended() const { return this->input_end; }

size_t ByteStream::buffer_size() const { return this->Data.length(); }

bool ByteStream::buffer_empty() const { return this->Data.length() == 0; }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return this->written; }

size_t ByteStream::bytes_read() const { return this->readden; }

size_t ByteStream::remaining_capacity() const { return this->MemoryCapacity - this->Data.length(); }
