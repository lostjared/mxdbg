#ifndef __INT_STACK_H_
#define __INT_STACK_H_

#ifdef __cplusplus
extern "C" {
#endif
    extern void init_stack();
    extern void push_stack(long value);
    extern long pop_stack();
    extern void print_stack();
#ifdef __cplusplus
}
#endif
#endif