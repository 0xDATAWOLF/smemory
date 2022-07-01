# smemory - Custom Memory Allocator for C++

Smemory is designed for small-to-medium applications that require fast, successive
allocations that can be deallocated in bunk. Think "frame allocators" used in video games.
Smemory can be used for non-temporary heap allocations, and it is certainly fine to use it
in such a way, but it is up to the programmer to ensure efficient use of their allocations.
Smemory handles fragmentation through bulk deallocation of pages, and therefore relies
on the user to understand the lifetimes of their data.

# Documentation

Refer to the source file in `src/smemory.h` for API documentation.

### A quick, but non-encompassing rundown:

Allocations with smemory invoke the operating system's virtual allocation function.
For Windows, that's VirtualAlloc. Smemory performs this allocation in groups of pages rather than
arbitrary sizes. These groups of pages are referred to as a journal (because "book" doesn't
make for good tech-orientated nomenclature). All future allocations will go to this
shared journal until the offset pointer reaches the end of the journal. This means
that journals are monotonic in behavior, which is why it is not recommended to use
smemory purely as a general allocator. If there are no more journals available,
smemory will create another and will continue to do so for the lifetime of the
application. This monotonic behavior is both fast and efficient, but it has potential
for *enormous* memory fragmentation.

Journals are reclaimed to the operating system explicitly. The programmer must
decide when the optimal time may be to perform this operation. We can use the
monotonic behavior *and* this explicit releasing to the operating system to our advantage.
We can create private journals with the sole purpose of *not* freeing each allocation
and then flagging it to be reclaimed by the operating system all at once. This is
much faster than unwinding allocations. For shared journals, they are only reclaimed
by the operating system if they are not flagged with no-reclaim and have a commit of
zero. If even one allocation amongst an otherwise empty journal lingers, it will not
be deallocated until the lingering allocation is freed.

There are some major details glossed over here, but the general idea is smemory is
tailorable for your needs. Being that it removes the opaque memory management facilities
of the C Standard Library, you are able to control when, where, and how memory comes
in and out of your application.

### Future Features List

Here are a list of potential features I will add to smemory.

<table>
	<tr>
		<th>Feature</th>
		<th width="5%">Status</th>
		<th width="65%">Description</th>
	</tr>
	<tr>
		<td>Front-end Journals</td>
		<td>N/A</td>
		<td>
			Journals will be made referencable by allocations pointers. Journals
			can be dynamically created and modifiable by the user. Users can apply
			flags to the journal as desired.
		</td>
	</tr>
	<tr>
		<td>Push/Pop Allocations</td>
		<td>N/A</td>
		<td>
			An extended journal structure with push/pop functionality.
		</td>
	</tr>
	<tr>
		<td>"Slow" Alias Pointers</td>
		<td>N/A</td>
		<td>
			A type of pointer structure that indirects the location of memory it
			points to. This type of pointer is considered slow because it needs to
			translate the referencing location. Allows for "hole correction".
		</td>
	</tr>
	<tr>
		<td>Shared Pointers</td>
		<td>N/A</td>
		<td>
			A shared pointer type that uses reference counting to determine when
			the allocation should be deallocated.
		</td>
	</tr>
</table>
