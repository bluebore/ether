
#ifndef  __CS_ATOMIC_H_
#define  __CS_ATOMIC_H_

namespace bfs {

/**
 * @brief ԭ�Ӽ�,����ԭֵ
 *
 * @param [in/out] mem ԭ�ӱ���
 * @param [in] add		: ����
 * @return  inline int
 * @author yanshiguang02
 * @date 2012/09/09 13:55:38
**/
static inline int atomic_add(volatile int *mem, int add)
{
    asm volatile(
            "lock xadd %0, (%1);"
            : "=a"(add)
            : "r"(mem), "a"(add)
            : "memory"
    );
    return add;
}
static inline long atomic_add64(volatile long* mem, long add)
{
    asm volatile (
            "lock xaddq %0, (%1)"
            : "=a" (add)
            : "r" (mem), "a" (add)
            : "memory"
    );  
    return add;
}

/**
 * @brief ԭ������
 *
 * @param [in/out] mem   : volatile int*
 * @return  inline void
 * @author yanshiguang02
 * @date 2012/09/09 13:56:46
**/
static inline void atomic_inc(volatile int *mem)
{
    asm volatile(
            "lock incl %0;"
            : "=m"(*mem)
            : "m"(*mem)
    );
}
static inline void atomic_inc64(volatile long *mem)
{
    asm volatile(
            "lock incq %0;"
            : "=m"(*mem)
            : "m"(*mem)
    );
}

/**
 * @brief ԭ���Լ�
 *
 * @param [in/out] mem   : volatile int*
 * @return  inline void
 * @author yanshiguang02
 * @date 2012/09/09 13:57:54
**/
static inline void atomic_dec(volatile int *mem)
{
    asm volatile(
            "lock decl %0;"
            : "=m"(*mem)
            : "m"(*mem)
    );
}
static inline void atomic_dec64(volatile long *mem)
{
    asm volatile(
            "lock decq %0;"
            : "=m"(*mem)
            : "m"(*mem)
    );
}

/**
 * @brief swap
 *
 * @param [in/out] lockword   : volatile void*
 * @param [in/out] value   : int
 * @return  inline int
 * @author yanshiguang02
 * @date 2012/09/09 13:55:25
**/
static inline int atomic_swap(volatile void *lockword, int value)
{
    asm volatile(
            "lock xchg %0, (%1);"
            : "=a"(value)
            : "r"(lockword), "a"(value)
            : "memory"
    );
    return value;
}
static inline int atomic_swap64(volatile void *lockword, long value)
{
    asm volatile(
            "lock xchg %0, (%1);"
            : "=a"(value)
            : "r"(lockword), "a"(value)
            : "memory"
    );
    return value;
}


/**
 * @brief if set
    if(*mem == cmp)
        *mem = xchg;
    else
        cmp = *mem;
    return cmp;
 *
 * @param [in/out] mem   : volatile void*
 * @param [in/out] xchg   : int
 * @param [in/out] cmp   : int
 * @return  inline int
 * @author yanshiguang02
 * @date 2012/09/09 13:54:54
**/
static inline int atomic_comp_swap(volatile void *mem, int xchg, int cmp)
{
    asm volatile(
            "lock cmpxchg %1, (%2)"
            :"=a"(cmp)
            :"d"(xchg), "r"(mem), "a"(cmp)
    );
    return cmp;
}

/**
 * @brief 64λ if set
 *
 * @param [in/out] mem   : volatile void*
 * @param [in/out] xchg   : long long
 * @param [in/out] cmp   : long long
 * @return  inline int
 * @author yanshiguang02
 * @date 2012/09/09 13:54:15
**/
static inline int atomic_comp_swap64(volatile void *mem, long long xchg, long long cmp)
{
    asm volatile(
            "lock cmpxchg %1, (%2)"
            :"=a"(cmp)
            :"d"(xchg), "r"(mem), "a"(cmp)
    );
    return cmp;
}

} // namespace bfs

#endif  //__CS_ATOMIC_H_

/* vim: set ts=4 sw=4 sts=4 tw=100: */
