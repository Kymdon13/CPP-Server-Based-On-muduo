#include "Buffer.h"

#include <string.h>
#include <iostream>

Buffer::Buffer() {}

Buffer::~Buffer() {}

size_t Buffer::size(){
    return _buffer.size();
}

void Buffer::append(const char *str, size_t size){
    for (int i = 0; i < size; ++i) {
        if(str[i] == '\0') break;
        _buffer.push_back(str[i]);
    }
}

const char* Buffer::c_str() {
    return _buffer.c_str();
}

void Buffer::getline() {
    clear();
    std::getline(std::cin, _buffer);
}

void Buffer::clear() {
    _buffer.clear();
}
