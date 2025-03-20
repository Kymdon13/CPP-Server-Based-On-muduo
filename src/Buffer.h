#ifndef BUFFER_H_
#define BUFFER_H_

#include <string>

class Buffer {
private:
    std::string buf;
public:
    Buffer();
    ~Buffer();
    size_t size();
    void append(const char* _str, size_t _size);
    const char* c_str();
    void getline();
    void clear();
};

#endif // BUFFER_H_
