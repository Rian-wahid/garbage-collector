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
pthread_mutex_t rm_unused_holder_lock;
pthread_mutex_t clean_lock;
pthread_mutex_t clear_lock;
pthread_mutex_t assign_ptr_lock;
pthread_mutex_t force_free_lock;
pthread_mutex_t add_holder_lock;
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
  pthread_mutex_destroy(&clean_lock);
  pthread_mutex_destroy(&rm_unused_holder_lock);
  pthread_mutex_destroy(&assign_ptr_lock);
  pthread_mutex_destroy(&force_free_lock);
  pthread_mutex_destroy(&clear_lock);
  pthread_mutex_destroy(&add_holder_lock);
}
void gc_init(){
  if(!_gc_init_mutex){
    _gc_init_mutex=true;
    int err=pthread_mutex_init(&clean_lock,NULL);
    err+=pthread_mutex_init(&rm_unused_holder_lock,NULL);
    err+=pthread_mutex_init(&assign_ptr_lock,NULL);
    err+=pthread_mutex_init(&force_free_lock,NULL);
    err+=pthread_mutex_init(&clear_lock,NULL);
    err+=pthread_mutex_init(&add_holder_lock,NULL);
    if(err!=0){
      perror("failed to initialize mutexs (thread syncronizer)");
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
  pthread_mutex_lock(&rm_unused_holder_lock);
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
    pthread_mutex_unlock(&rm_unused_holder_lock);
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
  pthread_mutex_unlock(&rm_unused_holder_lock);
}

void clean(){
  gc_init();
  if(ptr_count==0){
    return;
  }
  pthread_mutex_lock(&clean_lock);
  size_t used=0;
  ptr_hold tmp[ptr_count*sizeof(ptr_hold)];
  for(size_t i=0; i<ptr_count; i++){
    remove_unused_holder(&ptrs[i]);
    if(ptrs[i].holder_count>0){
      tmp[used]=ptrs[i];
      used++;
    }else{
      free(ptrs[i].ptr);
    }
  }
  if(used==0){
    free(ptrs);
    ptr_count=0;
    pthread_mutex_unlock(&clean_lock);
    return;
  }
  if(used==ptr_count){
    pthread_mutex_unlock(&clean_lock);
    return;
  }
  free(ptrs);
  ptrs=(ptr_hold *)malloc(used*sizeof(ptr_hold));
  for(size_t i=0; i<used; i++){
    ptrs[i]=tmp[i];
  }
  ptr_count=used;
  pthread_mutex_unlock(&clean_lock);
}

void add_holder(ptr_hold *pth,void *ptr){
  pthread_mutex_lock(&add_holder_lock);
  remove_unused_holder(pth);
  if(pth->holder_count==0){
    pth->holders=malloc(sizeof(void *));
    pth->holders[0]=ptr;

  }else{
    
    pth->holders=realloc(pth->holders,(pth->holder_count+1)*sizeof(void *));
    pth->holders[pth->holder_count]=ptr;
  }
  pth->holder_count++;
  pthread_mutex_unlock(&add_holder_lock);
}

void clear(){
  gc_init();
  pthread_mutex_lock(&clear_lock);
  for(size_t i=0; i<ptr_count; i++){
    ptrs[i].holder_count=0;
    free(ptrs[i].holders);
  }
  clean();
  pthread_mutex_unlock(&clear_lock);
}

void assign_ptr(void ** const ptr_to_ptr,void * const ptr){
  gc_init();
  pthread_mutex_lock(&assign_ptr_lock);
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
    for(size_t i=0; i<ptr_count; i++){
     
      if(ptrs[i].ptr==ptr){
        dup=true;
        free(pth.holders);
        add_holder(&ptrs[i], ptr_to_ptr);
      }
      remove_unused_holder(&ptrs[i]);
      if(ptrs[i].holder_count==0){
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
    clean();
  }
  pthread_mutex_unlock(&assign_ptr_lock);
}

void alloc_mem(void ** const ptr_to_ptr,size_t size){
  assign_ptr(ptr_to_ptr,malloc(size));
}

void force_free(void *ptr){
  gc_init();
  pthread_mutex_lock(&force_free_lock);
  for(size_t i=0; i<ptr_count; i++){
    if(ptrs[i].ptr==ptr){
      free(ptrs[i].holders);
      ptrs[i].holder_count=0;
      clean();
      pthread_mutex_unlock(&force_free_lock);
      return;
    }
  }
  free(ptr);
  pthread_mutex_unlock(&force_free_lock);
}
