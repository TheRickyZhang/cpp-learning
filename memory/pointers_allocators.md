# Pointers
- Subtracting pointers that are not in the same array / allocation is undefined behavior
- Instead, a `reinterpret_cast<uintptr_t>(buf)` is the canonical way to compare raw addresses, and thus perform all sorts of pointer checks. This mostly appears in very low-level code.
- `char\*` and `std::byte\*` are basically the same; only prefer `char\*` when you need the flexibility of looking inside.

## Pointers vs Raw Arrays
While both refer to starting memory address, and a raw array can decay into a pointer, an array contains the elements, while a pointer contains only an address.

The most obvious different manifests in that arrays are inlined storage, while pointers are not, that is:
```cpp
struct foo {
  int[4] a; // is owned by foo, part of memory layout
};
struct bar {
  int* b; // points outside bar, need to have been allocated elsewhere
}
```

# Smart Pointers
- unique_ptr -> owns the T\*, manages lifetime. You can assign multiple to same, but would be bad because of double freeing.
- shared_ptr -> owns the T\*, reference count, destroys object when total count reaches 0.
- weak_ptr -> observer via control block. Only wants the ability to know if an object is still alive, and obtain a shared_ptr if so.

So while shared pointers have T\*, both shared an weak manage control block. \* is deleted when no more shared pointers, and control block deleted when no weak or shared pointers.

A possible issue with naive logic implementation is that in multithreaded context, could access freed memory. We fix by having one extra implicit weak that only gets decremented when strong = 0. (more efficient than each shared impacting the weak count as well)

# Allocators
using AT = std::allocator_traits<A>; // May seem repetitive but is a compatibility + policy layer
::operator new(n \* sizeof(T)) ->  AT::allocate(alloc, n);
::operator delete(a)           ->  AT::deallocate(alloc, a, cap);
construct_at(a+i, ...)         ->  AT::construct(alloc, a+i, ...);
destroy_at(a+i)                ->  AT::destroy(alloc, a+i);

- Use std::allocator<T>, which has std::allocator_traits<T>::{...}, which supports operations like allocate(), deallocate(), construct(), destroy() that calls underlying object.
- Note any mismatch between allocate()/deallocate() and construct()/destroy() leads to UB
- You must destroy constructed objects first before deallocating.
- When you know the type, you can call a.allocate(...) instead of allocator_traits<T>::allocate(a, $$...).$$
- Note that allocate and deallocate MUST be called from the same starting point and size. You should not be looping over elements to deallocate.

To create a custom allocator class, without traits, only need to implement constructor / copy constructor, destructor, allocate() and deallocate();
Use std::align for alignment. Has fairly tricky signature!

Pool vs arena: pool = fixed-size blocks with a free list (freelist.cpp); arena = bump pointer, free everything at once (arena.cpp).

## Placement new
Plain `new T(...)` allocates + constructs (usual form); placement new `new (ptr) T(...)` only constructs at existing memory.

## Regarding std::launder
The fundamental idea behind laundering is that C++ does not model objects by their addresses, but by their lifetimes. So we want to launder when the compiler may have stale assumptions based on the previous lifetime.
I.e. memory laundering: prevents the compiler from tracing where the memory came from, so stale assumptions (e.g. about const members) can't leak through a reinterpret_cast or in-place reconstruction.

If we do something like:
```cpp
T* obj = construct_at(p, std::forward<Args>(args)...);
return obj;
```
The returned T* from construct_at is a NEW lifetime, so no std::launder is needed.
But if we do:
```cpp
construct_at(p, std::forward<Args>(args)...);
return std::launder(reinterpret_cast<T*>(p));
```
Launder is needed, since we are recreating the T* from the memory.

In general, the rule for using is:
- If we have a T* returned by placement new or access a live T*, use it
- If we recover a T* from raw storage using reinterpret_cast, used std::launder
