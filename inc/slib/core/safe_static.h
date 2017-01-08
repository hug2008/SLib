#ifndef CHECKHEADER_SLIB_CORE_SAFE_STATIC
#define CHECKHEADER_SLIB_CORE_SAFE_STATIC

#include "definition.h"

#include "spin_lock.h"

/****************************************************************
 C++ 11 introduces thread-safe static local variable initialization.
 But we need customized implementation to check the storage-status on
 exit especially for Win32 applications, and also to avoid the option
 "-fno-threadsafe-statics" on GCC compilers.
****************************************************************/

#if defined(SLIB_PLATFORM_IS_WIN32)
#	define SLIB_SAFE_STATIC_DESTRUCTOR(TYPE, NAME) static slib::_SafeStaticDestructor<TYPE> _static_destructor_instance_##NAME(&NAME);
#	define SLIB_SAFE_STATIC_CHECK_FREED(NAME) _static_destructor_instance_##NAME.flagFreed
#else
#	define SLIB_SAFE_STATIC_DESTRUCTOR(TYPE, NAME)
#	define SLIB_SAFE_STATIC_CHECK_FREED(NAME) 0
#endif

#define SLIB_SAFE_STATIC(TYPE, NAME, ...) \
	SLIB_ALIGN(8) static sl_uint8 _static_safemem_##NAME[sizeof(TYPE)]; \
	static sl_int32 _static_safeflag_##NAME = 0; \
	TYPE& NAME = *(reinterpret_cast<TYPE*>(_static_safemem_##NAME)); \
	SLIB_STATIC_SPINLOCK(_static_safelock_##NAME); \
	if (_static_safeflag_##NAME == 0) { \
		_static_safelock_##NAME.lock(); \
		if (_static_safeflag_##NAME == 0) { \
			new (&NAME) TYPE(__VA_ARGS__); \
			_static_safeflag_##NAME = 1; \
		} \
		_static_safelock_##NAME.unlock(); \
	} \
	SLIB_SAFE_STATIC_DESTRUCTOR(TYPE, NAME)

#define SLIB_SAFE_STATIC_GETTER(TYPE, FUNC, ...) \
	static TYPE* FUNC() { \
		SLIB_SAFE_STATIC(TYPE, ret, ##__VA_ARGS__) \
		if (SLIB_SAFE_STATIC_CHECK_FREED(ret)) { \
			return 0; \
		} \
		return &ret; \
	}

#define SLIB_STATIC_ZERO_INITIALIZED(TYPE, NAME) \
	SLIB_ALIGN(8) static char _static_safemem_##NAME[sizeof(TYPE)] = {0}; \
	TYPE& NAME = *(reinterpret_cast<TYPE*>(_static_safemem_##NAME)); \
	SLIB_SAFE_STATIC_DESTRUCTOR(TYPE, NAME)


SLIB_NAMESPACE_BEGIN

template <class T>
class _SafeStaticDestructor
{
public:
	T* object;
	volatile sl_bool flagFreed;
	
public:
	SLIB_INLINE _SafeStaticDestructor(T* p)
	: object(p), flagFreed(sl_false)
	{
	}
	
	~_SafeStaticDestructor()
	{
		flagFreed = sl_true;
		object->~T();
	}
	
};

SLIB_NAMESPACE_END


#endif

