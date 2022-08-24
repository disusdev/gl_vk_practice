#include <defines.h>

// #define CVEC_STDLIB
// #define CVEC_IMPLEMENTATION
// #include <containers/cvec.h>

// #define CQUE_STDLIB
// #define CQUE_IMPLEMENTATION
// #include <containers/cque.h>

#define CLST_STDLIB
#define CLST_IMPLEMENTATION
#include <containers/clst.h>
#include <containers/cset.h>

#include <stdio.h>

// void cvec_test();
// void cque_test();
void clst_test();

int main()
{
  //cvec_test();

  //cque_test();

  clst_test();
  
  return(0);
}

// void cvec_test()
// {
//   printf("+ CVec Start.\n");
//   cvec(u64) uvec = cvec_create(u64);

//   u64 num = 11;
//   cvec_push(uvec, num);
//   num = 22;
//   cvec_push(uvec, num);
//   cvec_push(uvec, 33llu);

//   for (u64 i = 0; i < cvec_size(uvec); i++)
//   {
//     printf("uvec[%lld] = %lld\n", i, uvec[i]);
//   }

//   while (cvec_size(uvec) > 1)
//   {
//     cvec_pop(uvec, 0);
//   }

//   cvec_pop(uvec, &num);
//   printf("Element value: %lld\n", num);

//   printf("Size: %d\n", cvec_size(uvec));
//   printf("Capacity: %d\n", cvec_capacity(uvec));
//   printf("Stride: %d\n", cvec_stride(uvec));

//   printf("+ End.\n");   
// }

// void cque_test()
// {
//   printf("+ CQue Start.\n");
//   cque(u64) uque = cque_create(u64);

//   u64 num = 11;
//   cque_push(uque, num);
//   num = 22;
//   cque_push(uque, num);
//   cque_push(uque, 33llu);
//   cque_push(uque, 44llu);
//   cque_push(uque, 444llu);

//   for (u64 i = 0; i < cque_capacity(uque); i++)
//   {
//     printf("uque[%lld] = %lld\n", i, uque[i]);
//   }

//   while (cque_size(uque) != 1)
//   {
//     cque_pop(uque, 0);
//   }

//   cque_push(uque, 5llu);
//   cque_push(uque, 55llu);
//   cque_push(uque, 555llu);
//   cque_push(uque, 5555llu);

//   cque_pop(uque, &num);
//   printf("Element value: %lld\n", num);

//   for (u64 i = 0; i < cque_capacity(uque); i++)
//   {
//     printf("uque[%lld] = %lld\n", i, uque[i]);
//   }

//   printf("Size: %d\n", cque_size(uque));
//   printf("Capacity: %d\n", cque_capacity(uque));
//   printf("Stride: %d\n", cque_stride(uque));
//   printf("Offset: %d\n", cque_offset(uque));

//   printf("+ End.\n");
// }

typedef struct
some_data
{
  u64 data1;
  u64 data2;
  u64 data3;
  u64 data4;
}
some_data;

b8 cmp_some_data(some_data d1, some_data d2)
{
  return d1.data1 == d2.data1 &&
         d1.data2 == d2.data2 &&
         d1.data3 == d2.data3 &&
         d1.data4 == d2.data4;
}

void clst_test()
{
  printf("+ CLst Start.\n");

  t_clst sdlst = clst_create(some_data);
  t_cset sdset = cset_create(some_data);

  some_data sd;
  sd.data1 = 1;
  sd.data2 = 2;
  sd.data3 = 3;
  sd.data4 = 4;
  clst_push(&sdlst, sd);
  cset_push(&sdset, sd);
  sd.data1 = 11;
  sd.data2 = 22;
  sd.data3 = 33;
  sd.data4 = 44;
  clst_push(&sdlst, sd);
  cset_push(&sdset, sd);
  sd.data1 = 111;
  sd.data2 = 222;
  sd.data3 = 333;
  sd.data4 = 444;
  clst_push(&sdlst, sd);
  cset_push(&sdset, sd);
  sd.data1 = 111;
  sd.data2 = 222;
  sd.data3 = 333;
  sd.data4 = 444;
  clst_push(&sdlst, sd);
  cset_push(&sdset, sd);

  some_data sd2;
  sd2.data1 = 111;
  sd2.data2 = 222;
  sd2.data3 = 333;
  sd2.data4 = 444;

  printf("sd == sd2: %d\n", cmp_some_data(sd, sd2));
  printf("find sd2: %d\n", clst_find(&sdlst, &sd2) != sdlst.end);

  printf("[lst]---------------------\n");
  for (t_clst_node* it = sdlst.begin; it != sdlst.end; it = it->next)
  {
    some_data* d = (some_data*)it->data;
    printf("Data1: %d\n", d->data1);
    printf("Data2: %d\n", d->data2);
    printf("Data3: %d\n", d->data3);
    printf("Data4: %d\n", d->data4);
    printf("---------------------\n");
  }
  clst_destroy(&sdlst);

  printf("[set]---------------------\n");
  for (t_cset_node* it = sdset.begin; it != sdset.end; it = it->next)
  {
    some_data* d = (some_data*)it->data;
    printf("Data1: %d\n", d->data1);
    printf("Data2: %d\n", d->data2);
    printf("Data3: %d\n", d->data3);
    printf("Data4: %d\n", d->data4);
    printf("---------------------\n");
  }
  cset_destroy(&sdset);


  printf("+ CLst End.\n");
}