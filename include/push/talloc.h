#ifndef PUSH_TALLOC_H
#define PUSH_TALLOC_H
/* 
   Copyright (C) Andrew Tridgell 2004-2005
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <push/typesafe_cb.h>
#include <push/config.h>

/*
  this uses a little trick to allow __LINE__ to be stringified
*/
#ifndef __location__
#define __PUSH_TALLOC_STRING_LINE1__(s)    #s
#define __PUSH_TALLOC_STRING_LINE2__(s)   __PUSH_TALLOC_STRING_LINE1__(s)
#define __PUSH_TALLOC_STRING_LINE3__  __PUSH_TALLOC_STRING_LINE2__(__LINE__)
#define __location__ __FILE__ ":" __PUSH_TALLOC_STRING_LINE3__
#endif

#if HAVE_ATTRIBUTE_PRINTF
/** Use gcc attribute to check printf fns.  a1 is the 1-based index of
 * the parameter containing the format, and a2 the index of the first
 * argument. Note that some gcc 2.x versions don't handle this
 * properly **/
#define PRINTF_ATTRIBUTE(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#else
#define PRINTF_ATTRIBUTE(a1, a2)
#endif

/* try to make push_talloc_set_destructor() and push_talloc_steal() type safe,
   if we have a recent gcc */
#if HAVE_TYPEOF
#define _PUSH_TALLOC_TYPEOF(ptr) __typeof__(ptr)
#else
#define _PUSH_TALLOC_TYPEOF(ptr) void *
#endif

#define push_talloc_move(ctx, ptr) (_PUSH_TALLOC_TYPEOF(*(ptr)))_push_talloc_move((ctx),(void *)(ptr))

/**
 * push_talloc - allocate dynamic memory for a type
 * @ctx: context to be parent of this allocation, or NULL.
 * @type: the type to be allocated.
 *
 * The push_talloc() macro is the core of the push_talloc library. It takes a memory
 * context and a type, and returns a pointer to a new area of memory of the
 * given type.
 *
 * The returned pointer is itself a push_talloc context, so you can use it as the
 * context argument to more calls to push_talloc if you wish.
 *
 * The returned pointer is a "child" of @ctx. This means that if you
 * push_talloc_free() @ctx then the new child disappears as well.  Alternatively you
 * can free just the child.
 *
 * @ctx can be NULL, in which case a new top level context is created.
 *
 * Example:
 *	unsigned int *a, *b;
 *	a = push_talloc(NULL, unsigned int);
 *	b = push_talloc(a, unsigned int);
 *
 * See Also:
 *	push_talloc_zero, push_talloc_array, push_talloc_steal, push_talloc_free.
 */
#define push_talloc(ctx, type) (type *)push_talloc_named_const(ctx, sizeof(type), #type)

/**
 * push_talloc_set - allocate dynamic memory for a type, into a pointer
 * @ptr: pointer to the pointer to assign.
 * @ctx: context to be parent of this allocation, or NULL.
 *
 * push_talloc_set() does a push_talloc, but also adds a destructor which will make the
 * pointer invalid when it is freed.  This can find many use-after-free bugs.
 *
 * Note that the destructor is chained off a zero-length allocation, and so
 * is not affected by push_talloc_set_destructor().
 *
 * Example:
 *	unsigned int *a;
 *	a = push_talloc(NULL, unsigned int);
 *	push_talloc_set(&b, a, unsigned int);
 *	push_talloc_free(a);
 *	*b = 1; // This will crash!
 *
 * See Also:
 *	push_talloc.
 */
#define push_talloc_set(pptr, ctx) \
	_push_talloc_set((pptr), (ctx), sizeof(&**(pptr)), __location__)

/**
 * push_talloc_free - free push_talloc'ed memory and its children
 * @ptr: the push_talloced pointer to free
 *
 * The push_talloc_free() function frees a piece of push_talloc memory, and all its
 * children. You can call push_talloc_free() on any pointer returned by push_talloc().
 *
 * The return value of push_talloc_free() indicates success or failure, with 0
 * returned for success and -1 for failure. The only possible failure condition
 * is if the pointer had a destructor attached to it and the destructor
 * returned -1. See push_talloc_set_destructor() for details on destructors.
 * errno will be preserved unless the push_talloc_free fails.
 *
 * If this pointer has an additional parent when push_talloc_free() is called then
 * the memory is not actually released, but instead the most recently
 * established parent is destroyed. See push_talloc_reference() for details on
 * establishing additional parents.
 *
 * For more control on which parent is removed, see push_talloc_unlink().
 *
 * push_talloc_free() operates recursively on its children.
 *
 * Example:
 *	unsigned int *a, *b;
 *	a = push_talloc(NULL, unsigned int);
 *	b = push_talloc(a, unsigned int);
 *	// Frees a and b
 *	push_talloc_free(a);
 *
 * See Also:
 *	push_talloc_set_destructor, push_talloc_unlink
 */
int push_talloc_free(const void *ptr);

/**
 * push_talloc_set_destructor: set a destructor for when this pointer is freed
 * @ptr: the push_talloc pointer to set the destructor on
 * @destructor: the function to be called
 *
 * The function push_talloc_set_destructor() sets the "destructor" for the pointer
 * @ptr.  A destructor is a function that is called when the memory used by a
 * pointer is about to be released.  The destructor receives the pointer as an
 * argument, and should return 0 for success and -1 for failure.
 *
 * The destructor can do anything it wants to, including freeing other pieces
 * of memory. A common use for destructors is to clean up operating system
 * resources (such as open file descriptors) contained in the structure the
 * destructor is placed on.
 *
 * You can only place one destructor on a pointer. If you need more than one
 * destructor then you can create a zero-length child of the pointer and place
 * an additional destructor on that.
 *
 * To remove a destructor call push_talloc_set_destructor() with NULL for the
 * destructor.
 *
 * If your destructor attempts to push_talloc_free() the pointer that it is the
 * destructor for then push_talloc_free() will return -1 and the free will be
 * ignored. This would be a pointless operation anyway, as the destructor is
 * only called when the memory is just about to go away.
 *
 * Example:
 * static int destroy_fd(int *fd)
 * {
 *	close(*fd);
 *	return 0;
 * }
 *
 * int *open_file(const char *filename)
 * {
 *	int *fd = push_talloc(NULL, int);
 *	*fd = open(filename, O_RDONLY);
 *	if (*fd < 0) {
 *		push_talloc_free(fd);
 *		return NULL;
 *	}
 *	// Whenever they free this, we close the file.
 *	push_talloc_set_destructor(fd, destroy_fd);
 *	return fd;
 * }
 *
 * See Also:
 *	push_talloc, push_talloc_free
 */
#define push_talloc_set_destructor(ptr, function)				      \
	_push_talloc_set_destructor((ptr), typesafe_cb(int, (function), (ptr)))

/**
 * push_talloc_zero - allocate zeroed dynamic memory for a type
 * @ctx: context to be parent of this allocation, or NULL.
 * @type: the type to be allocated.
 *
 * The push_talloc_zero() macro is equivalent to:
 *
 *  ptr = push_talloc(ctx, type);
 *  if (ptr) memset(ptr, 0, sizeof(type));
 *
 * Example:
 *	unsigned int *a, *b;
 *	a = push_talloc_zero(NULL, unsigned int);
 *	b = push_talloc_zero(a, unsigned int);
 *
 * See Also:
 *	push_talloc, push_talloc_zero_size, push_talloc_zero_array
 */
#define push_talloc_zero(ctx, type) (type *)_push_talloc_zero(ctx, sizeof(type), #type)

/**
 * push_talloc_array - allocate dynamic memory for an array of a given type
 * @ctx: context to be parent of this allocation, or NULL.
 * @type: the type to be allocated.
 * @count: the number of elements to be allocated.
 *
 * The push_talloc_array() macro is a safe way of allocating an array.  It is
 * equivalent to:
 *
 *  (type *)push_talloc_size(ctx, sizeof(type) * count);
 *
 * except that it provides integer overflow protection for the multiply,
 * returning NULL if the multiply overflows.
 *
 * Example:
 *	unsigned int *a, *b;
 *	a = push_talloc_zero(NULL, unsigned int);
 *	b = push_talloc_array(a, unsigned int, 100);
 *
 * See Also:
 *	push_talloc, push_talloc_zero_array
 */
#define push_talloc_array(ctx, type, count) (type *)_push_talloc_array(ctx, sizeof(type), count, #type)

/**
 * push_talloc_size - allocate a particular size of memory
 * @ctx: context to be parent of this allocation, or NULL.
 * @size: the number of bytes to allocate
 *
 * The function push_talloc_size() should be used when you don't have a convenient
 * type to pass to push_talloc(). Unlike push_talloc(), it is not type safe (as it
 * returns a void *), so you are on your own for type checking.
 *
 * Best to use push_talloc() or push_talloc_array() instead.
 *
 * Example:
 *	void *mem = push_talloc_size(NULL, 100);
 *
 * See Also:
 *	push_talloc, push_talloc_array, push_talloc_zero_size
 */
#define push_talloc_size(ctx, size) push_talloc_named_const(ctx, size, __location__)

#ifdef HAVE_TYPEOF
/**
 * push_talloc_steal - change/set the parent context of a push_talloc pointer
 * @ctx: the new parent
 * @ptr: the push_talloc pointer to reparent
 *
 * The push_talloc_steal() function changes the parent context of a push_talloc
 * pointer. It is typically used when the context that the pointer is currently
 * a child of is going to be freed and you wish to keep the memory for a longer
 * time.
 *
 * The push_talloc_steal() function returns the pointer that you pass it. It does
 * not have any failure modes.
 *
 * NOTE: It is possible to produce loops in the parent/child relationship if
 * you are not careful with push_talloc_steal(). No guarantees are provided as to
 * your sanity or the safety of your data if you do this.
 *
 * push_talloc_steal (new_ctx, NULL) will return NULL with no sideeffects.
 *
 * Example:
 *	unsigned int *a, *b;
 *	a = push_talloc(NULL, unsigned int);
 *	b = push_talloc(NULL, unsigned int);
 *	// Reparent b to a as if we'd done 'b = push_talloc(a, unsigned int)'.
 *	push_talloc_steal(a, b);
 *
 * See Also:
 *	push_talloc_reference
 */
#define push_talloc_steal(ctx, ptr) ({ _PUSH_TALLOC_TYPEOF(ptr) _push_talloc_steal_ret = (_PUSH_TALLOC_TYPEOF(ptr))_push_talloc_steal((ctx),(ptr)); _push_talloc_steal_ret; }) /* this extremely strange macro is to avoid some braindamaged warning stupidity in gcc 4.1.x */
#else
#define push_talloc_steal(ctx, ptr) (_PUSH_TALLOC_TYPEOF(ptr))_push_talloc_steal((ctx),(ptr))
#endif /* HAVE_TYPEOF */

/**
 * push_talloc_report_full - report all the memory used by a pointer and children.
 * @ptr: the context to report on
 * @f: the file to report to
 *
 * Recursively print the entire tree of memory referenced by the
 * pointer. References in the tree are shown by giving the name of the pointer
 * that is referenced.
 *
 * You can pass NULL for the pointer, in which case a report is printed for the
 * top level memory context, but only if push_talloc_enable_null_tracking() has been
 * called.
 *
 * Example:
 *	unsigned int *a, *b;
 *	a = push_talloc(NULL, unsigned int);
 *	b = push_talloc(a, unsigned int);
 *	fprintf(stderr, "Dumping memory tree for a:\n");
 *	push_talloc_report_full(a, stderr);
 *
 * See Also:
 *	push_talloc_report
 */
void push_talloc_report_full(const void *ptr, FILE *f);

/**
 * push_talloc_reference - add an additional parent to a context
 * @ctx: the additional parent
 * @ptr: the push_talloc pointer
 *
 * The push_talloc_reference() function makes @ctx an additional parent of @ptr.
 *
 * The return value of push_talloc_reference() is always the original pointer @ptr,
 * unless push_talloc ran out of memory in creating the reference in which case it
 * will return NULL (each additional reference consumes around 48 bytes of
 * memory on intel x86 platforms).
 *
 * If @ptr is NULL, then the function is a no-op, and simply returns NULL.
 *
 * After creating a reference you can free it in one of the following ways:
 *
 *  - you can push_talloc_free() any parent of the original pointer. That will
 *    reduce the number of parents of this pointer by 1, and will cause this
 *    pointer to be freed if it runs out of parents.
 *
 *  - you can push_talloc_free() the pointer itself. That will destroy the most
 *    recently established parent to the pointer and leave the pointer as a
 *    child of its current parent.
 *
 * For more control on which parent to remove, see push_talloc_unlink().
 * Example:
 *	unsigned int *a, *b, *c;
 *	a = push_talloc(NULL, unsigned int);
 *	b = push_talloc(NULL, unsigned int);
 *	c = push_talloc(a, unsigned int);
 *	// b also serves as a parent of c.
 *	push_talloc_reference(b, c);
 */
#define push_talloc_reference(ctx, ptr) (_PUSH_TALLOC_TYPEOF(ptr))_push_talloc_reference((ctx),(ptr))

/**
 * push_talloc_unlink: remove a specific parent from a push_talloc pointer.
 * @context: the parent to remove
 * @ptr: the push_talloc pointer
 *
 * The push_talloc_unlink() function removes a specific parent from @ptr. The
 * context passed must either be a context used in push_talloc_reference() with this
 * pointer, or must be a direct parent of @ptr.
 *
 * Note that if the parent has already been removed using push_talloc_free() then
 * this function will fail and will return -1.  Likewise, if @ptr is NULL,
 * then the function will make no modifications and return -1.
 *
 * Usually you can just use push_talloc_free() instead of push_talloc_unlink(), but
 * sometimes it is useful to have the additional control on which parent is
 * removed.
 * Example:
 *	unsigned int *a, *b, *c;
 *	a = push_talloc(NULL, unsigned int);
 *	b = push_talloc(NULL, unsigned int);
 *	c = push_talloc(a, unsigned int);
 *	// b also serves as a parent of c.
 *	push_talloc_reference(b, c);
 *	push_talloc_unlink(b, c);
 */
int push_talloc_unlink(const void *context, void *ptr);

/**
 * push_talloc_report - print a summary of memory used by a pointer
 *
 * The push_talloc_report() function prints a summary report of all memory
 * used by @ptr.  One line of report is printed for each immediate child of
 * @ptr, showing the total memory and number of blocks used by that child.
 *
 * You can pass NULL for the pointer, in which case a report is printed for the
 * top level memory context, but only if push_talloc_enable_null_tracking() has been
 * called.
 *
 * Example:
 *	unsigned int *a, *b;
 *	a = push_talloc(NULL, unsigned int);
 *	b = push_talloc(a, unsigned int);
 *	fprintf(stderr, "Summary of memory tree for a:\n");
 *	push_talloc_report(a, stderr);
 *
 * See Also:
 *	push_talloc_report_full
 */
void push_talloc_report(const void *ptr, FILE *f);

/**
 * push_talloc_ptrtype - allocate a size of memory suitable for this pointer
 * @ctx: context to be parent of this allocation, or NULL.
 * @ptr: the pointer whose type we are to allocate
 *
 * The push_talloc_ptrtype() macro should be used when you have a pointer and
 * want to allocate memory to point at with this pointer. When compiling
 * with gcc >= 3 it is typesafe. Note this is a wrapper of push_talloc_size()
 * and push_talloc_get_name() will return the current location in the source file.
 * and not the type.
 *
 * Example:
 *	unsigned int *a = push_talloc_ptrtype(NULL, a);
 */
#define push_talloc_ptrtype(ctx, ptr) (_PUSH_TALLOC_TYPEOF(ptr))push_talloc_size(ctx, sizeof(*(ptr)))

/**
 * push_talloc_new - create a new context
 * @ctx: the context to use as a parent.
 *
 * This is a utility macro that creates a new memory context hanging off an
 * exiting context, automatically naming it "push_talloc_new: __location__" where
 * __location__ is the source line it is called from. It is particularly useful
 * for creating a new temporary working context.
 */
#define push_talloc_new(ctx) push_talloc_named_const(ctx, 0, "push_talloc_new: " __location__)

/**
 * push_talloc_zero_size -  allocate a particular size of zeroed memory
 *
 * The push_talloc_zero_size() function is useful when you don't have a known type.
 */
#define push_talloc_zero_size(ctx, size) _push_talloc_zero(ctx, size, __location__)

/**
 * push_talloc_zero_array -  allocate an array of zeroed types
 * @ctx: context to be parent of this allocation, or NULL.
 * @type: the type to be allocated.
 * @count: the number of elements to be allocated.
 *
 * Just like push_talloc_array, but zeroes the memory.
 */
#define push_talloc_zero_array(ctx, type, count) (type *)_push_talloc_zero_array(ctx, sizeof(type), count, #type)

/**
 * push_talloc_array_size - allocate an array of elements of the given size
 * @ctx: context to be parent of this allocation, or NULL.
 * @size: the size of each element
 * @count: the number of elements to be allocated.
 *
 * Typeless form of push_talloc_array.
 */
#define push_talloc_array_size(ctx, size, count) _push_talloc_array(ctx, size, count, __location__)

/**
 * push_talloc_array_ptrtype - allocate an array of memory suitable for this pointer
 * @ctx: context to be parent of this allocation, or NULL.
 * @ptr: the pointer whose type we are to allocate
 * @count: the number of elements for the array
 *
 * Like push_talloc_ptrtype(), except it allocates an array.
 */
#define push_talloc_array_ptrtype(ctx, ptr, count) (_PUSH_TALLOC_TYPEOF(ptr))push_talloc_array_size(ctx, sizeof(*(ptr)), count)

/**
 * push_talloc_realloc - resize a push_talloc array
 * @ctx: the parent to assign (if p is NULL)
 * @p: the memory to reallocate
 * @type: the type of the object to allocate
 * @count: the number of objects to reallocate
 *
 * The push_talloc_realloc() macro changes the size of a push_talloc pointer. The "count"
 * argument is the number of elements of type "type" that you want the
 * resulting pointer to hold.
 *
 * push_talloc_realloc() has the following equivalences:
 *
 *  push_talloc_realloc(context, NULL, type, 1) ==> push_talloc(context, type);
 *  push_talloc_realloc(context, NULL, type, N) ==> push_talloc_array(context, type, N);
 *  push_talloc_realloc(context, ptr, type, 0)  ==> push_talloc_free(ptr);
 *
 * The "context" argument is only used if "ptr" is NULL, otherwise it is
 * ignored.
 *
 * push_talloc_realloc() returns the new pointer, or NULL on failure. The call will
 * fail either due to a lack of memory, or because the pointer has more than
 * one parent (see push_talloc_reference()).
 */
#define push_talloc_realloc(ctx, p, type, count) (type *)_push_talloc_realloc_array(ctx, p, sizeof(type), count, #type)

/**
 * push_talloc_realloc_size - resize push_talloc memory
 * @ctx: the parent to assign (if p is NULL)
 * @ptr: the memory to reallocate
 * @size: the new size of memory.
 *
 * The push_talloc_realloc_size() function is useful when the type is not known so
 * the typesafe push_talloc_realloc() cannot be used.
 */
#define push_talloc_realloc_size(ctx, ptr, size) _push_talloc_realloc(ctx, ptr, size, __location__)

/**
 * push_talloc_strdup - duplicate a string
 * @ctx: the push_talloc context for the new string
 * @p: the string to copy
 *
 * The push_talloc_strdup() function is equivalent to:
 *
 *  ptr = push_talloc_size(ctx, strlen(p)+1);
 *  if (ptr) memcpy(ptr, p, strlen(p)+1);
 *
 * This functions sets the name of the new pointer to the passed string. This
 * is equivalent to:
 *
 *  push_talloc_set_name_const(ptr, ptr)
 */
char *push_talloc_strdup(const void *t, const char *p);

/**
 * push_talloc_strndup - duplicate a limited length of a string
 * @ctx: the push_talloc context for the new string
 * @p: the string to copy
 * @n: the maximum length of the returned string.
 *
 * The push_talloc_strndup() function is the push_talloc equivalent of the C library
 * function strndup(): the result will be truncated to @n characters before
 * the nul terminator.
 *
 * This functions sets the name of the new pointer to the passed string. This
 * is equivalent to:
 *
 *   push_talloc_set_name_const(ptr, ptr)
 */
char *push_talloc_strndup(const void *t, const char *p, size_t n);

/**
 * push_talloc_memdup - duplicate some push_talloc memory
 *
 * The push_talloc_memdup() function is equivalent to:
 *
 *  ptr = push_talloc_size(ctx, size);
 *  if (ptr) memcpy(ptr, p, size);
 */
#define push_talloc_memdup(t, p, size) _push_talloc_memdup(t, p, size, __location__)

/**
 * push_talloc_asprintf - sprintf into a push_talloc buffer.
 * @t: The context to allocate the buffer from
 * @fmt: printf-style format for the buffer.
 *
 * The push_talloc_asprintf() function is the push_talloc equivalent of the C library
 * function asprintf().
 *
 * This functions sets the name of the new pointer to the new string. This is
 * equivalent to:
 *
 *   push_talloc_set_name_const(ptr, ptr)
 */
char *push_talloc_asprintf(const void *t, const char *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 * push_talloc_append_string - concatenate onto a push_tallocated string 
 * @orig: the push_tallocated string to append to
 * @append: the string to add, or NULL to add nothing.
 *
 * The push_talloc_append_string() function appends the given formatted string to
 * the given string.
 *
 * This function sets the name of the new pointer to the new string. This is
 * equivalent to:
 *
 *    push_talloc_set_name_const(ptr, ptr)
 */
char *push_talloc_append_string(char *orig, const char *append);

/**
 * push_talloc_asprintf_append - sprintf onto the end of a push_talloc buffer.
 * @s: The push_tallocated string buffer
 * @fmt: printf-style format to append to the buffer.
 *
 * The push_talloc_asprintf_append() function appends the given formatted string to
 * the given string.
 *
 * This functions sets the name of the new pointer to the new string. This is
 * equivalent to:
 *   push_talloc_set_name_const(ptr, ptr)
 */
char *push_talloc_asprintf_append(char *s, const char *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 * push_talloc_vasprintf - vsprintf into a push_talloc buffer.
 * @t: The context to allocate the buffer from
 * @fmt: printf-style format for the buffer
 * @ap: va_list arguments
 *
 * The push_talloc_vasprintf() function is the push_talloc equivalent of the C library
 * function vasprintf()
 *
 * This functions sets the name of the new pointer to the new string. This is
 * equivalent to:
 *
 *   push_talloc_set_name_const(ptr, ptr)
 */
char *push_talloc_vasprintf(const void *t, const char *fmt, va_list ap) PRINTF_ATTRIBUTE(2,0);

/**
 * push_talloc_vasprintf_append - sprintf onto the end of a push_talloc buffer.
 * @t: The context to allocate the buffer from
 * @fmt: printf-style format for the buffer
 * @ap: va_list arguments
 *
 * The push_talloc_vasprintf_append() function is equivalent to
 * push_talloc_asprintf_append(), except it takes a va_list.
 */
char *push_talloc_vasprintf_append(char *s, const char *fmt, va_list ap) PRINTF_ATTRIBUTE(2,0);

/**
 * push_talloc_set_type - force the name of a pointer to a particular type
 * @ptr: the push_talloc pointer
 * @type: the type whose name to set the ptr name to.
 *
 * This macro allows you to force the name of a pointer to be a particular
 * type. This can be used in conjunction with push_talloc_get_type() to do type
 * checking on void* pointers.
 *
 * It is equivalent to this:
 *   push_talloc_set_name_const(ptr, #type)
 */
#define push_talloc_set_type(ptr, type) push_talloc_set_name_const(ptr, #type)

/**
 * push_talloc_get_type - convert a push_talloced pointer with typechecking
 * @ptr: the push_talloc pointer
 * @type: the type which we expect the push_talloced pointer to be.
 *
 * This macro allows you to do type checking on push_talloc pointers. It is
 * particularly useful for void* private pointers. It is equivalent to this:
 *
 *   (type *)push_talloc_check_name(ptr, #type)
 */
#define push_talloc_get_type(ptr, type) (type *)push_talloc_check_name(ptr, #type)

/**
 * push_talloc_find_parent_byname - find a push_talloc parent by type
 * @ptr: the push_talloc pointer
 * @type: the type we're looking for
 *
 * Find a parent memory context of the current context that has the given
 * name. This can be very useful in complex programs where it may be difficult
 * to pass all information down to the level you need, but you know the
 * structure you want is a parent of another context.
 */
#define push_talloc_find_parent_bytype(ptr, type) (type *)push_talloc_find_parent_byname(ptr, #type)

/**
 * push_talloc_increase_ref_count - hold a reference to a push_talloc pointer
 * @ptr: the push_talloc pointer
 *
 * The push_talloc_increase_ref_count(ptr) function is exactly equivalent to:
 *
 *  push_talloc_reference(NULL, ptr);
 *
 * You can use either syntax, depending on which you think is clearer in your
 * code.
 *
 * It returns 0 on success and -1 on failure.
 */
int push_talloc_increase_ref_count(const void *ptr);

/**
 * push_talloc_set_name - set the name for a push_talloc pointer
 * @ptr: the push_talloc pointer
 * @fmt: the printf-style format string for the name
 *
 * Each push_talloc pointer has a "name". The name is used principally for debugging
 * purposes, although it is also possible to set and get the name on a pointer
 * in as a way of "marking" pointers in your code.
 *
 * The main use for names on pointer is for "push_talloc reports". See
 * push_talloc_report() and push_talloc_report_full() for details. Also see
 * push_talloc_enable_leak_report() and push_talloc_enable_leak_report_full().
 *
 * The push_talloc_set_name() function allocates memory as a child of the
 * pointer. It is logically equivalent to:
 *   push_talloc_set_name_const(ptr, push_talloc_asprintf(ptr, fmt, ...));
 *
 * Note that multiple calls to push_talloc_set_name() will allocate more memory
 * without releasing the name. All of the memory is released when the ptr is
 * freed using push_talloc_free().
 */
const char *push_talloc_set_name(const void *ptr, const char *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 * push_talloc_set_name_const - set a push_talloc pointer name to a string constant
 * @ptr: the push_talloc pointer to name
 * @name: the strucng constant.
 *
 * The function push_talloc_set_name_const() is just like push_talloc_set_name(), but it
 * takes a string constant, and is much faster. It is extensively used by the
 * "auto naming" macros, such as push_talloc().
 *
 * This function does not allocate any memory. It just copies the supplied
 * pointer into the internal representation of the push_talloc ptr. This means you
 * must not pass a name pointer to memory that will disappear before the ptr is
 * freed with push_talloc_free().
 */
void push_talloc_set_name_const(const void *ptr, const char *name);

/**
 * push_talloc_named - create a specifically-named push_talloc pointer
 * @context: the parent context for the allocation
 * @size: the size to allocate
 * @fmt: the printf-style format for the name
 *
 * The push_talloc_named() function creates a named push_talloc pointer. It is equivalent
 * to:
 *
 *   ptr = push_talloc_size(context, size);
 *   push_talloc_set_name(ptr, fmt, ....);
 */
void *push_talloc_named(const void *context, size_t size, 
		   const char *fmt, ...) PRINTF_ATTRIBUTE(3,4);

/**
 * push_talloc_named_const - create a specifically-named push_talloc pointer
 * @context: the parent context for the allocation
 * @size: the size to allocate
 * @name: the string constant to use as the name
 *
 * This is equivalent to:
 *
 *   ptr = push_talloc_size(context, size);
 *   push_talloc_set_name_const(ptr, name);
 */
void *push_talloc_named_const(const void *context, size_t size, const char *name);

/**
 * push_talloc_get_name - get the name of a push_talloc pointer
 * @ptr: the push_talloc pointer
 *
 * This returns the current name for the given push_talloc pointer. See
 * push_talloc_set_name() for details.
 */
const char *push_talloc_get_name(const void *ptr);

/**
 * push_talloc_check_name - check if a pointer has the specified name
 * @ptr: the push_talloc pointer
 * @name: the name to compare with the pointer's name
 *
 * This function checks if a pointer has the specified name. If it does then
 * the pointer is returned. It it doesn't then NULL is returned.
 */
void *push_talloc_check_name(const void *ptr, const char *name);

/**
 * push_talloc_init - create a top-level context of particular name
 * @fmt: the printf-style format of the name
 *
 * This function creates a zero length named push_talloc context as a top level
 * context. It is equivalent to:
 *
 *   push_talloc_named(NULL, 0, fmt, ...);
 */
void *push_talloc_init(const char *fmt, ...) PRINTF_ATTRIBUTE(1,2);

/**
 * push_talloc_total_size - get the bytes used by the pointer and its children
 * @ptr: the push_talloc pointer
 *
 * The push_talloc_total_size() function returns the total size in bytes used by
 * this pointer and all child pointers. Mostly useful for debugging.
 *
 * Passing NULL is allowed, but it will only give a meaningful result if
 * push_talloc_enable_leak_report() or push_talloc_enable_leak_report_full() has been
 * called.
 */
size_t push_talloc_total_size(const void *ptr);

/**
 * push_talloc_total_blocks - get the number of allocations for the pointer
 * @ptr: the push_talloc pointer
 *
 * The push_talloc_total_blocks() function returns the total allocations used by
 * this pointer and all child pointers. Mostly useful for debugging. For
 * example, a pointer with no children will return "1".
 *
 * Passing NULL is allowed, but it will only give a meaningful result if
 * push_talloc_enable_leak_report() or push_talloc_enable_leak_report_full() has been
 * called.
 */
size_t push_talloc_total_blocks(const void *ptr);

/**
 * push_talloc_report_depth_cb - walk the entire push_talloc tree under a push_talloc pointer
 * @ptr: the push_talloc pointer to recurse under
 * @depth: the current depth of traversal
 * @max_depth: maximum depth to traverse, or -1 for no maximum
 * @callback: the function to call on each pointer
 * @private_data: pointer to hand to @callback.
 *
 * This provides a more flexible reports than push_talloc_report(). It will
 * recursively call the callback for the entire tree of memory referenced by
 * the pointer. References in the tree are passed with is_ref = 1 and the
 * pointer that is referenced.
 *
 * You can pass NULL for the pointer, in which case a report is printed for the
 * top level memory context, but only if push_talloc_enable_leak_report() or
 * push_talloc_enable_leak_report_full() has been called.
 *
 * The recursion is stopped when depth >= max_depth.  max_depth = -1 means only
 * stop at leaf nodes.
 */
void push_talloc_report_depth_cb(const void *ptr, int depth, int max_depth,
			    void (*callback)(const void *ptr,
			  		     int depth, int max_depth,
					     int is_ref,
					     void *private_data),
			    void *private_data);

/**
 * push_talloc_report_depth_file - report push_talloc usage to a maximum depth
 * @ptr: the push_talloc pointer to recurse under
 * @depth: the current depth of traversal
 * @max_depth: maximum depth to traverse, or -1 for no maximum
 * @f: the file to report to
 *
 * This provides a more flexible reports than push_talloc_report(). It will let you
 * specify the depth and max_depth.
 */
void push_talloc_report_depth_file(const void *ptr, int depth, int max_depth, FILE *f);

/**
 * push_talloc_enable_null_tracking - enable tracking of top-level push_tallocs
 *
 * This enables tracking of the NULL memory context without enabling leak
 * reporting on exit. Useful for when you want to do your own leak reporting
 * call via push_talloc_report_null_full();
 */
void push_talloc_enable_null_tracking(void);

/**
 * push_talloc_disable_null_tracking - enable tracking of top-level push_tallocs
 *
 * This disables tracking of the NULL memory context.
 */
void push_talloc_disable_null_tracking(void);

/**
 * push_talloc_enable_leak_report - call push_talloc_report on program exit
 *
 * This enables calling of push_talloc_report(NULL, stderr) when the program
 * exits. In Samba4 this is enabled by using the --leak-report command line
 * option.
 *
 * For it to be useful, this function must be called before any other push_talloc
 * function as it establishes a "null context" that acts as the top of the
 * tree. If you don't call this function first then passing NULL to
 * push_talloc_report() or push_talloc_report_full() won't give you the full tree
 * printout.
 *
 * Here is a typical push_talloc report:
 *
 * push_talloc report on 'null_context' (total 267 bytes in 15 blocks)
 *         libcli/auth/spnego_parse.c:55  contains     31 bytes in   2 blocks
 *         libcli/auth/spnego_parse.c:55  contains     31 bytes in   2 blocks
 *         iconv(UTF8,CP850)              contains     42 bytes in   2 blocks
 *         libcli/auth/spnego_parse.c:55  contains     31 bytes in   2 blocks
 *         iconv(CP850,UTF8)              contains     42 bytes in   2 blocks
 *         iconv(UTF8,UTF-16LE)           contains     45 bytes in   2 blocks
 *         iconv(UTF-16LE,UTF8)           contains     45 bytes in   2 blocks
 */
void push_talloc_enable_leak_report(void);

/**
 * push_talloc_enable_leak_report - call push_talloc_report_full on program exit
 *
 * This enables calling of push_talloc_report_full(NULL, stderr) when the program
 * exits. In Samba4 this is enabled by using the --leak-report-full command
 * line option.
 *
 * For it to be useful, this function must be called before any other push_talloc
 * function as it establishes a "null context" that acts as the top of the
 * tree. If you don't call this function first then passing NULL to
 * push_talloc_report() or push_talloc_report_full() won't give you the full tree
 * printout.
 *
 * Here is a typical full report:
 *
 * full push_talloc report on 'root' (total 18 bytes in 8 blocks)
 *    p1                        contains     18 bytes in   7 blocks (ref 0)
 *         r1                        contains     13 bytes in   2 blocks (ref 0)
 *             reference to: p2
 *         p2                        contains      1 bytes in   1 blocks (ref 1)
 *         x3                        contains      1 bytes in   1 blocks (ref 0)
 *         x2                        contains      1 bytes in   1 blocks (ref 0)
 *         x1                        contains      1 bytes in   1 blocks (ref 0)
 */
void push_talloc_enable_leak_report_full(void);

/**
 * push_talloc_autofree_context - a context which will be freed at exit
 *
 * This is a handy utility function that returns a push_talloc context which will be
 * automatically freed on program exit. This can be used to reduce the noise in
 * memory leak reports.
 */
void *push_talloc_autofree_context(void);

/**
 * push_talloc_get_size - get the size of an allocation
 * @ctx: the push_talloc pointer whose allocation to measure.
 *
 * This function lets you know the amount of memory alloced so far by this
 * context. It does NOT account for subcontext memory.  This can be used to
 * calculate the size of an array.
 */
size_t push_talloc_get_size(const void *ctx);

/**
 * push_talloc_find_parent_byname - find a parent of this context with this name
 * @ctx: the context whose ancestors to search
 * @name: the name to look for
 *
 * Find a parent memory context of @ctx that has the given name. This can be
 * very useful in complex programs where it may be difficult to pass all
 * information down to the level you need, but you know the structure you want
 * is a parent of another context.
 */
void *push_talloc_find_parent_byname(const void *ctx, const char *name);

/**
 * push_talloc_add_external - create an externally allocated node
 * @ctx: the parent
 * @realloc: the realloc() equivalent
 * @lock: the call to lock before manipulation of external nodes
 * @unlock: the call to unlock after manipulation of external nodes
 *
 * push_talloc_add_external() creates a node which uses a separate allocator.  All
 * children allocated from that node will also use that allocator.
 *
 * Note: Currently there is only one external allocator, not per-node,
 * and it is set with this function.
 *
 * @lock is handed a pointer which was previous returned from your realloc
 * function; you should use that to figure out which lock to get if you have
 * multiple external pools.
 *
 * The parent pointers in realloc is the push_talloc pointer of the parent, if any.
 */
void *push_talloc_add_external(const void *ctx,
			  void *(*realloc)(const void *parent,
					   void *ptr, size_t),
			  void (*lock)(const void *p),
			  void (*unlock)(void));

/* The following definitions come from push_talloc.c  */
void *_push_talloc(const void *context, size_t size);
void _push_talloc_set(void *ptr, const void *ctx, size_t size, const char *name);
void _push_talloc_set_destructor(const void *ptr, int (*destructor)(void *));
size_t push_talloc_reference_count(const void *ptr);
void *_push_talloc_reference(const void *context, const void *ptr);

void *_push_talloc_realloc(const void *context, void *ptr, size_t size, const char *name);
void *push_talloc_parent(const void *ptr);
const char *push_talloc_parent_name(const void *ptr);
void *_push_talloc_steal(const void *new_ctx, const void *ptr);
void *_push_talloc_move(const void *new_ctx, const void *pptr);
void *_push_talloc_zero(const void *ctx, size_t size, const char *name);
void *_push_talloc_memdup(const void *t, const void *p, size_t size, const char *name);
void *_push_talloc_array(const void *ctx, size_t el_size, unsigned count, const char *name);
void *_push_talloc_zero_array(const void *ctx, size_t el_size, unsigned count, const char *name);
void *_push_talloc_realloc_array(const void *ctx, void *ptr, size_t el_size, unsigned count, const char *name);
void *push_talloc_realloc_fn(const void *context, void *ptr, size_t size);
void push_talloc_show_parents(const void *context, FILE *file);
int push_talloc_is_parent(const void *context, const void *ptr);

#endif /* PUSH_TALLOC_H */
