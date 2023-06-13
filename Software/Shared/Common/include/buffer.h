#ifndef BUFFER_H__
#define BUFFER_H__

#include <string.h>

/* A Buffer is a simple wrapper for a byte buffer. It allows bytes to be added without always having to explicitly check for
	overflow, but will not overwrite memory and can signal an overflow. It can also add memory blocks and two bytes.
 */
class Buffer {
	uint8_t m_size;
	uint8_t* m_buf;
	uint8_t* m_p;
	bool m_ovf;

public:
	Buffer() : m_size(0), m_buf(NULL) { clear(); }
	Buffer(uint8_t size) : m_size(size), m_buf(new uint8_t[size]) { clear(); }
	/* Buffer(const Buffer& b) : m_size(b.size()), m_buf(new uint8_t[m_size]), m_p(m_buf + b.len()), m_ovf(b.m_ovf) {
		memcpy(m_buf, b.m_buf, b.len());
	} */
	Buffer(const Buffer& b) = delete;
	void resize(uint8_t newsize) {
		if (newsize != size()) {
			const uint8_t* const old_buf = m_buf;
			m_size = newsize;
			m_ovf = (len() > newsize);
			const uint8_t newlen = m_ovf ? newsize : len();
			m_buf = new uint8_t[newsize];
			memcpy(m_buf, old_buf, newlen);
			m_p = m_buf + newlen;
			delete old_buf;
		}
	}
	Buffer& operator = (const Buffer& rhs) {
		if (rhs.size() != size()) { delete m_buf; m_buf = new uint8_t[rhs.size()]; m_size = rhs.size(); }
		memcpy(m_buf, rhs.m_buf, rhs.len());
		m_p = m_buf + rhs.len();
		m_ovf = rhs.m_ovf;
		return *this;
	}
	~Buffer() { delete [] m_buf; }
	uint8_t free() const { return size() - len(); }
	uint8_t size() const { return m_size; }
	uint8_t len() const { return m_p - m_buf; }
	bool ovf() const { return m_ovf; }
	void clear() { m_p = m_buf; m_ovf = false; }
	void add(uint8_t c) { if (free() > 0) *m_p++ = c; else m_ovf = true; }
	void addMem(const uint8_t* buf, uint8_t sz) {
		if (sz > free()) {
			sz = free();
			m_ovf = true;
		}
		memcpy(m_p, buf, sz); m_p += sz;
	}
	void assignMem(const uint8_t* buf, uint8_t sz) {
		if (sz > size()) {
			sz = size();
			m_ovf = true;
		}
		memcpy(m_buf, buf, sz); m_p = m_buf + sz;
	}

	bool addHexStr(const char* str) {
		uint8_t cc = 0U;
		uint8_t v, c;
		while ('\0' != (c = (uint8_t)*str++)) {
			uint8_t v4;
			if ((c >= '0') && (c <= '9'))
				v4 = (uint8_t)(c - '0');
			else if ((c >= 'a') && (c <= 'f'))
				v4 = (uint8_t)(c -'a' + 10);
			else if ((c >= 'A') && (c <= 'F'))
				v4 = (uint8_t)(c -'A' + 10);
			else
				return false;

			if (cc & 1) {
				add((uint8_t)((v << 4) | v4));
				if (ovf()) return false;	// Early exit, save time.
			}
			else
				v = v4;
			cc ^= 1;
		}
		return !cc;		// Fail if odd number of hex chars.
	}
	void addU16_le(uint16_t x) { add(static_cast<uint8_t>(x)); add(static_cast<uint8_t>(x>>8)); }
	void addU16_be(uint16_t x) { add(static_cast<uint8_t>(x>>8)); add(static_cast<uint8_t>(x)); }
	uint16_t getU16_le(uint8_t idx) const {
		return static_cast<uint16_t>(get(idx)) | (static_cast<uint16_t>(get(idx+1))<<8);
	}
	uint16_t getU16_be(uint8_t idx) const {
		return static_cast<uint16_t>(get(idx+1)) | (static_cast<uint16_t>(get(idx))<<8);
	}
	operator const uint8_t*() const { return m_buf; }
	uint8_t get(uint8_t idx) const { return m_buf[idx]; }
};

#if 0
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
#endif

#endif	// BUFFER_H__
