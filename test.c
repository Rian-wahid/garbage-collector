#include"gc.h"
#include<assert.h>
#include <stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<time.h>
void *test_fn1(){
  void *ptr; 
  alloc_mem(&ptr,4);
  assert(ptr!=NULL);
  force_free(ptr);
  alloc_mem(&ptr,4);
  *(int *)ptr=1;
  return ptr;
}

void test_fn2(){
  void *ptr;
  void *fn1ret=test_fn1();
  assert(*(int *)fn1ret==1);
  assign_ptr(&ptr,fn1ret);
  assert(ptr!=NULL);
  assert(*(int *)ptr==1);
  force_free(ptr);
}

struct timespec create_time(int nanosec){
  struct timespec tm={.tv_sec=0, .tv_nsec=nanosec};
  return tm;
}

void test(){
  test_fn2();
  struct timespec ts=create_time(1000);
  assert(nanosleep(&ts,&ts)==0);
  ts=create_time(1200);
  void *a;
  for(int i=0; i<200; i++){
    {
      void *b;
      alloc_mem(&b,4);
      assert(b!=NULL);
      *(int *)b=i;
      assign_ptr(&a,b);
      assert(a==b);
    }
    {
      assert(*(int *)a==i);
      void *c;
      assign_ptr(&c,a);
      assert(c==a);
    }
  }
  assert(nanosleep(&ts,&ts)==0);
  ts=create_time(2000);
  int total=200;
  void ***ptrs=malloc(total*sizeof(void *));
  void ***ptrs2=malloc(total*sizeof(void *));
  void ***ptrs3=malloc(total*sizeof(void *));
  for(int i=0; i<total; i++){
    ptrs[i]=malloc(sizeof(void *));
    ptrs2[i]=malloc(sizeof(void *));
    ptrs3[i]=malloc(sizeof(void *));
  }
  assert(nanosleep(&ts,&ts)==0);
  ts=create_time(900);
  for(int i=0; i<total; i++){
    alloc_mem(ptrs[i],4);
    *(int *)*ptrs[i]=i;
    assign_ptr(ptrs2[i],*ptrs[i]);
    *ptrs[i]=NULL;
    assign_ptr(ptrs3[i],*ptrs2[i]);
  }
  assert(nanosleep(&ts,&ts)==0);
  for(int i=0; i<total; i++){
    assert(*(int *)*ptrs2[i]==i);
    assert(*(int *)*ptrs3[i]==i);
    free(ptrs[i]);
    free(ptrs2[i]);
    free(ptrs3[i]);
  }
  free(ptrs);
  free(ptrs2);
  free(ptrs3);

}

void *thread_test(void *argp){
  test();
  return argp;
}
int main(){
  gc_init();
  pthread_t thrd[2];
  int err=pthread_create(&thrd[0],NULL,thread_test,NULL);
  err+=pthread_create(&thrd[1],NULL,thread_test,NULL);
  if(err!=0){
    perror("failed to create thread");
    exit(1);
  }
  pthread_join(thrd[0],NULL);
  pthread_join(thrd[1],NULL);
  return 0;
}
