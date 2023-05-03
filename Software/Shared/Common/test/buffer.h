#ifndef BUFFER_H__
#define BUFFER_H__

#include <string.h>

/* A Buffer is a simple wrapper for a byte buffer. It allows bytes to be added without always having to explicitly check for
	overflow, but will not oveerwrite memory and can signal an overflow. It can also add memory blocks and two bytes.
 */
 
class Buffer {
	uint8_t m_size;
	uint8_t* m_buf;
	uint8_t* m_p;
	bool m_ovf;
public:
	Buffer(uint8_t size) : m_size(size), m_buf(new uint8_t[size]) { clear(); }
	/* Buffer(const Buffer& b) : m_size(b.size()), m_buf(new uint8_t[m_size]), m_p(m_buf + b.len()), m_ovf(b.m_ovf) { 
		memcpy(m_buf, b.m_buf, b.len()); 
	} */
	Buffer(const Buffer& b) = delete;
	Buffer& operator = (const Buffer& rhs) = delete;
	~Buffer() { delete [] m_buf; }
	uint8_t free() const { return size() - len(); }
	uint8_t size() const { return m_size; }
	uint8_t len() const { return m_p - m_buf; }
	bool ovf() const { return m_ovf; }
	void clear() { m_p = m_buf; m_ovf = false; }
	void add(uint8_t c) { if (free() > 0) *m_p++ = c; else m_ovf = true; }
	void add(char c) { add((uint8_t)c); }
	void add(const uint8_t* buf, uint8_t sz) { 
		if (sz > free()) sz = free(); 
		memcpy(m_p, buf, sz); m_p += sz; 
	}
	void add(uint16_t u16) { 
		add((const uint8_t*)&u16, sizeof(u16)); 
	}
	uint8_t operator [](uint8_t idx) const { return m_buf[idx]; }
	operator const uint8_t*() const { return m_buf; }
};

// Uses a static buffer and a pointer to next free location, which is NULL on overflow. 
template <const size_t CAPACITY_>
class SBuffer {
	static constexpr m_size = CAPACITY_;
	uint8_t m_buf[CAPACITY_];		
	uint8_t* m_p;		// NULL on overflow.
public:
	SBuffer() { clear(); }
	// SBuffer(const SBuffer<CAPACITY_>& b) { memcpy(m_buf, b.m_buf, b.len()); m_p = b.ovf() ? NULL : m_buf + b.len(); }
	Buffer(const Buffer& b) = delete;
	Buffer& operator = (const Buffer& rhs) = delete;
	~Buffer() { }
	void clear() { m_p = m_buf; }
	uint8_t size() const { return m_size; }
	uint8_t len() const { return ovf() ? m_size : m_p - m_buf; }	
	uint8_t free() const { return ovf() ? 0 : m_buf + m_size - m_p; }	
	bool ovf() const { return NULL != m_p; }
	void add(uint8_t c) { if (free() > 0) *m_p++ = c; else m_p = NULL; }
	void add(char c) { add((uint8_t)c); }
	void add(const uint8_t* buf, uint8_t sz) { 
	if (!ovf()) {
		if (sz > free()) { sz = free(); 
		memcpy(m_p, buf, sz); m_p += sz; 
	}
	void add(uint16_t u16) { 
		add((const uint8_t*)&u16, sizeof(u16)); 
	}
	uint8_t operator [](uint8_t idx) const { return m_buf[idx]; }
	operator const uint8_t*() const { return m_buf; }
};


#endif	// BUFFER_H__
