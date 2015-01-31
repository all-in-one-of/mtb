#include <cstdint>
#include <algorithm>
#include <type_traits>

struct noncopyable {
	noncopyable() = default;

	noncopyable(noncopyable const&) = delete;
	noncopyable& operator=(noncopyable const&) = delete;
};

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

	T** pp() { return &p; }

	operator T*() const { return p; }

	void reset() { p = nullptr; }
};

template <typename T>
class GlobalSingleton {
	typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type mData;
public:
	struct sScope : noncopyable {
		GlobalSingleton<T>& obj;
		sScope(GlobalSingleton<T>& obj) : obj(obj) { }
		~sScope() {
			obj.Dtor();
		}
	};

	template <typename ... Args>
	void Ctor(Args&&... args) {
		::new(&mData) T(std::forward<Args>(args)...);
	}
	template <typename ... Args>
	sScope CtorScoped(Args&&... args) {
		::new(&mData) T(std::forward<Args>(args)...);
		return sScope(*this);
	}
	void Dtor() {
		T& t = Get();
		t.~T();
	}
	T& Get() { return *reinterpret_cast<T*>(&mData); }
	T const& Get() const { return *reinterpret_cast<T const*>(&mData); }

	operator T const& () const { return Get(); }
};


