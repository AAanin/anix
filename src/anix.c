#include "anix.h"
static uint32_t rd(const uint8_t *p) {
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint32_t crc(const uint8_t *p,size_t n) {
 uint32_t c=UINT32_MAX; size_t i; unsigned j;
 for(i=0;i<n;i++){ c^=p[i]; for(j=0;j<8;j++) c=(c>>1)^((0u-(c&1u))&UINT32_C(0xedb88320)); }
 return ~c;
}
static int edge(uint32_t e,uint32_t current,uint32_t base,uint32_t count) {
 return e>current && e>=base && (e-base)%32u==0u && (e-base)/32u<count;
}
anix_status_t anix_validate(const uint8_t *b,size_t len) {
 uint32_t payload,count,inputs,base,i,j,entry; const uint8_t *p,*n;
 if(b==NULL) return ANIX_ARGUMENT;
 if(len<68u) return ANIX_BOUNDS;
 if(b[0]!='A'||b[1]!='N'||b[2]!='I'||b[3]!='X'||rd(b+4)!=1u||rd(b+64)!=1u) return ANIX_FORMAT;
 payload=rd(b+16); count=rd(b+56); inputs=rd(b+60); entry=rd(b+12);
 if(count==0u||count>ANIX_MAX_NODES||inputs>ANIX_MAX_INPUTS) return ANIX_FORMAT;
 base=inputs*8u;
 if(payload!=base+count*32u || len-68u!=(size_t)payload) return ANIX_BOUNDS;
 if(entry<base||(entry-base)%32u!=0u||(entry-base)/32u>=count) return ANIX_BOUNDS;
 p=b+68;
 if(crc(p,payload)!=rd(b+20)) return ANIX_CHECKSUM;
 for(i=0;i<inputs;i++) if(rd(p+i*8u)>rd(p+i*8u+4u)) return ANIX_FORMAT;
 for(i=0;i<count;i++) {
  uint32_t off=base+i*32u; n=p+off;
  if(rd(n)==0u){for(j=2;j<8;j++) if(rd(n+j*4u)!=0u) return ANIX_FORMAT;}
  else if(rd(n)==1u) {
   if(rd(n+4)>=inputs||rd(n+12)>rd(n+16)) return ANIX_FORMAT;
   for(j=5;j<8;j++) if(!edge(rd(n+j*4u),off,base,count)) return ANIX_BOUNDS;
  } else return ANIX_FORMAT;
 }
 return ANIX_OK;
}
anix_status_t anix_execute(const uint8_t *b,size_t len,const uint32_t *in,size_t ilen,uint32_t *out) {
 anix_status_t s; uint32_t off,count,base,k,step; const uint8_t *p,*n;
 if(out==NULL||(in==NULL&&ilen!=0u)) return ANIX_ARGUMENT;
 s=anix_validate(b,len); if(s!=ANIX_OK) return s;
 count=rd(b+56); k=rd(b+60); base=k*8u;
 if(ilen!=(size_t)k||(k!=0u&&in==NULL)) return ANIX_CONTRACT;
 p=b+68; off=rd(b+12);
 for(step=0;step<count;step++) {
  uint32_t index,value;
  if(off<base||(off-base)%32u!=0u||(off-base)/32u>=count) return ANIX_BOUNDS;
  n=p+off; if(rd(n)==0u){*out=rd(n+4);return ANIX_OK;}
  index=rd(n+4); if(index>=k) return ANIX_BOUNDS;
  value=in[index];
  if(value<rd(p+index*8u)||value>rd(p+index*8u+4u)||value<rd(n+12)||value>rd(n+16)) off=rd(n+28);
  else off=rd(n+(value<=rd(n+8)?20u:24u));
 }
 return ANIX_BOUNDS;
}
anix_status_t anix_engine_init(anix_engine_t *e,const uint8_t *b,size_t n,anix_lock_fn enter,anix_lock_fn leave,void *ctx) {
 anix_status_t s;
 if(e==NULL||enter==NULL||leave==NULL) return ANIX_PLATFORM;
 s=anix_validate(b,n); if(s!=ANIX_OK) return s;
 e->banks[0].blob=b;e->banks[0].len=n;e->banks[1]=e->banks[0];e->active=0u;
 e->enter=enter;e->leave=leave;e->context=ctx;return ANIX_OK;
}
anix_status_t anix_hot_swap_bank(anix_engine_t *e,const uint8_t *b,size_t n) {
 anix_status_t s; unsigned next;
 if(e==NULL||e->enter==NULL||e->leave==NULL) return ANIX_PLATFORM;
 s=anix_validate(b,n);if(s!=ANIX_OK)return s;
 e->enter(e->context);next=e->active^1u;e->banks[next].blob=b;e->banks[next].len=n;e->active=next;e->leave(e->context);return ANIX_OK;
}
anix_status_t anix_engine_execute(anix_engine_t *e,const uint32_t *in,size_t n,uint32_t *out) {
 anix_status_t s;
 if(e==NULL||e->enter==NULL||e->leave==NULL)return ANIX_PLATFORM;
 e->enter(e->context);s=anix_execute(e->banks[e->active].blob,e->banks[e->active].len,in,n,out);e->leave(e->context);return s;
}
