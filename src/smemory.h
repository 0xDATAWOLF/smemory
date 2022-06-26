/**
 * ---------------------------------------------------------------------------------------------------------------------
 * smemory - Developed by Christopher N. DeJong, @Github 0xDATAWOLF, June 2022
 * 			 Review the license before using in your projects.
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef SOURCERY_SMEMORY_H
#define SOURCERY_SMEMORY_H

/**
 * ---------------------------------------------------------------------------------------------------------------------
 * Essential Documentation
 * ---------------------------------------------------------------------------------------------------------------------
 * 
 * Documentation will be coming soon.
 * 
 */

/**
 * ---------------------------------------------------------------------------------------------------------------------
 * Type Declarations and Macros
 * ---------------------------------------------------------------------------------------------------------------------
 */
#define BYTES(n)	 (size_t)(n)
#define KILOBYTES(n) (size_t)(BYTES(n)*1024)
#define MEGABYTES(n) (size_t)(KILOBYTES(n)*1024)
#define GIGABYTES(n) (size_t)(MEGABYTES(n)*1024)
#define TERABYTES(n) (size_t)(GIGABYTES(n)*1024)

/**
 * Since static qualifiers differ based on their useage, we are defining a series
 * of more explicit macros to differentiate them.
 */
#define persist static
#define global static
#define internal static

/**
 * Qualifiers, used for reference.
 */
#define _SMEM_OUT
#define _SMEM_OUT_OPT
#define _SMEM_IN
#define _SMEM_IN_OPT
#define _SMEM_VOID

/**
 * Provides primitive type information for integers, reals, and bools.
 */
typedef uint8_t			u8;
typedef uint16_t		u16;
typedef uint32_t		u32;
typedef uint64_t		u64;
typedef int8_t			i8;
typedef int16_t			i16;
typedef int32_t			i32;
typedef int64_t			i64;
typedef i32				b32;
typedef i64				b64;
typedef float			r32;
typedef double			r64;
#include <iostream>

/** Used internally by smemory. Do not use. */
#define __SMEM_INTERNAL_GET_INSTANCE() smemory& _smem = smemory::_get()
#define __SMEM_INTERNAL_DEFAULT_JLUPTBL_PAGES 16
#define __SMEM_INTERNAL_DEFAULT_ALIGNMENT 8
#define __SMEM_INTERNAL_DEFAULT_LUPTABLE_VADDR TERABYTES(1)
#define __SMEM_INTERNAL_DEFAULT_MANAGED_VADDR TERABYTES(2)

/**
 * ---------------------------------------------------------------------------------------------------------------------
 * SMemory Declaration
 * ---------------------------------------------------------------------------------------------------------------------
 */

class smemory
{
	public:
		/** Initializes smemory. */
		static void 	init(_SMEM_VOID);
		/** 
		 * Initializes smemory. Specifying jluptable_pages will determine the number
		 * of pages reserved for the journal lookup table. Alignment determines
		 * the memory alignment for allocations. The value for init_journal is
		 * the number of pages that will be generated for the first journal.
		 * If init_journal is zero, it will not initialize a journal during initialization
		 * and will instead be generated as needed.
		 */
		static void 	init(_SMEM_IN u32 jluptable_pages, _SMEM_IN_OPT u32 alignment = 8, _SMEM_IN_OPT u32 init_journal = 0);
		/** Free a region of memory allocated by smemory. */
		static void		free(_SMEM_IN void*);
		/** Allocates a region of memory managed by smemory. */
		static void*	alloc(_SMEM_IN size_t, _SMEM_IN_OPT u32);
		/** Returns the size of the operating system's page in bytes. */
		static size_t	page_size(_SMEM_VOID);

	protected:
		/** Returns the singleton instance of smemory. */
		static smemory& _get();

	protected:
		smemory();
		smemory(u32, u32);
		~smemory();

		/**
		 * Allocates memory using the OS's virtual allocation function. The vaddress
		 * parameter specifies where in, in virtual address space, the allocation should
		 * be made to. Allocations are done in terms of pages where the allocation size
		 * is n-pages times the number of pages requested. The out parameter, alloc_size,
		 * takes a pointer to a size_t. When the allocation is completed, alloc_size is set
		 * to the size of the allocation in bytes.
		 */ 
		void* 	_virtual_alloc(_SMEM_IN_OPT void* vaddress, _SMEM_IN u32 pages, _SMEM_OUT size_t* alloc_size);
		/**
		 * Returns a pointer to a JOURNAL_DESCRIPTOR that will fit n-bytes. If no
		 * journal exists that fits n-bytes, it will create a journal that will fit
		 * at least n-bytes.
		 */
		void* 	_get_avail_journal(_SMEM_IN size_t);
		/**
		 * Creates a journal with n-pages. The specified flags describes the journal
		 * being created.
		 */
		void*	_create_journal(_SMEM_IN u32 pages, _SMEM_IN u32 flags);

		void* 	_vaddress_offset;
		void* 	_vaddress_base;

		size_t 	_page_size;

		void* 	_journal_luptable_base;
		u32 	_journal_luptable_pages;
		u32 	_journal_luptable_count;

		u32 	_alloc_alignment;

	public:

		/** Describes a journal of pages. */
		struct JOURNAL_DESCRIPTOR
		{
			/** Describes the amount of memory in-use by the journal. */
			u32 commit;
			/** Describes of the offset location for the next allocation. */
			u32 allocation_offset;
			/** Number of pages contained within the journal. */
			u32 npages;
			/** Flags associated with the journal. */
			u32 flags;
		};

		/** Describes an allocation that resides within a journal. */
		struct ALLOC_DESCRIPTOR
		{
			/** The total size, in bytes, of the allocation. */
			u32 commit;
			/** The offset, in bytes, to the journal the allocation resides in. */ 
			u32 journal_offset;
		};

		/** Flags that describe a journal descriptor. */
		enum class JOURNAL_DESC_FLAGS: u32
		{
			/**
			 * Allows the journal to be used for general allocations. If this is not
			 * set, the journal is considered private and will not be used for
			 * general allocations.
			 * */
			SHARED = 0x0001,
			/**
			 * Disallows the journal to be reclaimed by the operating system. This
			 * is overriden by FORCERECLAIM and will be reclaimed.
			 * */
			NORECLAIM = 0x0002,
			/** 
			 * Forces the journal to be reclaimed by the operating system regardless
			 * if there are allocations still committed to the journal. Setting a
			 * journal as NORECLAIM will not override this behavior.
			 * */
			FORCERECLAIM = 0x0004,
		};

};

/**
 * ---------------------------------------------------------------------------------------------------------------------
 * WIN32 Definitions
 * ---------------------------------------------------------------------------------------------------------------------
 */
#if (defined(WIN32) || defined(_WIN32))
#include <windows.h>

smemory::smemory()
{
	// Automatically set the defaults on construction in case init is not called.
	this->_journal_luptable_pages = __SMEM_INTERNAL_DEFAULT_JLUPTBL_PAGES;
	this->_journal_luptable_count = 0;
	this->_vaddress_base = 			(void*)__SMEM_INTERNAL_DEFAULT_MANAGED_VADDR;
	this->_vaddress_offset = 		0;
	this->_alloc_alignment = 		__SMEM_INTERNAL_DEFAULT_ALIGNMENT;

	// Determine the size of pages we receive from the operating system.
	SYSTEM_INFO _sys_info = {};
	GetSystemInfo(&_sys_info);
	this->_page_size = (size_t)_sys_info.dwPageSize;
}

smemory::~smemory()
{
	/**
	 * A note to anyone curious as to why there is no memory cleanup on deconstruction:
	 * 
	 * Smemory resides as a local static variable and inherits all the properties of
	 * global static variable scoped to the function it resides within. As such,
	 * smemory will only deconstruct when the application exits. We can take advantage
	 * of the fact that the operating system will do the reclamation for us once
	 * the application closes and therefore we do not need to free our existing
	 * allocations.
	 */
}

smemory& smemory::_get()
{
	persist smemory _smem = {};
	return _smem;
}

void* smemory::_virtual_alloc(void* vaddress, u32 pages, size_t* alloc_size)
{
	*alloc_size = pages * _page_size;
	LPVOID _allocation_ptr = VirtualAlloc((LPVOID)vaddress, (SIZE_T)*alloc_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	return (void*)_allocation_ptr;
}

void* smemory::_create_journal(u32 pages, u32 flags)
{

	// Virtually allocate the journal.
	void* _allocation_vaddr = (void*)((u64)_vaddress_base + (u64)_vaddress_offset);
	size_t _allocation_size = {};
	void* _allocation_ptr = _virtual_alloc(_allocation_vaddr, pages, &_allocation_size);

	// Initialize the journal descriptor.
	JOURNAL_DESCRIPTOR* _jdescriptor = (JOURNAL_DESCRIPTOR*)_allocation_ptr;
	_jdescriptor->commit = 	0;
	_jdescriptor->npages = 	pages;
	_jdescriptor->flags = 	flags;

	// Add it as an entry to the journal lookup table. What you are witnessing is not
	// for the faint of heart. Here be dragons, yada yada.
	*((void**)this->_journal_luptable_base + (this->_journal_luptable_count++)) = _allocation_ptr;

	return _allocation_ptr;

}

void smemory::init()
{
	// The constructor will automatically set defaults.
	__SMEM_INTERNAL_GET_INSTANCE();
	size_t _jluptable_alloc_size = {};
	_smem._journal_luptable_base = _smem._virtual_alloc((void*)__SMEM_INTERNAL_DEFAULT_LUPTABLE_VADDR,
		_smem._journal_luptable_pages, &_jluptable_alloc_size);
	return;
}

void smemory::init(u32 jluptable_pages, u32 alignment, u32 init_journal)
{
	// Set smemory member properties.
	__SMEM_INTERNAL_GET_INSTANCE();
	_smem._journal_luptable_pages = jluptable_pages;
	_smem._alloc_alignment = alignment;

	// Generate the journal lookup table.
	size_t _jluptable_alloc_size = {};
	_smem._journal_luptable_base = _smem._virtual_alloc((void*)__SMEM_INTERNAL_DEFAULT_LUPTABLE_VADDR,
		_smem._journal_luptable_pages, &_jluptable_alloc_size);

	// Creates a journal at on initialization time if specified.
	if (init_journal)
	{
		_smem._create_journal(init_journal, (u32)(JOURNAL_DESC_FLAGS::SHARED));
	}

	return;
}

#endif

#endif