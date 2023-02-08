/* Implementation from Adam Dunkels' Protothreads macros. */

#if CFG_LC2_USE_SWITCH

/* The switch implementation works on all compilers. */
typedef unsigned short tlc_t;
#define TLC_INIT(s_) (s_) = 0;
#define TLC_RESUME(s_) switch(s_) { case 0:
#define TLC_SET(s_) (s_) = __LINE__; case __LINE__:
#define TLC_END(s_) }

#else

/* The goto label implementation is special to gcc. */
typedef void* tlc_t;
#define TLC_INIT(s_) ((s_) = NULL)
#define TLC_RESUME(s_)						\
  do {										\
	if (NULL != (s_))						\
	  goto *(s_);							\
  } while(0)

#define TLC_CONCAT2(s1_, s2_) s1_##s2_
#define TLC_CONCAT(s1_, s2_) TLC_CONCAT2(s1_, s2_)

#define TLC_SET(s_)																	\
  do {																				\
	TLC_CONCAT(TLC_LABEL_, __LINE__): (s_) = &&TLC_CONCAT(TLC_LABEL_, __LINE__);	\
  } while(0)

#define TLC_END(s_) /* empty */

#endif

