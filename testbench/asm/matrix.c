#include "defines.h"

extern int STACK;
void main();

#define STDOUT 0xd0580000


__asm (".section .text");
__asm (".global _start");
__asm ("_start:");

// Enable Caches in MRAC
__asm ("li t0, 0x5f555555");
__asm ("csrw 0x7c0, t0");

// Set stack pointer.
__asm ("la sp, STACK");

__asm ("jal main");

// Write 0xff to STDOUT for TB to termiate test.
__asm (".global _finish");
__asm ("_finish:");
__asm ("li t0, 0xd0580000");
__asm ("addi t1, zero, 0xff");
__asm ("sb t1, 0(t0)");
__asm ("beq x0, x0, _finish");
__asm (".rept 10");
__asm ("nop");
__asm (".endr");

/* Configuration: TOTAL_DATA_SIZE
        Define total size for data algorithms will operate on
*/
#ifndef TOTAL_DATA_SIZE
#define TOTAL_DATA_SIZE 2*1000
#endif

/* Configuration : HAS_PRINTF
        Define to 1 if the platform has stdio.h and implements the printf function.
*/
#ifndef HAS_PRINTF
#define HAS_PRINTF 1
int whisperPrintf(const char* format, ...);
#define ee_printf whisperPrintf
#endif

#ifndef MEM_LOCATION
// #define MEM_LOCATION "STACK"
 #define MEM_LOCATION "STATIC"
#endif

/* Data Types :
        To avoid compiler issues, define the data types that need ot be used for 8b, 16b and 32b in <core_portme.h>.

        *Imprtant* :
        ee_ptr_int needs to be the data type used to hold pointers, otherwise coremark may fail!!!
*/
typedef signed short ee_s16;
typedef unsigned short ee_u16;
typedef signed int ee_s32;
typedef double ee_f32;
typedef unsigned char ee_u8;
typedef unsigned int ee_u32;
typedef ee_u32 ee_ptr_int;

/* align_mem :
        This macro is used to align an offset to point to a 32b value. It is used in the Matrix algorithm to initialize the input memory blocks.
*/
#define align_mem(x) (void *)(4 + (((ee_ptr_int)(x) - 1) & ~3))

/* Configuration : SEED_METHOD
        Defines method to get seed values that cannot be computed at compile time.

        Valid values :
        SEED_ARG - from command line.
        SEED_FUNC - from a system function.
        SEED_VOLATILE - from volatile variables.
*/

/* Configuration : MAIN_HAS_NOARGC
        Needed if platform does not support getting arguments to main.

        Valid values :
        0 - argc/argv to main is supported
        1 - argc/argv to main is not supported

        Note :
        This flag only matters if MULTITHREAD has been defined to a value greater then 1.
*/
#ifndef MAIN_HAS_NOARGC
#define MAIN_HAS_NOARGC 1
#endif

/* Configuration : MAIN_HAS_NORETURN
        Needed if platform does not support returning a value from main.

        Valid values :
        0 - main returns an int, and return value will be 0.
        1 - platform does not support returning a value from main
*/
#ifndef MAIN_HAS_NORETURN
#define MAIN_HAS_NORETURN 1
#endif

/* Variable : default_num_contexts
        Not used for this simple port, must cintain the value 1.
*/

#if !defined(PROFILE_RUN) && !defined(PERFORMANCE_RUN) && !defined(VALIDATION_RUN)
#if (TOTAL_DATA_SIZE==1200)
#define PROFILE_RUN 1
#elif (TOTAL_DATA_SIZE==2000)
#define PERFORMANCE_RUN 1
#else
#define VALIDATION_RUN 1
#endif
#endif


#if HAS_STDIO
#include <stdio.h>
#endif
#if HAS_PRINTF
#ifndef ee_printf
#define ee_printf printf
#endif
#endif


#if MAIN_HAS_NORETURN
#define MAIN_RETURN_VAL
#define MAIN_RETURN_TYPE void
#else
#define MAIN_RETURN_VAL 0
#define MAIN_RETURN_TYPE int
#endif

typedef signed short ee_s16;
typedef unsigned short ee_u16;
typedef signed int ee_s32;
typedef double ee_f32;
typedef unsigned char ee_u8;
typedef unsigned int ee_u32;
typedef ee_u32 ee_ptr_int;

ee_u8 static_memblk[TOTAL_DATA_SIZE];

typedef ee_s16 MATDAT;
typedef ee_s32 MATRES;

typedef struct MAT_PARAMS_S {
        int N;
        MATDAT *A;
        MATDAT *B;
        MATRES *C;
} mat_params;

void matrix_test(ee_u32 N, MATRES *C, MATDAT *A, MATDAT *B, MATDAT val);
ee_s16 matrix_sum(ee_u32 N, MATRES *C, MATDAT clipval);
void matrix_mul_const(ee_u32 N, MATRES *C, MATDAT *A, MATDAT val);
void matrix_mul_vect(ee_u32 N, MATRES *C, MATDAT *A, MATDAT *B);
void matrix_mul_matrix(ee_u32 N, MATRES *C, MATDAT *A, MATDAT *B);
void matrix_mul_matrix_bitextract(ee_u32 N, MATRES *C, MATDAT *A, MATDAT *B);
void matrix_add_const(ee_u32 N, MATDAT *A, MATDAT val);

#if MAIN_HAS_NOARGC
MAIN_RETURN_TYPE main(void) {
        int argc=0;
        char *argv[1];
#else
MAIN_RETURN_TYPE main(int argc, char *argv[]) {
#endif
        ee_printf("hello my");
        void *memblock = (void *)static_memblk;
        mat_params *mat;
        ee_u32 size = TOTAL_DATA_SIZE;
        core_init_matrix(size, memblock, mat);
        core_bench_matrix(mat, (ee_s16)5);
        return MAIN_RETURN_VAL;
}

/*
Topic: Description
        Matrix manipulation benchmark

        This very simple algorithm forms the basis of many more complex algorithms.

        The tight inner loop is the focus of many optimizations (compiler as well as hardware based)
        and is thus relevant for embedded processing.

        The total available data space will be divided to 3 parts:
        NxN Matrix A - initialized with small values (upper 3/4 of the bits all zero).
        NxN Matrix B - initialized with medium values (upper half of the bits all zero).
        NxN Matrix C - used for the result.

        The actual values for A and B must be derived based on input that is not available at compile time.
*/


#define matrix_test_next(x) (x+1)
#define matrix_clip(x,y) ((y) ? (x) & 0x0ff : (x) & 0x0ffff)
#define matrix_big(x) (0xf000 | (x))
#define bit_extract(x,from,to) (((x)>>(from)) & (~(0xffffffff << (to))))

#if CORE_DEBUG
void printmat(MATDAT *A, ee_u32 N, char *name) {
        ee_u32 i,j;
        ee_printf("Matrix %s [%dx%d]:\n",name,N,N);
        for (i=0; i<N; i++) {
                for (j=0; j<N; j++) {
                        if (j!=0)
                                ee_printf(",");
                        ee_printf("%d",A[i*N+j]);
                }
                ee_printf("\n");
        }
}
void printmatC(MATRES *C, ee_u32 N, char *name) {
        ee_u32 i,j;
        ee_printf("Matrix %s [%dx%d]:\n",name,N,N);
        for (i=0; i<N; i++) {
                for (j=0; j<N; j++) {
                        if (j!=0)
                                ee_printf(",");
                        ee_printf("%d",C[i*N+j]);
                }
                ee_printf("\n");
        }
}
#endif
/* Function: core_bench_matrix
        Benchmark function

        Iterate <matrix_test> N times,
        changing the matrix values slightly by a constant amount each time.
*/
void core_bench_matrix(mat_params *p, ee_s16 seed) {
        ee_u32 N=p->N;
        MATRES *C=p->C;
        MATDAT *A=p->A;
        MATDAT *B=p->B;
        MATDAT val=(MATDAT)seed;

        matrix_test(N,C,A,B,val);
}

/* Function: matrix_test
        Perform matrix manipulation.

        Parameters:
        N - Dimensions of the matrix.
        C - memory for result matrix.
        A - input matrix
        B - operator matrix (not changed during operations)

        Returns:
        A CRC value that captures all results calculated in the function.
        In particular, crc of the value calculated on the result matrix
        after each step by <matrix_sum>.

        Operation:

        1 - Add a constant value to all elements of a matrix.
        2 - Multiply a matrix by a constant.
        3 - Multiply a matrix by a vector.
        4 - Multiply a matrix by a matrix.
        5 - Add a constant value to all elements of a matrix.

        After the last step, matrix A is back to original contents.
*/
void matrix_test(ee_u32 N, MATRES *C, MATDAT *A, MATDAT *B, MATDAT val) {
        MATDAT clipval=matrix_big(val);

        matrix_add_const(N,A,val); /* make sure data changes  */
#if CORE_DEBUG
        printmat(A,N,"matrix_add_const");
#endif
        matrix_mul_const(N,C,A,val);
        matrix_sum(N,C,clipval);
#if CORE_DEBUG
        printmatC(C,N,"matrix_mul_const");
#endif
        matrix_mul_vect(N,C,A,B);
        matrix_sum(N,C,clipval);
#if CORE_DEBUG
        printmatC(C,N,"matrix_mul_vect");
#endif
        matrix_mul_matrix(N,C,A,B);
        matrix_sum(N,C,clipval);
#if CORE_DEBUG
        printmatC(C,N,"matrix_mul_matrix");
#endif
        matrix_mul_matrix_bitextract(N,C,A,B);
        matrix_sum(N,C,clipval);
#if CORE_DEBUG
        printmatC(C,N,"matrix_mul_matrix_bitextract");
#endif

        matrix_add_const(N,A,-val); /* return matrix to initial value */
}

/* Function : matrix_init
        Initialize the memory block for matrix benchmarking.

        Parameters:
        blksize - Size of memory to be initialized.
        memblk - Pointer to memory block.
        seed - Actual values chosen depend on the seed parameter.
        p - pointers to <mat_params> containing initialized matrixes.

        Returns:
        Matrix dimensions.

        Note:
        The seed parameter MUST be supplied from a source that cannot be determined at compile time
*/
void core_init_matrix(ee_u32 blksize, void *memblk, mat_params *p) {
        ee_u32 N=0;
        MATDAT *A;
        MATDAT *B;
        ee_s32 order=1;
        MATDAT val;
        ee_u32 i=0,j=0;
        ee_s32 seed=1;
        while (j<blksize) {
                i++;
                j=i*i*2*4;
        }
        N=i-1;
        A=(MATDAT *)align_mem(memblk);
        B=A+N*N;

        for (i=0; i<N; i++) {
                for (j=0; j<N; j++) {
                        seed = ( ( order * seed ) % 65536 );
                        val = (seed + order);
                        val=matrix_clip(val,0);
                        B[i*N+j] = val;
                        val =  (val + order);
                        val=matrix_clip(val,1);
                        A[i*N+j] = val;
                        order++;
                }
        }

        p->A=A;
        p->B=B;
        p->C=(MATRES *)align_mem(B+N*N);
        p->N=N;
#if CORE_DEBUG
        printmat(A,N,"A");
        printmat(B,N,"B");
#endif
}


/* Function: matrix_sum
        Calculate a function that depends on the values of elements in the matrix.

        For each element, accumulate into a temporary variable.

        As long as this value is under the parameter clipval,
        add 1 to the result if the element is bigger then the previous.

        Otherwise, reset the accumulator and add 10 to the result.
*/
ee_s16 matrix_sum(ee_u32 N, MATRES *C, MATDAT clipval) {
        MATRES tmp=0,prev=0,cur=0;
        ee_s16 ret=0;
        ee_u32 i,j;
        for (i=0; i<N; i++) {
                for (j=0; j<N; j++) {
                        cur=C[i*N+j];
                        tmp+=cur;
                        if (tmp>clipval) {
                                ret+=10;
                                tmp=0;
                        } else {
                                ret += (cur>prev) ? 1 : 0;
                        }
                        prev=cur;
                }
        }
        return ret;
}

/* Function: matrix_mul_const
        Multiply a matrix by a constant.
        This could be used as a scaler for instance.
*/
void matrix_mul_const(ee_u32 N, MATRES *C, MATDAT *A, MATDAT val) {
        ee_u32 i,j;
        for (i=0; i<N; i++) {
                for (j=0; j<N; j++) {
                        C[i*N+j]=(MATRES)A[i*N+j] * (MATRES)val;
                }
        }
}

/* Function: matrix_add_const
        Add a constant value to all elements of a matrix.
*/
void matrix_add_const(ee_u32 N, MATDAT *A, MATDAT val) {
        ee_u32 i,j;
        for (i=0; i<N; i++) {
                for (j=0; j<N; j++) {
                        A[i*N+j] += val;
                }
        }
}

/* Function: matrix_mul_vect
        Multiply a matrix by a vector.
        This is common in many simple filters (e.g. fir where a vector of coefficients is applied to the matrix.)
*/
void matrix_mul_vect(ee_u32 N, MATRES *C, MATDAT *A, MATDAT *B) {
        ee_u32 i,j;
        for (i=0; i<N; i++) {
                C[i]=0;
                for (j=0; j<N; j++) {
                        C[i]+=(MATRES)A[i*N+j] * (MATRES)B[j];
                }
        }
}

/* Function: matrix_mul_matrix
        Multiply a matrix by a matrix.
        Basic code is used in many algorithms, mostly with minor changes such as scaling.
*/
void matrix_mul_matrix(ee_u32 N, MATRES *C, MATDAT *A, MATDAT *B) {
        ee_u32 i,j,k;
        for (i=0; i<N; i++) {
                for (j=0; j<N; j++) {
                        C[i*N+j]=0;
                        for(k=0;k<N;k++)
                        {
                                C[i*N+j]+=(MATRES)A[i*N+k] * (MATRES)B[k*N+j];
                        }
                }
        }
}

/* Function: matrix_mul_matrix_bitextract
        Multiply a matrix by a matrix, and extract some bits from the result.
        Basic code is used in many algorithms, mostly with minor changes such as scaling.
*/
void matrix_mul_matrix_bitextract(ee_u32 N, MATRES *C, MATDAT *A, MATDAT *B) {
        ee_u32 i,j,k;
        for (i=0; i<N; i++) {
                for (j=0; j<N; j++) {
                        C[i*N+j]=0;
                        for(k=0;k<N;k++)
                        {
                                MATRES tmp=(MATRES)A[i*N+k] * (MATRES)B[k*N+j];
                                C[i*N+j]+=bit_extract(tmp,2,4)*bit_extract(tmp,5,7);
                        }
                }
        }
}

#include <stdarg.h>
#include <stdlib.h>


// Special address. Writing (store byte instruction) to this address
// causes the simulator to write to the console.
volatile char __whisper_console_io = 0;


static int
whisperPutc(char c)
{
//  __whisper_console_io = c;
//  __whisper_console_io = c;
  *(volatile char*)(STDOUT) = c;
  return c;
}


static int
whisperPuts(const char* s)
{
  while (*s)
    whisperPutc(*s++);
  return 1;
}


static int
whisperPrintDecimal(int value)
{
  char buffer[20];
  int charCount = 0;

  unsigned neg = value < 0;
  if (neg)
    {
      value = -value;
      whisperPutc('-');
    }

  do
    {
      char c = '0' + (value % 10);
      value = value / 10;
      buffer[charCount++] = c;
    }
  while (value);

  char* p = buffer + charCount - 1;
  for (unsigned i = 0; i < charCount; ++i)
    whisperPutc(*p--);

  if (neg)
    charCount++;

  return charCount;
}


static int
whisperPrintInt(int value, int base)
{
  if (base == 10)
    return whisperPrintDecimal(value);

  char buffer[20];
  int charCount = 0;

  unsigned uu = value;

  if (base == 8)
    {
      do
        {
          char c = '0' + (uu & 7);
          buffer[charCount++] = c;
          uu >>= 3;
        }
      while (uu);
    }
  else if (base == 16)
    {
      do
        {
          int digit = uu & 0xf;
          char c = digit < 10 ? '0' + digit : 'a' + digit;
          buffer[charCount++] = c;
          uu >>= 4;
        }
      while (uu);
    }
  else
    return -1;

  char* p = buffer + charCount - 1;
  for (unsigned i = 0; i < charCount; ++i)
    whisperPutc(*p--);

  return charCount;
}


int
whisperPrintfImpl(const char* format, va_list ap)
{
  int count = 0;  // Printed character count

  for (const char* fp = format; *fp; fp++)
    {
      if (*fp != '%')
        {
          whisperPutc(*fp);
          ++count;
          continue;
        }

      ++fp;  // Skip %

      if (*fp == 0)
        break;

      if (*fp == '%')
        {
          whisperPutc('%');
          continue;
        }

      if (*fp == '-')
        {
          fp++;  // Pad right not yet implemented.
        }

      while (*fp == '0')
        {
          fp++;  // Pad zero not yet implented.
        }

      if (*fp == '*')
        {
          int width = va_arg(ap, int);
          fp++;  // Width not yet implemented.
        }
      else
        {
          while (*fp >= '0' && *fp <= '9')
            ++fp;   // Width not yet implemented.
        }

      switch (*fp)
        {
        case 'd':
          count += whisperPrintDecimal(va_arg(ap, int));
          break;

        case 'u':
          count += whisperPrintDecimal((unsigned) va_arg(ap, unsigned));
          break;

        case 'x':
        case 'X':
          count += whisperPrintInt(va_arg(ap, int), 16);
          break;

        case 'o':
          count += whisperPrintInt(va_arg(ap, int), 8);
          break;

        case 'c':
          whisperPutc(va_arg(ap, int));
          ++count;
          break;

        case 's':
          count += whisperPuts(va_arg(ap, char*));
          break;
        }
    }

  return count;
}


int
whisperPrintf(const char* format, ...)
{
  va_list ap;

  va_start(ap, format);
  int code = whisperPrintfImpl(format, ap);
  va_end(ap);

  return code;
}


int
printf(const char* format, ...)
{
  va_list ap;

  va_start(ap, format);
  int code = whisperPrintfImpl(format, ap);
  va_end(ap);

  return code;
}

void* memset(void* s, int c, size_t n)
{
  asm("mv t0, a0");
  asm("add a2, a2, a0");  // end = s + n
  asm(".memset_loop: bge a0, a2, .memset_end");
  asm("sb a1, 0(a0)");
  asm("addi a0, a0, 1");
  asm("j .memset_loop");
  asm(".memset_end:");
  asm("mv a0, t0");
  asm("jr ra");
}
