# smemory - Custom Memory Allocator for C++

Smemory is a custom C++ memory allocator for use in my personal projects. You can
find a considerable amount of documentation within `src/smemory.h`. Although it
functions closer to a general purpose allocator, it is designed for applications
that take advantage of temporary allocations that can be easily discarded in
bulk. Due to the inherit bias towards performance, smemory hogs memory when used
as a general purpose allocator. Therefore, it is up to the user (mostly me) to
tailor fit the allocator to the application I am using it.

# Documentation

Refer to the source file in `src/smemory.h` for documentation. For the time being,
that is the only place I can feasibly maintain it.
