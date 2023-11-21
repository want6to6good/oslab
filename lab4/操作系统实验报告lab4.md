## 操作系统实验报告 lab4

### 练习1:分配并初始化一个进程控制块（需要编码）

   - 代码如下：
   ```
    static struct proc_struct *
    alloc_proc(void) {
       struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));
       if (proc != NULL) {
        proc->state = PROC_UNINIT;
        proc->pid = -1;                     //-1表示进程无效
        proc->runs = 0;                     //运行次数
        proc->kstack = 0;
        proc->need_resched = 0;             //是否需要调度器调用
        proc->parent = NULL;                // 里面保存了进程的父进程的指针。
        proc->mm = NULL;                        // 这里面保存了内存管理的信息，包括内存映射，虚存管理等内容。
        memset(&(proc->context), 0, sizeof(struct context));// 保存了进程执行的上下文，也就是几个关键的寄存器的值。
        proc->tf = NULL;                        // 保存了进程的中断帧。进程的执行状态被保存在了中断帧中。我们可以通过调整中断帧来使得系统调用返回特定的值。
        proc->cr3 = boot_cr3;                   // CR3reg: 保存PDT基址。页表基址所在的位置。
        proc->flags = 0;                        // 进程标志位
        memset(proc->name, 0, PROC_NAME_LEN);   //进程名
    }
    return proc;
    }

   ```

   
   - struct context context:
     用于保存进程的上下文信息的结构体，用于进程切换。主要保存了前一个进程的现场(各个寄存器的状态)。在uCore中，所有的进程在内核中也是相对独立的。使用context保存寄存器的目的就在于在内核态中能够进行上下文之间的切换，保存当前进程的执行状态，并加载下一个进程的执行状态。在本实验中，这个成员变量被用于保存进程的上下文信息，包括寄存器的状态。
   
   - struct trapframe *tf:
     一个指向中断帧的指针，总是指向内核栈的某个位置:当进程从用户空间跳到内核空间时，中断帧记录了进程在被中断前的状态。当内核需要跳回用户空间时，需要调整中断帧以恢复让进程继续执行的各寄存器值。在本实验中，这个指针被用于指向进程的中断帧，即保存中断发生时的当前执行状态。
   
### 练习2:为新创建的内核线程分配资源（需要编码）

- 代码如下：
   ```
    int
    do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf) {
    int ret = -E_NO_FREE_PROC;
    struct proc_struct *proc;
    if (nr_process >= MAX_PROCESS) {
        goto fork_out;
    }
    ret = -E_NO_MEM;
    proc = alloc_proc();
    if (proc == NULL) {
        goto fork_out;
    }
    proc->parent = current;
    if (setup_kstack(proc) != 0) {
        goto bad_fork_cleanup_kstack;
    }
    if (copy_mm(clone_flags, proc) != 0) {
        goto bad_fork_cleanup_proc;
    }
    copy_thread(proc, stack, tf);
    bool intr_flag;
    local_intr_save(intr_flag);
    proc->pid = get_pid();
    hash_proc(proc);
    list_add(&proc_list, &(proc->list_link));
    nr_process++;
    local_intr_restore(intr_flag);
    wakeup_proc(proc);
    ret = proc->pid;
    fork_out:
    return ret;
    bad_fork_cleanup_kstack:
    put_kstack(proc);
    bad_fork_cleanup_proc:
    kfree(proc);
    goto fork_out;
    }

   ```

- 在get_pid(void)函数中实现过程如下：
  1.(last_pid作为一个静态变量记录上一个分配的pid。
  2.当get_pid函数被调用时，首先检查是否last_pid超过了MAX_PID。如果超过了，将1ast_pid重新设置为1，从头开始分配。
  3.在循环中，遍历进程列表，检查是否有其他进程已经使用了当前的last_pid。如果发现有其他进程使用了相同的pid，就将1ast_pid递增。
  4.如果没有找到其他进程使用当前的1ast_pid，则说明1ast_pid是唯一的，函数返回该值。
  通过此方法，理论上每个新 fork 的线程都会存在一个唯一的 id

### 练习3：编写 proc_run 函数（需要编码）
- 代码如下：
   ```
    void
    proc_run(struct proc_struct *proc) {
    if (proc != current) {
        bool intr_flag;
        local_intr_save(intr_flag);
        struct proc_struct *prev = current;
        struct proc_struct *next = proc;
        current = proc;
        lcr3(proc->cr3);
        switch_to(&(prev->context), &(next->context));
        local_intr_restore(intr_flag);
    }
    }

   ```
- 创建且运行了两个内核线程:
  1.idleproc:第一个内核进程，完成内核中各个子系统的初始化，之后立即调度，执行其他进程。
  2.initproc:用于完成实验的功能而调度的内核进程。

### Challenge 扩展练习

- 代码如下：
   ```
    local_intr_save(intr_flag);....local_intr_restore(intr_flag); 
    这俩个函数定义于kern/sync/sync.h中，函数声明如下：
    static inline bool __intr_save(void) {
    if (read_csr(sstatus) & SSTATUS_SIE) {
        intr_disable();
        return 1;
    }
    return 0;
    }
    static inline void __intr_restore(bool flag) {
        if (flag) {
            intr_enable();
        }
    }
    #define local_intr_save(x) \
        do {                   \
            x = __intr_save(); \
        } while (0)
    #define local_intr_restore(x) __intr_restore(x);
    而intr_disable()和ntr_enable()则是调用了set_csr()和clear_csr()实现的，函数定义如下：
    #define set_csr(reg, bit) ({ unsigned long __tmp; \
      asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "rK"(bit)); \
      __tmp; })

    #define clear_csr(reg, bit) ({ unsigned long __tmp; \
      asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "rK"(bit)); \
      __tmp; })

   ```
   - 这里采用的是内联汇编代码，%0代表输出操作数，即存储结果的变量tmp ，%1代表输入操作数，即传递给内联汇编代码的变量bit。
     该代码利用asm volatile关键词声明内联汇编代码，使用csrrs读取当前寄存器的值，并将特定位设1，
     与此同时使用%0和约束=r"(__tmp)使得_tmp和输出寄存器关联；%1和约束rK"(bit)使得bit和输入寄存器相关联，
     即汇编指令：csrrs rd, csr, rs1，将SSTATUS_SIE的值改变从而启用或禁止中断。
