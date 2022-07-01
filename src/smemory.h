/**
 * ---------------------------------------------------------------------------------------------------------------------
 * smemory - Developed by Christopher N. DeJong, @Github 0xDATAWOLF, June 2022
 * Please review the license before using in your projects. You can get in contact if
 * you'd like to use it for commercial projects.
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef SOURCERY_SMEMORY_H
#define SOURCERY_SMEMORY_H
#include <immintrin.h>
#include <emmintrin.h>
#include <stdint.h>
#include <iostream>

/**
 * ---------------------------------------------------------------------------------------------------------------------
 * Essential Documentation
 * ---------------------------------------------------------------------------------------------------------------------
 * 
 * Getting Started
 * 		You can include this file into your project and begin using it right away. There are no dependecies outside of
 * 		OS-level support. Smemory is designed for Windows but may include Linux support in the future. It is assumed that
 * 		you are using smemory on Windows.
 * 
 * 		Additionally, smemory requires that you are using "modern" hardware--your CPU must at least have SSE2 or AVX
 * 		supported in order to make it performant on your system.
 * 
 * 		You must initialize smemory with smemory::init() / smemory::init(_SMEM_IN_OUT SMEMORY_CONFIG*) at the start of
 * 		your application. Otherwise the journal lookup table will not be created and your app will be taken to funky town
 * 		if you attempt to allocate something.
 * 
 * 		For general allocations:
 * 		1. Allocate n-bytes with smemory::alloc(n).
 * 		2. Free that allocation pointer with smemory::alloc().
 * 
 * 		Note:
 * 		Smemory does not automatically release pages back to the operating system. Therefore, it is up to the user to
 * 		periodically (once per runtime loop is recommended) to invoke smemory::reclaim(). This will reclaim any journals
 * 		that have a zero-commit or flagged with FORCERECLAIM. Any journals that with a zero-commit that have the flag
 * 		NORECLAIM will not be reclaimed until the flag is toggled off or flagged with FORCERECLAIM.
 * 
 * Journals
 * 		Journals are a set of contiguous pages given back to use from the operating system when calling the virtual
 * 		allocation function. The term "book" doesn't make for good tech-orientated nomenclature, so that is what I went
 * 		with. As allocations are made, they are put into the first available journal with the capacity to contain it.
 * 		If no such journal is available, one is created with the minimum amount of space required to contain it.
 * 
 * 		Journals persist so long as they have an non-zero-commit. That means that an otherwise empty journal with a single
 * 		lingering allocation will not be reclaimed by the operating system.
 * 
 * General Allocations
 * 		Smemory is not designed to be a general allocator due to the way journals are laid out. Smemory does not track
 * 		individual allocations beyond what is necessary to maintain the journal's state. Therefore, it is up to the user
 * 		to ensure that allocations are free'd in a timely and efficient manner should smemory be used as a general
 * 		allocator.
 * 
 * -----------------------------------------------------------------------------
 * Front-end API
 * -----------------------------------------------------------------------------
 * 
 * smemory::init(_SMEM_VOID)
 * 		Initializes smemory and creates the journal lookup table.
 * 
 * smemory::init(_SMEM_IN_OUT SMEMORY_CONFIG*)
 * 		Initializes smemory with the provided configuration structure and creates
 * 		the journal lookup table. Modifies the provided configuration with defaults
 * 		if they are not provided.
 * 
 * smemory::page_size(_SMEM_VOID)
 * 		Returns the minimum page size used by smemory.
 * 
 * smemory::alloc(_SMEM_IN size_t)
 * 		Allocates n-bytes to the first available journal.
 * 
 * smemory::free(_SMEM_IN void*)
 * 		Frees an allocation and decommits from the associated journal.
 * 
 * smemory::reclaim(_SMEM_VOID)
 * 		Reclaims and decommits a journal back to the operating system. All the
 * 		allocations made in the journal are automatically free'd, but may cause
 * 		lingering pointers to become invalid and may produced undefined behavior.
 * 
 * smemory::memory_set_unaligned(_SMEM_IN void*, _SMEM_IN size_t, _SMEM_IN_OPT uint8_t)
 * 		A memory set routine that will set a region of memory to a given value.
 * 		This routine is much slower than the C Standard Library's implementation
 * 		and is only used to set a region of memory that is unaligned.
 * 
 * smemory::memory_set(_SMEM_IN void*, _SMEM_IN size_t, _SMEM_IN_OPT uint8_t)
 * 		A memory set routine that uses SSE2 / AVX to perform a memory set on
 * 		an aligned boundary. This will automatically check for alignment and will
 * 		correct the alignment. All allocations made using smemory are already made
 * 		to be aligned along the boundary of best fit. For user-defined regions,
 * 		this may not be the case.
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
#define _SMEM_IN_OUT
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

/**
 * ---------------------------------------------------------------------------------------------------------------------
 * Internal Macro Definitions
 * ---------------------------------------------------------------------------------------------------------------------
 */

// Helper macro functions.
#define __SMEM_CONFIG_ZERO_CHECKSET(config, entry, default) config->entry = (config->entry > 0) ? config->entry : default
#define __SMEM_INTERNAL_GET_INSTANCE() smemory& _smem = smemory::_get()

// Defines the default number of pages allocated to the journal lookup table.
#define __SMEM_INTERNAL_DEFAULT_JLUPTBL_PAGES 16

// Sets the starting virtual address for the journal lookup table.
#define __SMEM_INTERNAL_DEFAULT_LUPTABLE_VADDR TERABYTES(1)

// Determines if the smemory should check for alignment in the custom memset.
#define __SMEM_INTERNAL_CHECK_MEMSET_ALIGNMENT 1

// Determines if the free operation should clear the memory to zero when invoked.
#define __SMEM_CLEAR_ON_FREE 1

/**
 * ---------------------------------------------------------------------------------------------------------------------
 * SMemory Declaration
 * ---------------------------------------------------------------------------------------------------------------------
 */

/**
 * Used for initializing smemory.
 */
struct SMEMORY_CONFIG
{
	/** Defines the number of pages reserved for the journal lookup table. */
	_SMEM_IN_OPT u32 journal_luptbl_pages;

	/** Defines the minimum number of pages required for tables generated by smemory. */
	_SMEM_IN_OPT u32 journal_min_pages;

	/** If defined, a journal is created with n-pages at initialization time. */
	_SMEM_IN_OPT u32 journal_create_journal;

	/**
	 * Defines the byte alignment for all allocations. The default alignment
	 * is optimized for performance and is not recommended to be changed.
	 */
	_SMEM_IN_OPT u32 alloc_alignment;

};

/**
 * The journal descriptor heads the first handful of bytes of the pages returned
 * by the operating system's virtual allocation function. It describes the total
 * commit of all active allocations, the allocation offset for the next available
 * allocation, and the number of contiguous pages related to the allocation.
 */
struct JOURNAL_DESCRIPTOR
{
	/** Describes the amount of memory in-use by the journal. */
	u64 commit;

	/** Describes of the offset location for the next allocation. */
	u64 allocation_offset;

	/** Number of pages contained within the journal. */
	u32 npages;

	/** Flags associated with the journal. */
	u32 flags;
	
	/** Reserved to maintain alignment on a 32-byte boundary. */
	u64 _reserved;

};

/**
 * The allocation descriptor precedes an allocation pointer and describes the
 * commit size and journal offset necessary for deallocation.
 */
struct ALLOC_DESCRIPTOR
{
	/** The total size, in bytes, of the allocation. */
	u64 commit;

	/** The offset, in bytes, to the journal the allocation resides in. */ 
	u64 journal_offset;

	/** Padding to preserve 32-byte alignment. */
	u64 _reserved[2];

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

class smemory
{
	public:
		/**
		 * Initializes smemory.
		 */
		static void 	init(_SMEM_VOID void);

		/** 
		 * Initializes smemory with a provided configuration. The config provided
		 * will be returned with the values used to configure smemory. Values that
		 * are set as NULL or zero will be defaulted. 
		 */
		static void 	init(_SMEM_IN_OUT SMEMORY_CONFIG* config);

		/**
		 * Free a region of memory allocated by smemory.
		 */
		static void		free(_SMEM_IN void* addr);

		/**
		 * Allocates a n-bytes of memory to the first available shared journal.
		 */
		static void* 	alloc(_SMEM_IN size_t nbytes);

		/**
		 * Reclaims any journals (SHARED or PRIVATE) with zero-commits back to the
		 * operating system. Any journals marked as NORECLAIM are ignored except if
		 * they are marked as FORCERECLAIM.
		 */
		static void 	reclaim(_SMEM_VOID void);

		/**
		 * Returns the size of the operating system's page in bytes.
		 */
		static size_t	page_size(_SMEM_VOID void);

		/**
		 * Sets a region of memory to a given byte value. If value is not specified,
		 * the region of memory will be set to zero.
		 */
		static void 	memory_set(_SMEM_IN void* addr, _SMEM_IN size_t size, _SMEM_IN_OPT u8 val = 0x00);

		/**
		 * Sets a region of memory to a given byte value that may/may not be aligned
		 * to a particular byte boundary. If the value is not specified, the region
		 * of memory will be set to zero.
		 */
		static void 	memory_set_unaligned(_SMEM_IN void* set_addr, _SMEM_IN size_t size, _SMEM_IN_OPT u8 val = 0x00);

	protected:
		/**
		 * Returns the singleton instance of smemory.
		 */
		static smemory& _get();

		/**
		 * Allocates memory using the OS's virtual allocation function.
		 */ 
		static void* 	_virtual_alloc(_SMEM_IN_OPT void* vaddress, _SMEM_IN u32 pages, _SMEM_OUT size_t* alloc_size);

		/**
		 * Frees memory using the OS's virtual free function. This operation will
		 * release the virtually allocated region back to the operating system and
		 * will make future accesses to the pointers within this region throw.
		 */
		static void 	_virtual_free(_SMEM_IN void* vaddress);

	protected:
		/**
		 * The constructor and deconstructors are set to protected as the smemory
		 * management API is not meant to be manually constructed by the user.
		 */

		smemory();
		smemory(u32, u32);
		~smemory();

		/**
		 * Returns a void pointer to a JOURNAL_DESCRIPTOR struct that will fit n-bytes. If no
		 * journal exists that fits n-bytes, it will create a journal that will fit
		 * at least n-bytes.
		 */
		void* 	_get_avail_journal(_SMEM_IN size_t);

		/**
		 * Creates a journal with n-pages. The specified flags describes the journal
		 * being created.
		 */
		void*	_create_journal(_SMEM_IN u32 pages, _SMEM_IN u32 flags);

		/**
		 * Collects information about the support for intrinsics.
		 */
		void	_get_intrinsic_support(_SMEM_VOID void);

		void* 	_journal_luptable_base;
		u32 	_journal_luptable_pages;
		u32 	_journal_luptable_count;
		u32 	_journal_minimum_pages;

		u32 	_alloc_alignment;


	protected:
		inline static b32 		_intrinsic_SSE2_128;
		inline static b32 		_intrinsic_AVX_256;
		inline static size_t 	_page_size = 0;

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
	// Determine intrinsic support.
	_get_intrinsic_support();

	// Determine default alignment requirements.
	u32 _default_alignment = 8;
	if (smemory::_intrinsic_SSE2_128) _default_alignment = 16;
	if (smemory::_intrinsic_AVX_256) _default_alignment = 32;

	// Automatically set the defaults on construction in case init is not called.
	this->_journal_minimum_pages = 1;
	this->_journal_luptable_pages = __SMEM_INTERNAL_DEFAULT_JLUPTBL_PAGES;
	this->_journal_luptable_count = 0;
	this->_alloc_alignment = 		_default_alignment;

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

void smemory::_get_intrinsic_support()
{

	/**
	 * We will need to determine intrinsic support for our memory_set operations.
	 * This is a Windows, MSVC-specific procedure, with the compiler intrinsic 
	 * __cpuidex(). I'm not entirely sure what other compilers on Windows require
	 * so we will only perform this procedure if the compiler is MSVC.
	 */
#if defined(_MSC_VER)
	int _cpuinfo[4] = {};
	__cpuidex(_cpuinfo, 0x80000000, 0);
	unsigned int ids = (unsigned int)_cpuinfo[0];
	if (ids >= 0x00000001)
	{
		__cpuidex(_cpuinfo, 0x00000001, 0);
		smemory::_intrinsic_SSE2_128 = 	(_cpuinfo[3] & ((int)1 << 26)) != 0;
		smemory::_intrinsic_AVX_256 = 	(_cpuinfo[2] & ((int)1 << 28)) != 0;
	}
#endif

}

void* smemory::_virtual_alloc(void* vaddress, u32 pages, size_t* alloc_size)
{
	*alloc_size = pages * _page_size;
	LPVOID _allocation_ptr = VirtualAlloc((LPVOID)vaddress, (SIZE_T)*alloc_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	return (void*)_allocation_ptr;
}

void smemory::_virtual_free(void* vaddress)
{
	BOOL _fstatus = VirtualFree(vaddress, NULL, MEM_RELEASE);
	return;
}

void* smemory::_create_journal(u32 pages, u32 flags)
{

	// Determine the number of pages to allocate.
	if (pages < this->_journal_minimum_pages) pages = this->_journal_minimum_pages;

	// Virtually allocate the journal.
	size_t _allocation_size = {};
	void* _allocation_ptr = _virtual_alloc(NULL, pages, &_allocation_size);

	// Initialize the journal descriptor.
	JOURNAL_DESCRIPTOR* _jdescriptor = (JOURNAL_DESCRIPTOR*)_allocation_ptr;
	_jdescriptor->commit = 	0;
	_jdescriptor->npages = 	pages;
	_jdescriptor->flags = 	flags;

	// Add it as an entry to the journal lookup table. What you see below is not for the faint of heart.
	*((void**)this->_journal_luptable_base + (this->_journal_luptable_count++)) = _allocation_ptr;

	return _allocation_ptr;

}

void* smemory::_get_avail_journal(size_t nbytes)
{

	// Search for a journal that will fit the required size.
	JOURNAL_DESCRIPTOR* _jdescriptor = nullptr;
	for (u32 i = 0; i < this->_journal_luptable_count; ++i)
	{
		// Retrieve the journal descriptor from the lookup table.
		void* _jptr = *((void**)this->_journal_luptable_base + i);
		JOURNAL_DESCRIPTOR* _currentjd = (JOURNAL_DESCRIPTOR*)_jptr;

		// Calculate the remaining free space within the journal.
		size_t _cjd_free = (_currentjd->npages * this->_page_size) -
			(_currentjd->allocation_offset + sizeof(JOURNAL_DESCRIPTOR));

		if ((nbytes < _cjd_free) && (_currentjd->flags & (u32)JOURNAL_DESC_FLAGS::SHARED))
		{
			_jdescriptor = _currentjd;
			break;
		}
	}

	// A journal does not exist that will accept the size required by the allocation.
	// The size of the journal is determined by the allocation divided by the page
	// size plus 1.
	if (_jdescriptor == nullptr)
	{
		u32 _required_pages = (u32)((nbytes / this->_page_size) + 1);
		_jdescriptor = (JOURNAL_DESCRIPTOR*)this->_create_journal(_required_pages,
			(u32)(JOURNAL_DESC_FLAGS::SHARED));
	}

	return _jdescriptor;
}

inline void smemory::memory_set_unaligned(void* set_addr, size_t size, u8 val)
{

	// Shift each 1-byte value into all 8 positions of the 64-bit value.
	u64 _set = 0; for (int i = 0; i < 8; ++i) _set |= ((u64)val << i*8);

	// 64-bit memory set.
	for (int i = 0; i < (size / 8); ++i)
	{
		*((u64*)set_addr + i) = _set;
	}

	// 8-bit memory set for allocations that aren't aligned at 8-byte boundaries.
	for (int i = 0; i < (size % 8); ++i)
	{
		*((u8*)set_addr + i + (size / 8)) = val;
	}

	return;
}

size_t smemory::page_size()
{
	// Even though we aren't using, we are required to fetch it to ensure that
	// the constructor is invoked at least once before returning a valid page size.
	__SMEM_INTERNAL_GET_INSTANCE();
	return smemory::_page_size;
}

void smemory::memory_set(void* set_addr, size_t size, u8 val)
{

	/**
	 * Note, this is not the most optimized memory set that can be achieved. To
	 * reduce time in memory, and for particularly large regions of memory, it may
	 * be wise to reduce branching and determine a particular memory set routine
	 * at initialization time. This may be the optimal approach as the library matures.
	 */

	/**
	 * If we do not have intrinsic support or the allocation we are setting is small,
	 * we can use a 64-bit, unaligned procedure as it will suffice to perform the
	 * required operation.
	 */
	if (!smemory::_intrinsic_SSE2_128 || !smemory::_intrinsic_AVX_256 || size < 32)
	{
		memory_set_unaligned(set_addr, size, val);
		return;
	}

	/**
	 * For SSE/AVX level memory_set, which can blast bits out to memory much faster
	 * than its unaligned counterpart, we can utilize AVX for 256-bit per-instruction
	 * and SSE2 for 126-bit per-instruction.
	 */

	// 256-bit level setting.
	if (smemory::_intrinsic_AVX_256)
	{
		// Ensure boundary alignment. If we do hit unalignment, it is because smemory
		// was improperly configured or the user is using the memory_set on a region
		// of memory they are managing themselves. In either case, we should align it.
		u64 _unal = (u64)set_addr % 32;
		if (_unal)	memory_set_unaligned(set_addr, _unal, val); 
		set_addr = (u8*)set_addr + _unal;
		size -= _unal;

		// 256-bit set memory set procedure.
		__m256i _set = _mm256_set1_epi8(val);
		for (int i = 0; i < (size / 32); ++i)
		{
			_mm256_store_si256((__m256i*)set_addr+i, _set);
		}

		// We will need to set the rest.
		set_addr = (u8*)set_addr + (size - (size % 32));
		size = (size % 32);
		memory_set_unaligned(set_addr, size, val);
		
	}

	// 128-bit level setting for when AVX is not available.
	else
	{

		// Ensure boundary alignment. If we do hit unalignment, it is because smemory
		// was improperly configured or the user is using the memory_set on a region
		// of memory they are managing themselves. In either case, we should align it.
		u64 _unal = (u64)set_addr % 16;
		if (_unal)	memory_set_unaligned(set_addr, _unal, val); 
		set_addr = (u8*)set_addr + _unal;
		size -= _unal;

		// 256-bit set memory set procedure.
		__m128i _set = _mm_set1_epi8(val);
		for (int i = 0; i < (size / 16); ++i)
		{
			_mm_store_si128((__m128i*)set_addr+i, _set);
		}

		// We will need to set the rest.
		set_addr = (u8*)set_addr + (size - (size % 16));
		size = (size % 16);
		memory_set_unaligned(set_addr, size, val);

	}

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

void smemory::init(SMEMORY_CONFIG* config)
{

	
	// Fill out the configuration provided by the user.
	__SMEM_INTERNAL_GET_INSTANCE();
	__SMEM_CONFIG_ZERO_CHECKSET(config, journal_luptbl_pages, _smem._journal_luptable_pages);
	__SMEM_CONFIG_ZERO_CHECKSET(config, journal_min_pages, _smem._journal_minimum_pages);
	__SMEM_CONFIG_ZERO_CHECKSET(config, journal_create_journal, 0);
	__SMEM_CONFIG_ZERO_CHECKSET(config, alloc_alignment, _smem._alloc_alignment);

	// Set smemory member properties.
	_smem._journal_luptable_pages = config->journal_luptbl_pages;
	_smem._journal_minimum_pages = 	config->journal_min_pages;
	_smem._alloc_alignment = 		config->alloc_alignment;

	// Generate the journal lookup table.
	size_t _jluptable_alloc_size = {};
	_smem._journal_luptable_base = _smem._virtual_alloc((void*)__SMEM_INTERNAL_DEFAULT_LUPTABLE_VADDR,
		_smem._journal_luptable_pages, &_jluptable_alloc_size);

	// Creates a journal at on initialization time if specified.
	if (config->journal_create_journal)
	{
		_smem._create_journal(config->journal_create_journal, (u32)(JOURNAL_DESC_FLAGS::SHARED));
	}

	return;
}

void* smemory::alloc(size_t nbytes)
{

	__SMEM_INTERNAL_GET_INSTANCE();
	size_t _alloc_desc_size = sizeof(ALLOC_DESCRIPTOR);
	size_t _alloc_req = nbytes + _alloc_desc_size;
	size_t _alloc_alignment_pad = _smem._alloc_alignment - (_alloc_req % _smem._alloc_alignment);
	size_t _alloc_size = _alloc_req + _alloc_alignment_pad;

	// Retrieve a journal to fit the requested allocation.
	JOURNAL_DESCRIPTOR* _jdescriptor = (JOURNAL_DESCRIPTOR*)_smem._get_avail_journal(nbytes);

	// Get the base location of the journal heap and then calculate where 
	// the allocation should go.
	void* _jdesc_base = (void*)((u8*)_jdescriptor + sizeof(JOURNAL_DESCRIPTOR));
	void* _alloc = (void*)((u8*)_jdesc_base + _jdescriptor->allocation_offset); 
	_jdescriptor->allocation_offset += (u32)_alloc_size;
	_jdescriptor->commit += (u32)_alloc_size;

	// Set the ALLOC_DESCRIPTOR details.
	ALLOC_DESCRIPTOR* _adescriptor = (ALLOC_DESCRIPTOR*)_alloc;
	_adescriptor->commit = (u64)_alloc_size;
	_adescriptor->journal_offset = ((u64)_alloc - (u64)_jdescriptor);

	// Get the base location of the allocated region the user assigns to.
	void* _alloc_ptr = (void*)((u8*)_alloc + sizeof(ALLOC_DESCRIPTOR));
	return _alloc_ptr;

}

void smemory::free(void* addr)
{

	// Backstep to retrieve the allocation descriptor.
	void* _pptr = (void*)((u8*)addr - sizeof(ALLOC_DESCRIPTOR));
	ALLOC_DESCRIPTOR* _adescriptor = (ALLOC_DESCRIPTOR*)_pptr;
	if (_adescriptor->commit == 0) return; // No need to deallocate, already considered deallocated.

	// Reduce the commit of on the journal descriptor.
	JOURNAL_DESCRIPTOR* _jdescriptor = (JOURNAL_DESCRIPTOR*)((u8*)_pptr - _adescriptor->journal_offset);
	_jdescriptor->commit -= _adescriptor->commit;

#if __SMEM_CLEAR_ON_FREE == 1
	// Clear out the bits.
	memory_set(_pptr, _adescriptor->commit, 0x00);
#endif

	// Set the commit to zero to prevent multiple decommits to the journal.
	_adescriptor->commit = 0;
	return; 

}

void smemory::reclaim()
{

	__SMEM_INTERNAL_GET_INSTANCE();
	for (u32 i = 0; i < _smem._journal_luptable_count; ++i)
	{
		// Grab the journal descriptor pointer for the lookup table.
		void* _jptr = *((void**)_smem._journal_luptable_base + i);
		JOURNAL_DESCRIPTOR* _jdescriptor = (JOURNAL_DESCRIPTOR*)_jptr;
		
		// Process the requirements to reclaim.
		b32 _reclaim = false;

		// Reclaim any journals marked as force-reclaim.
		if (_jdescriptor->flags & (u32)JOURNAL_DESC_FLAGS::FORCERECLAIM) _reclaim = true;

		// Reclaim any journals with a zero-commit and isn't set as no-reclaim.
		if (!(_jdescriptor->flags & (u32)JOURNAL_DESC_FLAGS::NORECLAIM)
			&& _jdescriptor->commit == 0) _reclaim = true;

		if (_reclaim != false)
		{
			// Release the pages back to the operating system.
			_virtual_free(_jptr);

			// Remove from the lookup table.
			*((void**)_smem._journal_luptable_base + i) = nullptr; 
			
			// Swap with the tail as needed to prevent holes in the lookup table.
			if (i != _smem._journal_luptable_count-1)
			{
				void* _tail = *((void**)_smem._journal_luptable_base + (_smem._journal_luptable_count - 1));
				*((void**)_smem._journal_luptable_base + i) = _tail;
				*((void**)_smem._journal_luptable_base + (_smem._journal_luptable_count - 1)) = nullptr;
			}

			_smem._journal_luptable_count--;

		}

	}


}

#endif

#endif