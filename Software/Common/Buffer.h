// A little class to hold a buffer for chars from a device. It cannot overflow, and can be tested for overflow, and return the current size of the content.

#ifndef BUFFER_H__
#define BUFFER_H__

class Buffer {
	uint8_t* _buf;
	uint8_t* _end;
	uint8_t* _p;
	bool _overflow;
public:
	Buffer(uint8_t sz=0)  : _buf(new uint8_t[sz]), _end(_buf + sz) {
		reset(); 
	}
	~Buffer() { 
		delete(_buf); 
	}
	void resize(uint8_t sz) {
		delete _buf;
		_buf = new uint8_t[sz];
		_end = _buf + sz;
		reset(); 
	}
	void reset() {  
		_p = _buf; 
		_overflow = false;
	}
	void add(uint8_t c) { 
		if (free() > 0)
			*_p++ = c;
		else
			_overflow = true;		// Signal overflow.
	}
	bool overflow() const { return _overflow; }
	uint8_t len() const { return _p - _buf; }
	uint8_t free() const { return _end - _p; }
	operator const uint8_t*() const { return (const uint8_t*)_buf; }
};

#endif // BUFFER_H__
