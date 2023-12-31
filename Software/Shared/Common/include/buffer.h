#ifndef BUFFER_H__
#define BUFFER_H__

#include <string.h>

/* A BufferDynamic is a simple wrapper for a byte buffer. It allows bytes to be added without always having to explicitly check for
	overflow, but will not overwrite memory and can signal an overflow. It can also add memory blocks and two bytes.
 */
class BufferDynamic {
	uint8_t m_size;
	uint8_t* m_buf;
	uint8_t* m_p;

public:
	BufferDynamic(uint8_t size=0) : m_size(size), m_buf(new uint8_t[size]) { clear(); }
	/* BufferDynamic(const BufferDynamic& b) : m_size(b.size()), m_buf(new uint8_t[m_size]), m_p(m_buf + b.len()), m_ovf(b.m_ovf) {
		memcpy(m_buf, b.m_buf, b.len());
	} */
	BufferDynamic(const BufferDynamic& b) = delete;
	void resize(uint8_t newsize) {
		if (newsize != size()) {
			uint8_t* const new_buf = new uint8_t[newsize];
			if (len() > newsize) {
				memcpy(new_buf, m_buf, newsize);
				m_p = nullptr;
			}
			else {
				memcpy(new_buf, m_buf, len());
				m_p = new_buf + len();
			}
			m_size = newsize;
			delete m_buf;
			m_buf = new_buf;
		}
	}
	BufferDynamic& operator = (const BufferDynamic& rhs) {
		if (rhs.size() != size()) { delete m_buf; m_buf = new uint8_t[rhs.size()]; m_size = rhs.size(); }
		memcpy(m_buf, rhs.m_buf, rhs.len());
		m_p = rhs.ovf() ? nullptr : (m_buf + rhs.len());
		return *this;
	}
	~BufferDynamic() { delete [] m_buf; }
	uint8_t free() const { return size() - len(); }
	uint8_t size() const { return m_size; }
	uint8_t len() const { return ovf() ? size() : (m_p - m_buf); }
	bool ovf() const { return (nullptr == m_p); }
	void clear() { m_p = m_buf; }
	void add(uint8_t c) {
		if (!ovf()) {
			if (free() >= 1) *m_p++ = c;
			else              m_p = nullptr;
		}
	}
	void addMem(const uint8_t* buf, uint8_t sz) {
		if (!ovf()) {
			if (sz > free()) { memcpy(m_p, buf, free()); m_p = nullptr;	}
			else { memcpy(m_p, buf, sz); m_p += sz; }
		}
	}
	void assignMem(const uint8_t* buf, uint8_t sz) {
		if (sz > size()) { memcpy(m_buf, buf, size()); m_p = nullptr; }
		else {             memcpy(m_buf, buf, sz); m_p = m_buf + sz;  }
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


// Uses a static buffer and a pointer to next free location, which is NULL on overflow.
template <const uint8_t CAPACITY_>
class SBuffer {
	uint8_t m_buf[CAPACITY_];
	uint8_t* m_p;		// NULL on overflow.
public:
	SBuffer() { clear(); }

	void clear() { m_p = m_buf; }
	uint8_t size() const { return CAPACITY_; }
	uint8_t free() const { return size() - len(); }
	uint8_t len() const { return ovf() ? size() : (m_p - m_buf); }
	bool ovf() const { return (nullptr == m_p); }
	operator const uint8_t*() const { return m_buf; }

#if 0
	/* BufferDynamic(const BufferDynamic& b) : m_size(b.size()), m_buf(new uint8_t[m_size]), m_p(m_buf + b.len()), m_ovf(b.m_ovf) {
		memcpy(m_buf, b.m_buf, b.len());
	} */
	BufferDynamic(const BufferDynamic& b) = delete;
	BufferDynamic& operator = (const BufferDynamic& rhs) {
		if (rhs.size() != size()) { delete m_buf; m_buf = new uint8_t[rhs.size()]; m_size = rhs.size(); }
		memcpy(m_buf, rhs.m_buf, rhs.len());
		m_p = rhs.ovf() ? nullptr : (m_buf + rhs.len());
		return *this;
	}
	~BufferDynamic() { delete [] m_buf; }
	void add(uint8_t c) {
		if (!ovf()) {
			if (free() >= 1) *m_p++ = c;
			else              m_p = nullptr;
		}
	}
	void addMem(const uint8_t* buf, uint8_t sz) {
		if (!ovf()) {
			if (sz > free()) { memcpy(m_p, buf, free()); m_p = nullptr;	}
			else { memcpy(m_p, buf, sz); m_p += sz; }
		}
	}
	void assignMem(const uint8_t* buf, uint8_t sz) {
		if (sz > size()) { memcpy(m_buf, buf, size()); m_p = nullptr; }
		else {             memcpy(m_buf, buf, sz); m_p = m_buf + sz;  }
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
	uint8_t get(uint8_t idx) const { return m_buf[idx]; }

#endif
};

#endif	// BUFFER_H__
