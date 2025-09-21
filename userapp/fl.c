#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int big_data()
{
  printf("%.2LfT", __INT64_C(1000000000000000)/1000000000000.0L);
  return 0;
}