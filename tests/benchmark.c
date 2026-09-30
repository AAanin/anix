#include "anix.h"
#include <stdio.h>
#include <time.h>
#include "fixture.h"
int main(void){
 unsigned i; uint32_t input=25u,out=0u; clock_t start=clock();
 for(i=0;i<100000u;i++) if(anix_execute(fixture,sizeof(fixture),&input,1,&out)!=ANIX_OK)return 1;
 printf("Validated execute: %.3f us/call (100000 calls, host CPU clock); action=%lu\n",1000000.0*(double)(clock()-start)/(double)CLOCKS_PER_SEC/100000.0,(unsigned long)out);
 return 0;
}
