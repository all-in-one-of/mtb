#include <cstdint>
#include <algorithm>
#include <type_traits>

template <std::size_t N> struct type_of_size_helper { typedef char type[N]; };
template <typename T, std::size_t Size> typename type_of_size_helper<Size>::type& sizeof_for_static_arrays_helper(T(&)[Size]);
#define SIZEOF_ARRAY(pArray) sizeof(sizeof_for_static_arrays_helper(pArray))


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

	operator char const* () const { return p; }
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
			obj.dtor();
		}
	};

	template <typename ... Args>
	void ctor(Args&&... args) {
		::new(&mData) T(std::forward<Args>(args)...);
	}
	template <typename ... Args>
	sScope ctor_scoped(Args&&... args) {
		::new(&mData) T(std::forward<Args>(args)...);
		return sScope(*this);
	}
	void dtor() {
		T& t = get();
		t.~T();
	}
	T& get() { return *reinterpret_cast<T*>(&mData); }
	T const& get() const { return *reinterpret_cast<T const*>(&mData); }

	operator T const& () const { return Get(); }
};


