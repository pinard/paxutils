## ------------------- ##
## See if %llu works.  ##
## ------------------- ##

# SunOS 4.1.3's printf treats %llu as %lu.  From Alexandre Oliva.

AC_DEFUN(fp_HAVE_PRINTF_LLU,
[AC_CACHE_CHECK(whether printf understands %llu, tar_cv_have_printf_llu,
[AC_TRY_RUN([#include <stdio.h>
main ()
{
  unsigned long long before = 1;
  char buffer[1024];
  while (before)
    {
       unsigned long long after = 0;
       sprintf(buffer, "%llu", before);
       sscanf(buffer, "%llu", &after);
       if (after != before)
	 exit (1);
       before <<= 1;
    }
  exit (0);
}
], tar_cv_have_printf_llu=yes, tar_cv_have_printf_llu=no,
  test "$tar_cv_have_printf_llu" = '' && tar_cv_have_printf_llu=cross)])
test "$tar_cv_have_printf_llu" = yes && AC_DEFINE(HAVE_PRINTF_LLU)
])
