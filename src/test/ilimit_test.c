/*
 * ilimit_test - test limits of ints and overflow charactersitics
 *
 * Thomas Helvey <tomh@inxpress.net>
 *
 * The output of this test should be:
 *
 * ui: 2147483647 i: 2147483647
 * ui: 2147483648 i: -2147483648
 * ui: 4294967295
 * ui: 0
 *
 * If you get results different than the above you may have problems
 * communciating integer values with other servers. Unfortunately there
 * are no guarantees about this sort of thing but this should produce
 * the same results on all 32 bit (ILP32) and most 64 bit hardware (I32LP64).
 * NOTE: This test should have some sort of expiration date, I'm sure this
 * won't be valid forever, but it seems to ok in 1999.
 *
 * $Id: ilimit_test.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
 */
#include <stdio.h>
#include <limits.h>

int main(int argc, char** argv)
{
  unsigned int ui = INT_MAX;
  int          i  = INT_MAX;

  printf("ui: %u i: %d\n", ui, i);
  ui += 1;
  i  += 1;
  printf("ui: %u i: %d\n", ui, i);
  ui = UINT_MAX;
  printf("ui: %u\n", ui);
  ui += 1;
  printf("ui: %u\n", ui);

  return 0;
}
