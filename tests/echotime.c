/*
 * echotime.c:  Print the time in seconds since Jan 1 1970 00:00 GMT.
 * Usage: 
 * $ ./echotime
 * 707152068
 */

main ()
{
	long	t;
	time (&t);
	printf ("%d\n", t);
	return (0);
}
