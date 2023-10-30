#include"gc.h"
#include<assert.h>
#include<stdlib.h>
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

void test(){
  test_fn2();
  void *a;
  for(int i=0; i<100000; i++){
    {
      void *b;
      alloc_mem(&b,200000);
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
}

int main(){
  atexit(&clear);
  test();
  return 0;
}
