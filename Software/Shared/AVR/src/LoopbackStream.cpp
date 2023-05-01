#include "LoopbackStream.h"

LoopbackStream::LoopbackStream(uint16_t buf_size) {
  this->buffer = (uint8_t*) malloc(buf_size);
  this->buffer_size = buf_size;
  this->pos = 0;
  this->size = 0;
}
LoopbackStream::~LoopbackStream() {
  free(buffer);
}

void LoopbackStream::clear() {
  this->pos = 0;
  this->size = 0;
}

int LoopbackStream::read() {
  if (size == 0) {
    return -1;
  } else {
    int ret = buffer[pos];
    pos++;
    size--;
    if (pos == buffer_size) {
      pos = 0;
    }
    return ret;
  }
}

size_t LoopbackStream::write(uint8_t v) {
  if (size == buffer_size) {
    return 0;
  } else {
    uint16_t p = pos + size;
    if (p >= buffer_size) {
      p -= buffer_size;
    }
    buffer[p] = v;
    size++;
    return 1;
  }  
}

int LoopbackStream::available() {
  return size;
}

int LoopbackStream::availableForWrite() {
  return buffer_size - size;
}

bool LoopbackStream::contains(char ch) {
  for (uint16_t i=0; i<size; i++){
	  // TODO: replace modulus with compare. See write(). 
    int p = (pos + i) % buffer_size;
    if (buffer[p] == ch) {
      return true;
    }
  }
  return false;
}

int LoopbackStream::peek() {
  return size == 0 ? -1 : buffer[pos];
}

void LoopbackStream::flush() {
  //I'm not sure what to do here...
}

