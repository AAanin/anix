#include "anix.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fixture.h"
static unsigned locked;
static void enter(void *ctx){(void)ctx;assert(locked==0u);locked=1u;}
static void leave(void *ctx){(void)ctx;assert(locked==1u);locked=0u;}
int main(void){
 uint32_t input=25u,out=777u; uint8_t bad[sizeof(fixture)]; size_t i;
 anix_engine_t e; uint8_t unaligned[sizeof(fixture)+1u];
 assert(anix_execute(fixture,sizeof(fixture),&input,1,&out)==ANIX_OK&&out==10u);
 input=50u;assert(anix_execute(fixture,sizeof(fixture),&input,1,&out)==ANIX_OK&&out==10u);
 input=51u;assert(anix_execute(fixture,sizeof(fixture),&input,1,&out)==ANIX_OK&&out==20u);
 memcpy(unaligned+1,fixture,sizeof(fixture));assert(anix_execute(unaligned+1,sizeof(fixture),&input,1,&out)==ANIX_OK&&out==20u);
 assert(anix_execute(fixture,sizeof(fixture),NULL,1,&out)==ANIX_ARGUMENT);
 assert(anix_engine_init(&e,fixture,sizeof(fixture),NULL,leave,NULL)==ANIX_PLATFORM);
 input=75u;assert(anix_execute(fixture,sizeof(fixture),&input,1,&out)==ANIX_OK&&out==20u);
 input=101u;assert(anix_execute(fixture,sizeof(fixture),&input,1,&out)==ANIX_OK&&out==99u);
 input=UINT32_MAX;assert(anix_execute(fixture,sizeof(fixture),&input,1,&out)==ANIX_OK&&out==99u);
 assert(anix_execute(fixture,sizeof(fixture),&input,0,&out)==ANIX_CONTRACT);
 assert(anix_execute(NULL,0,&input,1,&out)==ANIX_ARGUMENT);
 assert(anix_execute(fixture,sizeof(fixture),&input,1,NULL)==ANIX_ARGUMENT);
 for(i=0;i<sizeof(fixture);i++)assert(anix_validate(fixture,i)!=ANIX_OK);
 memcpy(bad,fixture,sizeof(bad));bad[68]^=1u;
 out=777u;assert(anix_execute(bad,sizeof(bad),&input,1,&out)==ANIX_CHECKSUM&&out==777u);
 memcpy(bad,fixture,sizeof(bad));bad[0]=0;assert(anix_validate(bad,sizeof(bad))==ANIX_FORMAT);
 assert(anix_validate(cycle_fixture,sizeof(cycle_fixture))==ANIX_BOUNDS);
 assert(anix_engine_init(&e,fixture,sizeof(fixture),enter,leave,NULL)==ANIX_OK);
 input=25u;assert(anix_engine_execute(&e,&input,1,&out)==ANIX_OK&&out==10u);
 assert(anix_hot_swap_bank(&e,alternate,sizeof(alternate))==ANIX_OK&&e.active==1u);
 assert(anix_engine_execute(&e,&input,1,&out)==ANIX_OK&&out==20u);
 assert(anix_hot_swap_bank(&e,bad,sizeof(bad))==ANIX_FORMAT&&e.active==1u);
 assert(anix_hot_swap_bank(&e,fixture,sizeof(fixture))==ANIX_OK&&e.active==0u);
 puts("PASS: graph decisions, shadow outliers, truncations, CRC, cycle rejection, output preservation, dual-bank swaps");
 return 0;
}
