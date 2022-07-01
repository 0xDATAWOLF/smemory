# smemory - Custom Memory Allocator for C++

Smemory is a custom C++ memory allocator for use in my personal projects. You can
find a considerable amount of documentation within `src/smemory.h`. Due to the
inherit bias towards performance, smemory hogs memory when used as a general purpose
allocator. For this reason, it is not recommended to use smemory in this manner.

Smemory is designed for small-to-medium applications that require fast, successive,
and temporary allocations. Think "frame allocators" used in video games. Smemory
can be used for non-temporary heap allocations, and it is certainly fine to use it
in such a way, it is up to the programmer to ensure efficient use of their allocations. 

# Documentation

Refer to the source file in `src/smemory.h` for documentation. For the time being,
that is the only place I can feasibly maintain it.

### A quick, but non-encompassing rundown:

Allocations with smemory invoke the operating system's virtual allocation function.
For Windows, that's VirtualAlloc. Smemory performs this allocation in groups of pages rather than
arbitrary sizes. These groups of pages are referred to as a journal (because "book" doesn't
make for good tech-orientated nomenclature). All future allocations will go to this
shared journal until the offset pointer reaches the end of the journal. This means
that journals are monotonic in behavior, which is why it is not recommended to use
smemory purely as a general allocator. If there are no more journals available,
smemory will create another and will continue to do so for the lifetime of the
application.

Journals are reclaimed to the operating system explicitly. The programmer must
decide when the optimal time may be to perform this operation. A side effect of
this behavior is that we can use the monotonic behavior *and* this explicit
releasing to the operating system to our advantage. We can create private journals
with the sole purpose of *not* freeing each allocation and flagging it to be reclaimed
by the operating system. This is much faster than unwinding allocations. For
shared journals, they are only reclaimed by the operating system if they
are not flagged with no-reclaim and have a commit of zero. If even one allocation
amongst an otherwise empty journal lingers, it can *not* be deallocated unless you
force it to be reclaimed with force-reclaim.

There are some major details glossed over here, but the general idea is smemory is
tailorable for your needs. Being that it removes the opaque memory management facilities
of the C Standard Library, you are able to control when, where, and how memory comes
in and out of your application.
