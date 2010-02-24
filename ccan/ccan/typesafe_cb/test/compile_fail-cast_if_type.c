#include <ccan/typesafe_cb/typesafe_cb.h>

void _set_some_value(void *val);

void _set_some_value(void *val)
{
}

#define set_some_value(expr)						\
	_set_some_value(cast_if_type((expr), unsigned long, void *))

int main(int argc, char *argv[])
{
#ifdef FAIL
	int x = 0;
	set_some_value(x);
#if !HAVE_TYPEOF||!HAVE_BUILTIN_CHOOSE_EXPR||!HAVE_BUILTIN_TYPES_COMPATIBLE_P
#error "Unfortunately we don't fail if cast_if_type is a noop."
#endif
#else
	void *p = 0;
	set_some_value(p);
#endif
	return 0;
}
