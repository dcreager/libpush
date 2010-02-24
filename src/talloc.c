/* 
   Samba Unix SMB/CIFS implementation.

   Samba trivial allocation library - new interface

   NOTE: Please read talloc_guide.txt for full documentation

   Copyright (C) Andrew Tridgell 2004
   Copyright (C) Stefan Metzmacher 2006
   
     ** NOTE! The following LGPL license applies to the talloc
     ** library. This does NOT imply that all of Samba is released
     ** under the LGPL
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
  inspired by http://swapped.cc/halloc/
*/

#include "push/talloc.h"
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

/* use this to force every realloc to change the pointer, to stress test
   code that might not cope */
#define ALWAYS_REALLOC 0


#define MAX_PUSH_TALLOC_SIZE 0x7FFFFFFF
#define PUSH_TALLOC_MAGIC 0xe814ec70
#define PUSH_TALLOC_FLAG_FREE 0x01
#define PUSH_TALLOC_FLAG_LOOP 0x02
#define PUSH_TALLOC_FLAG_EXT_ALLOC 0x04
#define PUSH_TALLOC_MAGIC_REFERENCE ((const char *)1)

/* by default we abort when given a bad pointer (such as when push_talloc_free() is called 
   on a pointer that came from malloc() */
#ifndef PUSH_TALLOC_ABORT
#define PUSH_TALLOC_ABORT(reason) abort()
#endif

#ifndef discard_const_p
#if defined(INTPTR_MIN)
# define discard_const_p(type, ptr) ((type *)((intptr_t)(ptr)))
#else
# define discard_const_p(type, ptr) ((type *)(ptr))
#endif
#endif

/* these macros gain us a few percent of speed on gcc */
#if HAVE_BUILTIN_EXPECT
/* the strange !! is to ensure that __builtin_expect() takes either 0 or 1
   as its first argument */
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) x
#define unlikely(x) x
#endif

/* this null_context is only used if push_talloc_enable_leak_report() or
   push_talloc_enable_leak_report_full() is called, otherwise it remains
   NULL
*/
static void *null_context;
static pid_t *autofree_context;

static void *(*tc_external_realloc)(const void *parent, void *ptr, size_t size);
static void (*tc_lock)(const void *ctx);
static void (*tc_unlock)(void);

struct push_talloc_reference_handle {
	struct push_talloc_reference_handle *next, *prev;
	void *ptr;
};

typedef int (*push_talloc_destructor_t)(void *);

struct push_talloc_chunk {
	struct push_talloc_chunk *next, *prev;
	struct push_talloc_chunk *parent, *child;
	struct push_talloc_reference_handle *refs;
	push_talloc_destructor_t destructor;
	const char *name;
	size_t size;
	unsigned flags;
};

/* 16 byte alignment seems to keep everyone happy */
#define TC_HDR_SIZE ((sizeof(struct push_talloc_chunk)+15)&~15)
#define TC_PTR_FROM_CHUNK(tc) ((void *)(TC_HDR_SIZE + (char*)tc))

/* panic if we get a bad magic value */
static inline struct push_talloc_chunk *push_talloc_chunk_from_ptr(const void *ptr)
{
	const char *pp = (const char *)ptr;
	struct push_talloc_chunk *tc = discard_const_p(struct push_talloc_chunk, pp - TC_HDR_SIZE);
	if (unlikely((tc->flags & (PUSH_TALLOC_FLAG_FREE | ~0xF)) != PUSH_TALLOC_MAGIC)) { 
		if (tc->flags & PUSH_TALLOC_FLAG_FREE) {
			PUSH_TALLOC_ABORT("Bad push_talloc magic value - double free"); 
		} else {
			PUSH_TALLOC_ABORT("Bad push_talloc magic value - unknown value"); 
		}
	}
	return tc;
}

/* hook into the front of the list */
#define _TLIST_ADD(list, p) \
do { \
        if (!(list)) { \
		(list) = (p); \
		(p)->next = (p)->prev = NULL; \
	} else { \
		(list)->prev = (p); \
		(p)->next = (list); \
		(p)->prev = NULL; \
		(list) = (p); \
	}\
} while (0)

/* remove an element from a list - element doesn't have to be in list. */
#define _TLIST_REMOVE(list, p) \
do { \
	if ((p) == (list)) { \
		(list) = (p)->next; \
		if (list) (list)->prev = NULL; \
	} else { \
		if ((p)->prev) (p)->prev->next = (p)->next; \
		if ((p)->next) (p)->next->prev = (p)->prev; \
	} \
	if ((p) && ((p) != (list))) (p)->next = (p)->prev = NULL; \
} while (0)

static int locked;
static inline void lock(const void *p)
{
	if (tc_lock && p) {
		struct push_talloc_chunk *tc = push_talloc_chunk_from_ptr(p);

		if (tc->flags & PUSH_TALLOC_FLAG_EXT_ALLOC) {
			if (locked)
				PUSH_TALLOC_ABORT("nested locking");
			tc_lock(tc);
			locked = 1;
		}
	}
}

static inline void unlock(void)
{
	if (locked) {
		tc_unlock();
		locked = 0;
	}
}

/*
  return the parent chunk of a pointer
*/
static inline struct push_talloc_chunk *push_talloc_parent_chunk(const void *ptr)
{
	struct push_talloc_chunk *tc;

	if (unlikely(ptr == NULL)) {
		return NULL;
	}

	tc = push_talloc_chunk_from_ptr(ptr);
	while (tc->prev) tc=tc->prev;

	return tc->parent;
}

/* This version doesn't do locking, so you must already have it. */
static void *push_talloc_parent_nolock(const void *ptr)
{
	struct push_talloc_chunk *tc;

	tc = push_talloc_parent_chunk(ptr);
	return tc ? TC_PTR_FROM_CHUNK(tc) : NULL;
}

void *push_talloc_parent(const void *ptr)
{
	void *parent;

	lock(ptr);
	parent = push_talloc_parent_nolock(ptr);
	unlock();
	return parent;
}

/*
  find parents name
*/
const char *push_talloc_parent_name(const void *ptr)
{
	struct push_talloc_chunk *tc;

	lock(ptr);
	tc = push_talloc_parent_chunk(ptr);
	unlock();

	return tc? tc->name : NULL;
}

static void *init_push_talloc(struct push_talloc_chunk *parent,
			 struct push_talloc_chunk *tc,
			 size_t size, int external)
{
	if (unlikely(tc == NULL))
		return NULL;

	tc->size = size;
	tc->flags = PUSH_TALLOC_MAGIC;
	if (external)
		tc->flags |= PUSH_TALLOC_FLAG_EXT_ALLOC;
	tc->destructor = NULL;
	tc->child = NULL;
	tc->name = NULL;
	tc->refs = NULL;

	if (likely(parent)) {
		if (parent->child) {
			parent->child->parent = NULL;
			tc->next = parent->child;
			tc->next->prev = tc;
		} else {
			tc->next = NULL;
		}
		tc->parent = parent;
		tc->prev = NULL;
		parent->child = tc;
	} else {
		tc->next = tc->prev = tc->parent = NULL;
	}

	return TC_PTR_FROM_CHUNK(tc);
}

/* 
   Allocate a bit of memory as a child of an existing pointer
*/
static inline void *__push_talloc(const void *context, size_t size)
{
	struct push_talloc_chunk *tc;
	struct push_talloc_chunk *parent = NULL;
	int external = 0;

	if (unlikely(context == NULL)) {
		context = null_context;
	}

	if (unlikely(size >= MAX_PUSH_TALLOC_SIZE)) {
		return NULL;
	}

	if (likely(context)) {
		parent = push_talloc_chunk_from_ptr(context);
		if (unlikely(parent->flags & PUSH_TALLOC_FLAG_EXT_ALLOC)) {
			tc = tc_external_realloc(context, NULL,
						 TC_HDR_SIZE+size);
			external = 1;
			goto alloc_done;
		}
	}

	tc = (struct push_talloc_chunk *)malloc(TC_HDR_SIZE+size);
alloc_done:
	return init_push_talloc(parent, tc, size, external);
}

/*
  setup a destructor to be called on free of a pointer
  the destructor should return 0 on success, or -1 on failure.
  if the destructor fails then the free is failed, and the memory can
  be continued to be used
*/
void _push_talloc_set_destructor(const void *ptr, int (*destructor)(void *))
{
	struct push_talloc_chunk *tc = push_talloc_chunk_from_ptr(ptr);
	tc->destructor = destructor;
}

/*
  increase the reference count on a piece of memory. 
*/
int push_talloc_increase_ref_count(const void *ptr)
{
	if (unlikely(!push_talloc_reference(null_context, ptr))) {
		return -1;
	}
	return 0;
}

/*
  helper for push_talloc_reference()

  this is referenced by a function pointer and should not be inline
*/
static int push_talloc_reference_destructor(struct push_talloc_reference_handle *handle)
{
	struct push_talloc_chunk *ptr_tc = push_talloc_chunk_from_ptr(handle->ptr);
	_TLIST_REMOVE(ptr_tc->refs, handle);
	return 0;
}

/*
   more efficient way to add a name to a pointer - the name must point to a 
   true string constant
*/
static inline void _push_talloc_set_name_const(const void *ptr, const char *name)
{
	struct push_talloc_chunk *tc = push_talloc_chunk_from_ptr(ptr);
	tc->name = name;
}

/*
  internal push_talloc_named_const()
*/
static inline void *_push_talloc_named_const(const void *context, size_t size, const char *name)
{
	void *ptr;

	ptr = __push_talloc(context, size);
	if (unlikely(ptr == NULL)) {
		return NULL;
	}

	_push_talloc_set_name_const(ptr, name);

	return ptr;
}

/*
  make a secondary reference to a pointer, hanging off the given context.
  the pointer remains valid until both the original caller and this given
  context are freed.
  
  the major use for this is when two different structures need to reference the 
  same underlying data, and you want to be able to free the two instances separately,
  and in either order
*/
void *_push_talloc_reference(const void *context, const void *ptr)
{
	struct push_talloc_chunk *tc;
	struct push_talloc_reference_handle *handle;
	if (unlikely(ptr == NULL)) return NULL;

	lock(context);
	tc = push_talloc_chunk_from_ptr(ptr);
	handle = (struct push_talloc_reference_handle *)_push_talloc_named_const(context,
						   sizeof(struct push_talloc_reference_handle),
						   PUSH_TALLOC_MAGIC_REFERENCE);
	if (unlikely(handle == NULL)) {
		unlock();
		return NULL;
	}

	/* note that we hang the destructor off the handle, not the
	   main context as that allows the caller to still setup their
	   own destructor on the context if they want to */
	push_talloc_set_destructor(handle, push_talloc_reference_destructor);
	handle->ptr = discard_const_p(void, ptr);
	_TLIST_ADD(tc->refs, handle);
	unlock();
	return handle->ptr;
}

/*
  return 1 if ptr is a parent of context
*/
static int _push_talloc_is_parent(const void *context, const void *ptr)
{
	struct push_talloc_chunk *tc;

	if (context == NULL) {
		return 0;
	}

	tc = push_talloc_chunk_from_ptr(context);
	while (tc) {
		if (TC_PTR_FROM_CHUNK(tc) == ptr) {
			return 1;
		}
		while (tc && tc->prev) tc = tc->prev;
		if (tc) {
			tc = tc->parent;
		}
	}
	return 0;
}

/* 
   move a lump of memory from one push_talloc context to another return the
   ptr on success, or NULL if it could not be transferred.
   passing NULL as ptr will always return NULL with no side effects.
*/
static void *__push_talloc_steal(const void *new_ctx, const void *ptr)
{
	struct push_talloc_chunk *tc, *new_tc;

	if (unlikely(!ptr)) {
		return NULL;
	}

	if (unlikely(new_ctx == NULL)) {
		new_ctx = null_context;
	}

	tc = push_talloc_chunk_from_ptr(ptr);

	if (unlikely(new_ctx == NULL)) {
		if (tc->parent) {
			_TLIST_REMOVE(tc->parent->child, tc);
			if (tc->parent->child) {
				tc->parent->child->parent = tc->parent;
			}
		} else {
			if (tc->prev) tc->prev->next = tc->next;
			if (tc->next) tc->next->prev = tc->prev;
		}
		
		tc->parent = tc->next = tc->prev = NULL;
		return discard_const_p(void, ptr);
	}

	new_tc = push_talloc_chunk_from_ptr(new_ctx);

	if (unlikely(tc == new_tc || tc->parent == new_tc)) {
		return discard_const_p(void, ptr);
	}

	if (tc->parent) {
		_TLIST_REMOVE(tc->parent->child, tc);
		if (tc->parent->child) {
			tc->parent->child->parent = tc->parent;
		}
	} else {
		if (tc->prev) tc->prev->next = tc->next;
		if (tc->next) tc->next->prev = tc->prev;
	}

	tc->parent = new_tc;
	if (new_tc->child) new_tc->child->parent = NULL;
	_TLIST_ADD(new_tc->child, tc);

	return discard_const_p(void, ptr);
}

/* 
   internal push_talloc_free call
*/
static inline int _push_talloc_free(const void *ptr)
{
	struct push_talloc_chunk *tc;
	void *oldparent = NULL;

	if (unlikely(ptr == NULL)) {
		return -1;
	}

	tc = push_talloc_chunk_from_ptr(ptr);

	if (unlikely(tc->refs)) {
		int is_child;
		/* check this is a reference from a child or grantchild
		 * back to it's parent or grantparent
		 *
		 * in that case we need to remove the reference and
		 * call another instance of push_talloc_free() on the current
		 * pointer.
		 */
		is_child = _push_talloc_is_parent(tc->refs, ptr);
		_push_talloc_free(tc->refs);
		if (is_child) {
			return _push_talloc_free(ptr);
		}
		return -1;
	}

	if (unlikely(tc->flags & PUSH_TALLOC_FLAG_LOOP)) {
		/* we have a free loop - stop looping */
		return 0;
	}

	if (unlikely(tc->destructor)) {
		push_talloc_destructor_t d = tc->destructor;
		if (d == (push_talloc_destructor_t)-1) {
			return -1;
		}
		tc->destructor = (push_talloc_destructor_t)-1;
		if (d(discard_const_p(void, ptr)) == -1) {
			tc->destructor = d;
			return -1;
		}
		tc->destructor = NULL;
	}

	if (unlikely(tc->flags & PUSH_TALLOC_FLAG_EXT_ALLOC))
		oldparent = push_talloc_parent_nolock(ptr);

	if (tc->parent) {
		_TLIST_REMOVE(tc->parent->child, tc);
		if (tc->parent->child) {
			tc->parent->child->parent = tc->parent;
		}
	} else {
		if (tc->prev) tc->prev->next = tc->next;
		if (tc->next) tc->next->prev = tc->prev;
	}

	tc->flags |= PUSH_TALLOC_FLAG_LOOP;

	while (tc->child) {
		/* we need to work out who will own an abandoned child
		   if it cannot be freed. In priority order, the first
		   choice is owner of any remaining reference to this
		   pointer, the second choice is our parent, and the
		   final choice is the null context. */
		void *child = TC_PTR_FROM_CHUNK(tc->child);
		const void *new_parent = null_context;
		if (unlikely(tc->child->refs)) {
			struct push_talloc_chunk *p = push_talloc_parent_chunk(tc->child->refs);
			if (p) new_parent = TC_PTR_FROM_CHUNK(p);
		}
		if (unlikely(_push_talloc_free(child) == -1)) {
			if (new_parent == null_context) {
				struct push_talloc_chunk *p = push_talloc_parent_chunk(ptr);
				if (p) new_parent = TC_PTR_FROM_CHUNK(p);
			}
			__push_talloc_steal(new_parent, child);
		}
	}

	tc->flags |= PUSH_TALLOC_FLAG_FREE;

	if (unlikely(tc->flags & PUSH_TALLOC_FLAG_EXT_ALLOC))
		tc_external_realloc(oldparent, tc, 0);
	else
		free(tc);

	return 0;
}

void *_push_talloc_steal(const void *new_ctx, const void *ptr)
{
	void *p;

	lock(new_ctx);
	p = __push_talloc_steal(new_ctx, ptr);
	unlock();
	return p;
}

/*
  remove a secondary reference to a pointer. This undo's what
  push_talloc_reference() has done. The context and pointer arguments
  must match those given to a push_talloc_reference()
*/
static inline int push_talloc_unreference(const void *context, const void *ptr)
{
	struct push_talloc_chunk *tc = push_talloc_chunk_from_ptr(ptr);
	struct push_talloc_reference_handle *h;

	if (unlikely(context == NULL)) {
		context = null_context;
	}

	for (h=tc->refs;h;h=h->next) {
		struct push_talloc_chunk *p = push_talloc_parent_chunk(h);
		if (p == NULL) {
			if (context == NULL) break;
		} else if (TC_PTR_FROM_CHUNK(p) == context) {
			break;
		}
	}
	if (h == NULL) {
		return -1;
	}

	return _push_talloc_free(h);
}

/*
  remove a specific parent context from a pointer. This is a more
  controlled varient of push_talloc_free()
*/
int push_talloc_unlink(const void *context, void *ptr)
{
	struct push_talloc_chunk *tc_p, *new_p;
	void *new_parent;

	if (ptr == NULL) {
		return -1;
	}

	if (context == NULL) {
		context = null_context;
	}

	lock(context);
	if (push_talloc_unreference(context, ptr) == 0) {
		unlock();
		return 0;
	}

	if (context == NULL) {
		if (push_talloc_parent_chunk(ptr) != NULL) {
			unlock();
			return -1;
		}
	} else {
		if (push_talloc_chunk_from_ptr(context) != push_talloc_parent_chunk(ptr)) {
			unlock();
			return -1;
		}
	}
	
	tc_p = push_talloc_chunk_from_ptr(ptr);

	if (tc_p->refs == NULL) {
		int ret = _push_talloc_free(ptr);
		unlock();
		return ret;
	}

	new_p = push_talloc_parent_chunk(tc_p->refs);
	if (new_p) {
		new_parent = TC_PTR_FROM_CHUNK(new_p);
	} else {
		new_parent = NULL;
	}

	if (push_talloc_unreference(new_parent, ptr) != 0) {
		unlock();
		return -1;
	}

	__push_talloc_steal(new_parent, ptr);
	unlock();

	return 0;
}

/*
  add a name to an existing pointer - va_list version
*/
static inline const char *push_talloc_set_name_v(const void *ptr, const char *fmt, va_list ap) PRINTF_ATTRIBUTE(2,0);

static inline const char *push_talloc_set_name_v(const void *ptr, const char *fmt, va_list ap)
{
	struct push_talloc_chunk *tc = push_talloc_chunk_from_ptr(ptr);
	tc->name = push_talloc_vasprintf(ptr, fmt, ap);
	if (likely(tc->name)) {
		_push_talloc_set_name_const(tc->name, ".name");
	}
	return tc->name;
}

/*
  add a name to an existing pointer
*/
const char *push_talloc_set_name(const void *ptr, const char *fmt, ...)
{
	const char *name;
	va_list ap;
	va_start(ap, fmt);
	name = push_talloc_set_name_v(ptr, fmt, ap);
	va_end(ap);
	return name;
}


/*
  create a named push_talloc pointer. Any push_talloc pointer can be named, and
  push_talloc_named() operates just like push_talloc() except that it allows you
  to name the pointer.
*/
void *push_talloc_named(const void *context, size_t size, const char *fmt, ...)
{
	va_list ap;
	void *ptr;
	const char *name;

	lock(context);
	ptr = __push_talloc(context, size);
	unlock();
	if (unlikely(ptr == NULL)) return NULL;

	va_start(ap, fmt);
	name = push_talloc_set_name_v(ptr, fmt, ap);
	va_end(ap);

	if (unlikely(name == NULL)) {
		push_talloc_free(ptr);
		return NULL;
	}

	return ptr;
}

/*
  return the name of a push_talloc ptr, or "UNNAMED"
*/
const char *push_talloc_get_name(const void *ptr)
{
	struct push_talloc_chunk *tc = push_talloc_chunk_from_ptr(ptr);
	if (unlikely(tc->name == PUSH_TALLOC_MAGIC_REFERENCE)) {
		return ".reference";
	}
	if (likely(tc->name)) {
		return tc->name;
	}
	return "UNNAMED";
}


/*
  check if a pointer has the given name. If it does, return the pointer,
  otherwise return NULL
*/
void *push_talloc_check_name(const void *ptr, const char *name)
{
	const char *pname;
	if (unlikely(ptr == NULL)) return NULL;
	pname = push_talloc_get_name(ptr);
	if (likely(pname == name || strcmp(pname, name) == 0)) {
		return discard_const_p(void, ptr);
	}
	return NULL;
}


/*
  this is for compatibility with older versions of push_talloc
*/
void *push_talloc_init(const char *fmt, ...)
{
	va_list ap;
	void *ptr;
	const char *name;

	/*
	 * samba3 expects push_talloc_report_depth_cb(NULL, ...)
	 * reports all push_talloc'ed memory, so we need to enable
	 * null_tracking
	 */
	push_talloc_enable_null_tracking();

	ptr = __push_talloc(NULL, 0);
	if (unlikely(ptr == NULL)) return NULL;

	va_start(ap, fmt);
	name = push_talloc_set_name_v(ptr, fmt, ap);
	va_end(ap);

	if (unlikely(name == NULL)) {
		push_talloc_free(ptr);
		return NULL;
	}

	return ptr;
}

/* 
   Allocate a bit of memory as a child of an existing pointer
*/
void *_push_talloc(const void *context, size_t size)
{
	return __push_talloc(context, size);
}

static int push_talloc_destroy_pointer(void ***pptr)
{
	if ((uintptr_t)**pptr < getpagesize())
		PUSH_TALLOC_ABORT("Double free or invalid push_talloc_set?");
	/* Invalidate pointer so it can't be used again. */
	**pptr = (void *)1;
	return 0;
}

void _push_talloc_set(void *ptr, const void *ctx, size_t size, const char *name)
{
	void ***child;
	void **pptr = ptr;

	*pptr = push_talloc_named_const(ctx, size, name);
	if (unlikely(!*pptr))
		return;

	child = push_talloc(*pptr, void **);
	if (unlikely(!child)) {
		push_talloc_free(*pptr);
		*pptr = NULL;
		return;
	}
	*child = pptr;
	push_talloc_set_name_const(child, "push_talloc_set destructor");
	push_talloc_set_destructor(child, push_talloc_destroy_pointer);
}

/*
  externally callable push_talloc_set_name_const()
*/
void push_talloc_set_name_const(const void *ptr, const char *name)
{
	_push_talloc_set_name_const(ptr, name);
}

/*
  create a named push_talloc pointer. Any push_talloc pointer can be named, and
  push_talloc_named() operates just like push_talloc() except that it allows you
  to name the pointer.
*/
void *push_talloc_named_const(const void *context, size_t size, const char *name)
{
	void *p;
	lock(context);
	p = _push_talloc_named_const(context, size, name);
	unlock();
	return p;
}

/* 
   free a push_talloc pointer. This also frees all child pointers of this 
   pointer recursively

   return 0 if the memory is actually freed, otherwise -1. The memory
   will not be freed if the ref_count is > 1 or the destructor (if
   any) returns non-zero
*/
int push_talloc_free(const void *ptr)
{
	int saved_errno = errno, ret;

	lock(ptr);
	ret = _push_talloc_free(discard_const_p(void, ptr));
	unlock();
	if (ret == 0)
		errno = saved_errno;
	return ret;
}



/*
  A push_talloc version of realloc. The context argument is only used if
  ptr is NULL
*/
void *_push_talloc_realloc(const void *context, void *ptr, size_t size, const char *name)
{
	struct push_talloc_chunk *tc;
	void *new_ptr;

	/* size zero is equivalent to free() */
	if (unlikely(size == 0)) {
		push_talloc_free(ptr);
		return NULL;
	}

	if (unlikely(size >= MAX_PUSH_TALLOC_SIZE)) {
		return NULL;
	}

	/* realloc(NULL) is equivalent to malloc() */
	if (ptr == NULL) {
		return push_talloc_named_const(context, size, name);
	}

	tc = push_talloc_chunk_from_ptr(ptr);

	/* don't allow realloc on referenced pointers */
	if (unlikely(tc->refs)) {
		return NULL;
	}

	lock(ptr);
	if (unlikely(tc->flags & PUSH_TALLOC_FLAG_EXT_ALLOC)) {
		/* need to get parent before setting free flag. */
		void *parent = push_talloc_parent_nolock(ptr);
		tc->flags |= PUSH_TALLOC_FLAG_FREE;
		new_ptr = tc_external_realloc(parent, tc, size + TC_HDR_SIZE);
	} else {
		/* by resetting magic we catch users of the old memory */
		tc->flags |= PUSH_TALLOC_FLAG_FREE;

#if ALWAYS_REALLOC
		new_ptr = malloc(size + TC_HDR_SIZE);
		if (new_ptr) {
			memcpy(new_ptr, tc, tc->size + TC_HDR_SIZE);
			free(tc);
		}
#else
		new_ptr = realloc(tc, size + TC_HDR_SIZE);
#endif
	}

	if (unlikely(!new_ptr)) {	
		tc->flags &= ~PUSH_TALLOC_FLAG_FREE; 
		unlock();
		return NULL; 
	}

	tc = (struct push_talloc_chunk *)new_ptr;
	tc->flags &= ~PUSH_TALLOC_FLAG_FREE; 
	if (tc->parent) {
		tc->parent->child = tc;
	}
	if (tc->child) {
		tc->child->parent = tc;
	}

	if (tc->prev) {
		tc->prev->next = tc;
	}
	if (tc->next) {
		tc->next->prev = tc;
	}

	tc->size = size;
	_push_talloc_set_name_const(TC_PTR_FROM_CHUNK(tc), name);
	unlock();

	return TC_PTR_FROM_CHUNK(tc);
}

/*
  a wrapper around push_talloc_steal() for situations where you are moving a pointer
  between two structures, and want the old pointer to be set to NULL
*/
void *_push_talloc_move(const void *new_ctx, const void *_pptr)
{
	const void **pptr = discard_const_p(const void *,_pptr);
	void *ret = _push_talloc_steal(new_ctx, *pptr);
	(*pptr) = NULL;
	return ret;
}

/*
  return the total size of a push_talloc pool (subtree)
*/
static size_t _push_talloc_total_size(const void *ptr)
{
	size_t total = 0;
	struct push_talloc_chunk *c, *tc;

	tc = push_talloc_chunk_from_ptr(ptr);

	if (tc->flags & PUSH_TALLOC_FLAG_LOOP) {
		return 0;
	}

	tc->flags |= PUSH_TALLOC_FLAG_LOOP;

	total = tc->size;
	for (c=tc->child;c;c=c->next) {
		total += _push_talloc_total_size(TC_PTR_FROM_CHUNK(c));
	}

	tc->flags &= ~PUSH_TALLOC_FLAG_LOOP;

	return total;
}

size_t push_talloc_total_size(const void *ptr)
{
	size_t total;

	if (ptr == NULL) {
		ptr = null_context;
	}
	if (ptr == NULL) {
		return 0;
	}

	lock(ptr);
	total = _push_talloc_total_size(ptr);
	unlock();
	return total;
}	

/*
  return the total number of blocks in a push_talloc pool (subtree)
*/
static size_t _push_talloc_total_blocks(const void *ptr)
{
	size_t total = 0;
	struct push_talloc_chunk *c, *tc = push_talloc_chunk_from_ptr(ptr);

	if (tc->flags & PUSH_TALLOC_FLAG_LOOP) {
		return 0;
	}

	tc->flags |= PUSH_TALLOC_FLAG_LOOP;

	total++;
	for (c=tc->child;c;c=c->next) {
		total += _push_talloc_total_blocks(TC_PTR_FROM_CHUNK(c));
	}

	tc->flags &= ~PUSH_TALLOC_FLAG_LOOP;

	return total;
}

size_t push_talloc_total_blocks(const void *ptr)
{
	size_t total;

	lock(ptr);
	total = _push_talloc_total_blocks(ptr);
	unlock();

	return total;
}

static size_t _push_talloc_reference_count(const void *ptr)
{
	struct push_talloc_chunk *tc = push_talloc_chunk_from_ptr(ptr);
	struct push_talloc_reference_handle *h;
	size_t ret = 0;

	for (h=tc->refs;h;h=h->next) {
		ret++;
	}
	return ret;
}

/*
  return the number of external references to a pointer
*/
size_t push_talloc_reference_count(const void *ptr)
{
	size_t ret;

	lock(push_talloc_chunk_from_ptr(ptr));
	ret = _push_talloc_reference_count(ptr);
	unlock();
	return ret;
}

/*
  report on memory usage by all children of a pointer, giving a full tree view
*/
static void _push_talloc_report_depth_cb(const void *ptr, int depth, int max_depth,
			    void (*callback)(const void *ptr,
			  		     int depth, int max_depth,
					     int is_ref,
					     void *private_data),
			    void *private_data)
{
	struct push_talloc_chunk *c, *tc;

	tc = push_talloc_chunk_from_ptr(ptr);

	if (tc->flags & PUSH_TALLOC_FLAG_LOOP) {
		return;
	}

	callback(ptr, depth, max_depth, 0, private_data);

	if (max_depth >= 0 && depth >= max_depth) {
		return;
	}

	tc->flags |= PUSH_TALLOC_FLAG_LOOP;
	for (c=tc->child;c;c=c->next) {
		if (c->name == PUSH_TALLOC_MAGIC_REFERENCE) {
			struct push_talloc_reference_handle *h = (struct push_talloc_reference_handle *)TC_PTR_FROM_CHUNK(c);
			callback(h->ptr, depth + 1, max_depth, 1, private_data);
		} else {
			_push_talloc_report_depth_cb(TC_PTR_FROM_CHUNK(c), depth + 1, max_depth, callback, private_data);
		}
	}
	tc->flags &= ~PUSH_TALLOC_FLAG_LOOP;
}

void push_talloc_report_depth_cb(const void *ptr, int depth, int max_depth,
			    void (*callback)(const void *ptr,
			  		     int depth, int max_depth,
					     int is_ref,
					     void *private_data),
			    void *private_data)
{
	if (ptr == NULL) {
		ptr = null_context;
	}
	if (ptr == NULL) return;

	lock(ptr);
	_push_talloc_report_depth_cb(ptr, depth, max_depth, callback, private_data);
	unlock();
}

static void push_talloc_report_depth_FILE_helper(const void *ptr, int depth, int max_depth, int is_ref, void *_f)
{
	const char *name = push_talloc_get_name(ptr);
	FILE *f = (FILE *)_f;

	if (is_ref) {
		fprintf(f, "%*sreference to: %s\n", depth*4, "", name);
		return;
	}

	if (depth == 0) {
		fprintf(f,"%spush_talloc report on '%s' (total %6lu bytes in %3lu blocks)\n", 
			(max_depth < 0 ? "full " :""), name,
			(unsigned long)_push_talloc_total_size(ptr),
			(unsigned long)_push_talloc_total_blocks(ptr));
		return;
	}

	fprintf(f, "%*s%-30s contains %6lu bytes in %3lu blocks (ref %d) %p\n", 
		depth*4, "",
		name,
		(unsigned long)_push_talloc_total_size(ptr),
		(unsigned long)_push_talloc_total_blocks(ptr),
		(int)_push_talloc_reference_count(ptr), ptr);

#if 0
	fprintf(f, "content: ");
	if (push_talloc_total_size(ptr)) {
		int tot = push_talloc_total_size(ptr);
		int i;

		for (i = 0; i < tot; i++) {
			if ((((char *)ptr)[i] > 31) && (((char *)ptr)[i] < 126)) {
				fprintf(f, "%c", ((char *)ptr)[i]);
			} else {
				fprintf(f, "~%02x", ((char *)ptr)[i]);
			}
		}
	}
	fprintf(f, "\n");
#endif
}

/*
  report on memory usage by all children of a pointer, giving a full tree view
*/
void push_talloc_report_depth_file(const void *ptr, int depth, int max_depth, FILE *f)
{
	push_talloc_report_depth_cb(ptr, depth, max_depth, push_talloc_report_depth_FILE_helper, f);
	fflush(f);
}

/*
  report on memory usage by all children of a pointer, giving a full tree view
*/
void push_talloc_report_full(const void *ptr, FILE *f)
{
	push_talloc_report_depth_file(ptr, 0, -1, f);
}

/*
  report on memory usage by all children of a pointer
*/
void push_talloc_report(const void *ptr, FILE *f)
{
	push_talloc_report_depth_file(ptr, 0, 1, f);
}

/*
  report on any memory hanging off the null context
*/
static void push_talloc_report_null(void)
{
	if (push_talloc_total_size(null_context) != 0) {
		push_talloc_report(null_context, stderr);
	}
}

/*
  report on any memory hanging off the null context
*/
static void push_talloc_report_null_full(void)
{
	if (push_talloc_total_size(null_context) != 0) {
		push_talloc_report_full(null_context, stderr);
	}
}

/*
  enable tracking of the NULL context
*/
void push_talloc_enable_null_tracking(void)
{
	if (null_context == NULL) {
		null_context = _push_talloc_named_const(NULL, 0, "null_context");
	}
}

/*
  disable tracking of the NULL context
*/
void push_talloc_disable_null_tracking(void)
{
	_push_talloc_free(null_context);
	null_context = NULL;
}

/*
  enable leak reporting on exit
*/
void push_talloc_enable_leak_report(void)
{
	push_talloc_enable_null_tracking();
	atexit(push_talloc_report_null);
}

/*
  enable full leak reporting on exit
*/
void push_talloc_enable_leak_report_full(void)
{
	push_talloc_enable_null_tracking();
	atexit(push_talloc_report_null_full);
}

/* 
   push_talloc and zero memory. 
*/
void *_push_talloc_zero(const void *ctx, size_t size, const char *name)
{
	void *p;

	lock(ctx);
	p = _push_talloc_named_const(ctx, size, name);
	unlock();

	if (p) {
		memset(p, '\0', size);
	}

	return p;
}

/*
  memdup with a push_talloc. 
*/
void *_push_talloc_memdup(const void *t, const void *p, size_t size, const char *name)
{
	void *newp;

	lock(t);
	newp = _push_talloc_named_const(t, size, name);
	unlock();

	if (likely(newp)) {
		memcpy(newp, p, size);
	}

	return newp;
}

/*
  strdup with a push_talloc 
*/
char *push_talloc_strdup(const void *t, const char *p)
{
	char *ret;
	if (!p) {
		return NULL;
	}
	ret = (char *)push_talloc_memdup(t, p, strlen(p) + 1);
	if (likely(ret)) {
		_push_talloc_set_name_const(ret, ret);
	}
	return ret;
}

/*
 append to a push_talloced string 
*/
char *push_talloc_append_string(char *orig, const char *append)
{
	char *ret;
	size_t olen = strlen(orig);
	size_t alenz;

	if (!append)
		return orig;

	alenz = strlen(append) + 1;

	ret = push_talloc_realloc(NULL, orig, char, olen + alenz);
	if (!ret)
		return NULL;

	/* append the string with the trailing \0 */
	memcpy(&ret[olen], append, alenz);

	_push_talloc_set_name_const(ret, ret);

	return ret;
}

/*
  strndup with a push_talloc 
*/
char *push_talloc_strndup(const void *t, const char *p, size_t n)
{
	size_t len;
	char *ret;

	for (len=0; len<n && p[len]; len++) ;

	lock(t);
	ret = (char *)__push_talloc(t, len + 1);
	unlock();
	if (!ret) { return NULL; }
	memcpy(ret, p, len);
	ret[len] = 0;
	_push_talloc_set_name_const(ret, ret);
	return ret;
}

char *push_talloc_vasprintf(const void *t, const char *fmt, va_list ap)
{	
	int len;
	char *ret;
	va_list ap2;
	char c;
	
	/* this call looks strange, but it makes it work on older solaris boxes */
	va_copy(ap2, ap);
	len = vsnprintf(&c, 1, fmt, ap2);
	va_end(ap2);
	if (len < 0) {
		return NULL;
	}

	lock(t);
	ret = (char *)__push_talloc(t, len+1);
	unlock();
	if (ret) {
		va_copy(ap2, ap);
		vsnprintf(ret, len+1, fmt, ap2);
		va_end(ap2);
		_push_talloc_set_name_const(ret, ret);
	}

	return ret;
}


/*
  Perform string formatting, and return a pointer to newly allocated
  memory holding the result, inside a memory pool.
 */
char *push_talloc_asprintf(const void *t, const char *fmt, ...)
{
	va_list ap;
	char *ret;

	va_start(ap, fmt);
	ret = push_talloc_vasprintf(t, fmt, ap);
	va_end(ap);
	return ret;
}


/**
 * Realloc @p s to append the formatted result of @p fmt and @p ap,
 * and return @p s, which may have moved.  Good for gradually
 * accumulating output into a string buffer.
 **/
char *push_talloc_vasprintf_append(char *s, const char *fmt, va_list ap)
{	
	struct push_talloc_chunk *tc;
	int len, s_len;
	va_list ap2;
	char c;

	if (s == NULL) {
		return push_talloc_vasprintf(NULL, fmt, ap);
	}

	tc = push_talloc_chunk_from_ptr(s);

	s_len = tc->size - 1;

	va_copy(ap2, ap);
	len = vsnprintf(&c, 1, fmt, ap2);
	va_end(ap2);

	if (len <= 0) {
		/* Either the vsnprintf failed or the format resulted in
		 * no characters being formatted. In the former case, we
		 * ought to return NULL, in the latter we ought to return
		 * the original string. Most current callers of this 
		 * function expect it to never return NULL.
		 */
		return s;
	}

	s = push_talloc_realloc(NULL, s, char, s_len + len+1);
	if (!s) return NULL;

	va_copy(ap2, ap);
	vsnprintf(s+s_len, len+1, fmt, ap2);
	va_end(ap2);
	_push_talloc_set_name_const(s, s);

	return s;
}

/*
  Realloc @p s to append the formatted result of @p fmt and return @p
  s, which may have moved.  Good for gradually accumulating output
  into a string buffer.
 */
char *push_talloc_asprintf_append(char *s, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	s = push_talloc_vasprintf_append(s, fmt, ap);
	va_end(ap);
	return s;
}

/*
  alloc an array, checking for integer overflow in the array size
*/
void *_push_talloc_array(const void *ctx, size_t el_size, unsigned count, const char *name)
{
	void *p;

	if (count >= MAX_PUSH_TALLOC_SIZE/el_size) {
		return NULL;
	}
	lock(ctx);
	p = _push_talloc_named_const(ctx, el_size * count, name);
	unlock();
	return p;
}

/*
  alloc an zero array, checking for integer overflow in the array size
*/
void *_push_talloc_zero_array(const void *ctx, size_t el_size, unsigned count, const char *name)
{
	void *p;

	if (count >= MAX_PUSH_TALLOC_SIZE/el_size) {
		return NULL;
	}
	p = _push_talloc_zero(ctx, el_size * count, name);
	return p;
}

/*
  realloc an array, checking for integer overflow in the array size
*/
void *_push_talloc_realloc_array(const void *ctx, void *ptr, size_t el_size, unsigned count, const char *name)
{
	if (count >= MAX_PUSH_TALLOC_SIZE/el_size) {
		return NULL;
	}
	return _push_talloc_realloc(ctx, ptr, el_size * count, name);
}

/*
  a function version of push_talloc_realloc(), so it can be passed as a function pointer
  to libraries that want a realloc function (a realloc function encapsulates
  all the basic capabilities of an allocation library, which is why this is useful)
*/
void *push_talloc_realloc_fn(const void *context, void *ptr, size_t size)
{
	return _push_talloc_realloc(context, ptr, size, NULL);
}


static int push_talloc_autofree_destructor(void *ptr)
{
	autofree_context = NULL;
	return 0;
}

static void push_talloc_autofree(void)
{
	if (autofree_context && *autofree_context == getpid())
		push_talloc_free(autofree_context);
}

/*
  return a context which will be auto-freed on exit
  this is useful for reducing the noise in leak reports
*/
void *push_talloc_autofree_context(void)
{
	if (autofree_context == NULL || *autofree_context != getpid()) {
		autofree_context = push_talloc(NULL, pid_t);
		*autofree_context = getpid();
		push_talloc_set_name_const(autofree_context, "autofree_context");
		
		push_talloc_set_destructor(autofree_context, push_talloc_autofree_destructor);
		atexit(push_talloc_autofree);
	}
	return autofree_context;
}

size_t push_talloc_get_size(const void *context)
{
	struct push_talloc_chunk *tc;

	if (context == NULL)
		return 0;

	tc = push_talloc_chunk_from_ptr(context);

	return tc->size;
}

/*
  find a parent of this context that has the given name, if any
*/
void *push_talloc_find_parent_byname(const void *context, const char *name)
{
	struct push_talloc_chunk *tc;

	if (context == NULL) {
		return NULL;
	}

	lock(context);
	tc = push_talloc_chunk_from_ptr(context);
	while (tc) {
		if (tc->name && strcmp(tc->name, name) == 0) {
			unlock();
			return TC_PTR_FROM_CHUNK(tc);
		}
		while (tc && tc->prev) tc = tc->prev;
		if (tc) {
			tc = tc->parent;
		}
	}
	unlock();
	return NULL;
}

/*
  show the parentage of a context
*/
void push_talloc_show_parents(const void *context, FILE *file)
{
	struct push_talloc_chunk *tc;

	if (context == NULL) {
		fprintf(file, "push_talloc no parents for NULL\n");
		return;
	}

	lock(context);
	tc = push_talloc_chunk_from_ptr(context);
	fprintf(file, "push_talloc parents of '%s'\n", push_talloc_get_name(context));
	while (tc) {
		fprintf(file, "\t'%s'\n", push_talloc_get_name(TC_PTR_FROM_CHUNK(tc)));
		while (tc && tc->prev) tc = tc->prev;
		if (tc) {
			tc = tc->parent;
		}
	}
	unlock();
	fflush(file);
}

int push_talloc_is_parent(const void *context, const void *ptr)
{
	int ret;
	lock(context);
	ret = _push_talloc_is_parent(context, ptr);
	unlock();
	return ret;
}

void *push_talloc_add_external(const void *ctx,
			  void *(*realloc)(const void *, void *, size_t),
			  void (*lock)(const void *p),
			  void (*unlock)(void))
{
	struct push_talloc_chunk *tc, *parent;
	void *p;

	if (tc_external_realloc && tc_external_realloc != realloc)
		PUSH_TALLOC_ABORT("push_talloc_add_external realloc replaced");
	tc_external_realloc = realloc;

	if (unlikely(ctx == NULL)) {
		ctx = null_context;
		parent = NULL;
	} else
		parent = push_talloc_chunk_from_ptr(ctx);	

	tc = tc_external_realloc(ctx, NULL, TC_HDR_SIZE);
	p = init_push_talloc(parent, tc, 0, 1);
	tc_lock = lock;
	tc_unlock = unlock;

	return p;
}
