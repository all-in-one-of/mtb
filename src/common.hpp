#include <cstdint>

class cstr {
public:
	char const* p;

	cstr() : p(nullptr) {}
	cstr(char const* p) : p(p) {}
};

template <typename T>
struct moveable_ptr {
	T* p;
	moveable_ptr(T* p = nullptr) : p(p) {}

	moveable_ptr(moveable_ptr& o) = delete;
	moveable_ptr& operator=(moveable_ptr& o) = delete;

	moveable_ptr(moveable_ptr&& o) : p(o.p) { o.p = nullptr; }
	moveable_ptr& operator=(moveable_ptr&& o) { p = o.p; o.p = nullptr; return *this; }

	T* operator->() const { return p; }
	T& operator*() const { return *p; }

	operator T*() const { return p; }

	void reset() { p = nullptr; }
};

template <typename T>
class GlobalSingleton {
	typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type mData;
public:
	template <typename ... Args>
	void Ctor(Args&&... args) {
		::new(&mData) T(std::forward<Args>(args)...);
	}
	void Dtor() {
		T& t = Get();
		t.~T();
	}
	T& Get() { return *reinterpret_cast<T*>(&mData); }
};
