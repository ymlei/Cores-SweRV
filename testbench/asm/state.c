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

/* state machine related stuff */
/* List of all the possible states for the FSM */
typedef enum CORE_STATE {
        CORE_START=0,
        CORE_INVALID,
        CORE_S1,
        CORE_S2,
        CORE_INT,
        CORE_FLOAT,
        CORE_EXPONENT,
        CORE_SCIENTIFIC,
        NUM_CORE_STATES
} core_state_e ;

enum CORE_STATE core_state_transition( ee_u8 **instr , ee_u32 *transition_count);

#if MAIN_HAS_NOARGC
MAIN_RETURN_TYPE main(void) {
        int argc=0;
        char *argv[1];
#else
MAIN_RETURN_TYPE main(int argc, char *argv[]) {
#endif
        ee_printf("hello my");
        void *memblock = (void *)static_memblk;
        ee_u32 size = TOTAL_DATA_SIZE;
        core_init_state(size, 0, memblock);
        core_bench_state(size, (ee_u8)memblock, 0, 0, 1);
        return MAIN_RETURN_VAL;
}


/* Function: core_bench_state
        Benchmark function

        Go over the input twice, once direct, and once after introducing some corruption.
*/
void core_bench_state(ee_u32 blksize, ee_u8 *memblock,
                ee_s16 seed1, ee_s16 seed2, ee_s16 step)
{
        ee_u32 final_counts[NUM_CORE_STATES];
        ee_u32 track_counts[NUM_CORE_STATES];
        ee_u8 *p=memblock;
        ee_u32 i;


#if CORE_DEBUG
        ee_printf("State Bench: %d,%d,%d,%04x\n",seed1,seed2,step);
#endif
        for (i=0; i<NUM_CORE_STATES; i++) {
                final_counts[i]=track_counts[i]=0;
        }
        /* run the state machine over the input */
        while (*p!=0) {
                enum CORE_STATE fstate=core_state_transition(&p,track_counts);
                final_counts[fstate]++;
#if CORE_DEBUG
        ee_printf("%d,",fstate);
        }
        ee_printf("\n");
#else
        }
#endif
        p=memblock;
        while (p < (memblock+blksize)) { /* insert some corruption */
                if (*p!=',')
                        *p^=(ee_u8)seed1;
                p+=step;
        }
        p=memblock;
        /* run the state machine over the input again */
        while (*p!=0) {
                enum CORE_STATE fstate=core_state_transition(&p,track_counts);
                final_counts[fstate]++;
#if CORE_DEBUG
        ee_printf("%d,",fstate);
        }
        ee_printf("\n");
#else
        }
#endif
        p=memblock;
        while (p < (memblock+blksize)) { /* undo corruption is seed1 and seed2 are equal */
                if (*p!=',')
                        *p^=(ee_u8)seed2;
                p+=step;
        }
}

/* Default initialization patterns */
static ee_u8 *intpat[4]  ={(ee_u8 *)"5012",(ee_u8 *)"1234",(ee_u8 *)"-874",(ee_u8 *)"+122"};
static ee_u8 *floatpat[4]={(ee_u8 *)"35.54400",(ee_u8 *)".1234500",(ee_u8 *)"-110.700",(ee_u8 *)"+0.64400"};
static ee_u8 *scipat[4]  ={(ee_u8 *)"5.500e+3",(ee_u8 *)"-.123e-2",(ee_u8 *)"-87e+832",(ee_u8 *)"+0.6e-12"};
static ee_u8 *errpat[4]  ={(ee_u8 *)"T0.3e-1F",(ee_u8 *)"-T.T++Tq",(ee_u8 *)"1T3.4e4z",(ee_u8 *)"34.0e-T^"};

/* Function: core_init_state
        Initialize the input data for the state machine.

        Populate the input with several predetermined strings, interspersed.
        Actual patterns chosen depend on the seed parameter.

        Note:
        The seed parameter MUST be supplied from a source that cannot be determined at compile time
*/
void core_init_state(ee_u32 size, ee_s16 seed, ee_u8 *p) {
        ee_u32 total=0,next=0,i;
        ee_u8 *buf=0;
#if CORE_DEBUG
        ee_u8 *start=p;
        ee_printf("State: %d,%d\n",size,seed);
#endif
        size--;
        next=0;
        while ((total+next+1)<size) {
                if (next>0) {
                        for(i=0;i<next;i++)
                                *(p+total+i)=buf[i];
                        *(p+total+i)=',';
                        total+=next+1;
                }
                seed++;
                switch (seed & 0x7) {
                        case 0: /* int */
                        case 1: /* int */
                        case 2: /* int */
                                buf=intpat[(seed>>3) & 0x3];
                                next=4;
                        break;
                        case 3: /* float */
                        case 4: /* float */
                                buf=floatpat[(seed>>3) & 0x3];
                                next=8;
                        break;
                        case 5: /* scientific */
                        case 6: /* scientific */
                                buf=scipat[(seed>>3) & 0x3];
                                next=8;
                        break;
                        case 7: /* invalid */
                                buf=errpat[(seed>>3) & 0x3];
                                next=8;
                        break;
                        default: /* Never happen, just to make some compilers happy */
                        break;
                }
        }
        size++;
        while (total<size) { /* fill the rest with 0 */
                *(p+total)=0;
                total++;
        }
#if CORE_DEBUG
        ee_printf("State Input: %s\n",start);
#endif
}

static ee_u8 ee_isdigit(ee_u8 c) {
        ee_u8 retval;
        retval = ((c>='0') & (c<='9')) ? 1 : 0;
        return retval;
}

/* Function: core_state_transition
        Actual state machine.

        The state machine will continue scanning until either:
        1 - an invalid input is detcted.
        2 - a valid number has been detected.

        The input pointer is updated to point to the end of the token, and the end state is returned (either specific format determined or invalid).
*/

enum CORE_STATE core_state_transition( ee_u8 **instr , ee_u32 *transition_count) {
        ee_u8 *str=*instr;
        ee_u8 NEXT_SYMBOL;
        enum CORE_STATE state=CORE_START;
        for( ; *str && state != CORE_INVALID; str++ ) {
                NEXT_SYMBOL = *str;
                if (NEXT_SYMBOL==',') /* end of this input */ {
                        str++;
                        break;
                }
                switch(state) {
                case CORE_START:
                        if(ee_isdigit(NEXT_SYMBOL)) {
                                state = CORE_INT;
                        }
                        else if( NEXT_SYMBOL == '+' || NEXT_SYMBOL == '-' ) {
                                state = CORE_S1;
                        }
                        else if( NEXT_SYMBOL == '.' ) {
                                state = CORE_FLOAT;
                        }
                        else {
                                state = CORE_INVALID;
                                transition_count[CORE_INVALID]++;
                        }
                        transition_count[CORE_START]++;
                        break;
                case CORE_S1:
                        if(ee_isdigit(NEXT_SYMBOL)) {
                                state = CORE_INT;
                                transition_count[CORE_S1]++;
                        }
                        else if( NEXT_SYMBOL == '.' ) {
                                state = CORE_FLOAT;
                                transition_count[CORE_S1]++;
                        }
                        else {
                                state = CORE_INVALID;
                                transition_count[CORE_S1]++;
                        }
                        break;
                case CORE_INT:
                        if( NEXT_SYMBOL == '.' ) {
                                state = CORE_FLOAT;
                                transition_count[CORE_INT]++;
                        }
                        else if(!ee_isdigit(NEXT_SYMBOL)) {
                                state = CORE_INVALID;
                                transition_count[CORE_INT]++;
                        }
                        break;
                case CORE_FLOAT:
                        if( NEXT_SYMBOL == 'E' || NEXT_SYMBOL == 'e' ) {
                                state = CORE_S2;
                                transition_count[CORE_FLOAT]++;
                        }
                        else if(!ee_isdigit(NEXT_SYMBOL)) {
                                state = CORE_INVALID;
                                transition_count[CORE_FLOAT]++;
                        }
                        break;
                case CORE_S2:
                        if( NEXT_SYMBOL == '+' || NEXT_SYMBOL == '-' ) {
                                state = CORE_EXPONENT;
                                transition_count[CORE_S2]++;
                        }
                        else {
                                state = CORE_INVALID;
                                transition_count[CORE_S2]++;
                        }
                        break;
                case CORE_EXPONENT:
                        if(ee_isdigit(NEXT_SYMBOL)) {
                                state = CORE_SCIENTIFIC;
                                transition_count[CORE_EXPONENT]++;
                        }
                        else {
                                state = CORE_INVALID;
                                transition_count[CORE_EXPONENT]++;
                        }
                        break;
                case CORE_SCIENTIFIC:
                        if(!ee_isdigit(NEXT_SYMBOL)) {
                                state = CORE_INVALID;
                                transition_count[CORE_INVALID]++;
                        }
                        break;
                default:
                        break;
                }
        }
        *instr=str;
        return state;
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
