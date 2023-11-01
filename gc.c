#include"gc.h"
#include<stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include<pthread.h>

typedef struct {
  void * ptr;
  
  size_t holder_count;
  void **holders;
} ptr_hold;

bool _gc_init_mutex=false;
pthread_mutex_t lock;

ptr_hold *ptrs;
size_t ptr_count=0;

#if (__SIZEOF_POINTER__==8)
bool cmp_holder(void *ptr,void *holder){
  return ptr==(void *)*(long *)holder;
}
#else
bool cmp_holder(void *ptr,void *holder){
  return ptr==(void *)*(int *)holder;
}
#endif

void destroy(){
  clear();
  pthread_mutex_destroy(&lock);
}
void gc_init(){
  if(!_gc_init_mutex){
    _gc_init_mutex=true;
    int err=pthread_mutex_init(&lock,NULL);
    if(err!=0){
      printf("failed to initialize mutexs (thread syncronizer)");
      exit(1);
    }else{
      atexit(&destroy);
    }
  }
}

void remove_unused_holder(ptr_hold *pth){
  if(pth->holder_count==0){
    return;
  }
  
  size_t used=0;
  void *tmp[pth->holder_count];
  for(size_t i=0; i<pth->holder_count; i++){
    if(cmp_holder(pth->ptr,pth->holders[i])){
      tmp[used]=pth->holders[i];
      used++;
    }
  }
  if(used==0){
    free(pth->holders);
    pth->holder_count=0;
    return;
  }
  
  if(used<(pth->holder_count)){
    free(pth->holders);
    pth->holders=malloc(used*sizeof(void *));
    for(size_t i=0; i<used; i++){
      pth->holders[i]=tmp[i];
    }
    pth->holder_count=used;
  }
}

void _clean(){
  if(ptr_count==0){
    return;
  }
  size_t used=0;
  ptr_hold tmp[ptr_count*sizeof(ptr_hold)];
  for(size_t i=ptr_count; i>0; i--){
    size_t j=i-1;
    remove_unused_holder(&ptrs[j]);
    if(ptrs[j].holder_count>0){
      tmp[used]=ptrs[j];
      used++;
    }else{
      free(ptrs[j].ptr);
    }
  }
  if(used==0){
    free(ptrs);
    ptr_count=0;
    return;
  }
  if(used==ptr_count){
    return;
  }
  free(ptrs);
  ptrs=(ptr_hold *)malloc(used*sizeof(ptr_hold));
  for(size_t i=0; i<used; i++){
    ptrs[i]=tmp[i];
  }
  ptr_count=used;
}

void clean(){
  pthread_mutex_lock(&lock);
  gc_init();
  _clean();
  pthread_mutex_unlock(&lock);
}

void add_holder(ptr_hold *pth,void *ptr){
  remove_unused_holder(pth);
  if(pth->holder_count==0){
    pth->holders=malloc(sizeof(void *));
    pth->holders[0]=ptr;

  }else{
    pth->holders=realloc(pth->holders,(pth->holder_count+1)*sizeof(void *));
    pth->holders[pth->holder_count]=ptr;
  }
  pth->holder_count++;
}

void _clear(){
  for(size_t i=ptr_count; i>0; i--){
    size_t j=i-1;
    ptrs[j].holder_count=0;
    free(ptrs[j].holders);
  }
  _clean();
}

void clear(){
  pthread_mutex_lock(&lock);
  gc_init();
  _clear();
  pthread_mutex_unlock(&lock);
}

void _assign_ptr(void ** const ptr_to_ptr,void * const ptr){
  *ptr_to_ptr=ptr;
  ptr_hold pth={.ptr=ptr};
  add_holder(&pth, ptr_to_ptr);
  bool to_clean=false;
  if(ptr_count==0){
    ptrs=malloc(sizeof(ptr_hold));
    ptrs[0]=pth;
    ptr_count=1;
  }else{
    bool dup=false;
    for(size_t i=ptr_count; i>0; i--){
     size_t j=i-1;
      if(ptrs[j].ptr==ptr){
        dup=true;
        free(pth.holders);
        add_holder(&ptrs[j], ptr_to_ptr);
      }
      remove_unused_holder(&ptrs[j]);
      if(ptrs[j].holder_count==0){
        to_clean=true;
      }
    }
    if(!dup){
      ptr_count++;
      ptrs=realloc(ptrs,sizeof(ptr_hold)*(ptr_count));
      ptrs[ptr_count-1]=pth;
    }
  }
  if(to_clean){
    _clean();
  }
}

void alloc_mem(void ** const ptr_to_ptr,size_t size){
  pthread_mutex_lock(&lock);
  gc_init();
  _assign_ptr(ptr_to_ptr,malloc(size));
  pthread_mutex_unlock(&lock);
}

void assign_ptr(void **const ptr_to_ptr,void *const ptr){
  pthread_mutex_lock(&lock);
  gc_init();
  _assign_ptr(ptr_to_ptr,ptr);
  pthread_mutex_unlock(&lock);
}

void _force_free(void *ptr){
  for(size_t i=0; i<ptr_count; i++){
    if(ptrs[i].ptr==ptr){
      free(ptrs[i].holders);
      ptrs[i].holder_count=0;
      _clean();
      return;
    }
  }
  free(ptr);
}

void force_free(void *ptr){
  pthread_mutex_lock(&lock);
  gc_init();
  _force_free(ptr);
  pthread_mutex_unlock(&lock);
}
