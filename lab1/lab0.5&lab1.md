## 操作系统实验报告 lab0.5&lab1

### lab0.5:

   - QEMU 模拟的 RISC-V 计算机加电开始运行到执行应用程序的第一条指令（即跳转到 0x80200000）这个阶段的执行过程。主要分为三个部分。
   
   - 第一部分是cpu通电后执行的三条指令，从0x1000到0x1010这三条指令，负责将地址跳转到0x80000000，这个地址是bootloader程序的入口地址。

   - 第二部分是bootloader的执行过程，从0x80000000开始，操作系统的内核被加载到内存中，之后跳转到0x80200000，进入操作系统的入口点。

   - 第三部分开始从操作系统执行汇编代码，入口点为kern\_entry,首先开辟一个内核栈，接着执行kern\_init,从而实现了字符串的输出


   - 对于qemu和gdb的使用有了一定的了解，对于上学期学习的计算机组成原理进行了回顾。

### lab1:

1. **练习1**:
   
   - la sp, bootstacktop这条指令设置了操作系统内核的初始堆栈，它将“bootstacktop”的地址加载到堆栈指针寄存器 sp 中。
   - bootstack中声明了KSTACKSIZE大小的内存空间用来作为堆栈空间，并存放了一个指向栈顶的地址。
   - tail kern\_init中tail是risc-v的伪指令，用于调用函数kern\_init。完成了操作系统的初始化。相较于lab0.5，该函数执行了中断初始化，中断时钟初始化和使能中断的初始化。

2. **练习2**:
  
   - 代码如下：
   ```yacas
   clock\_set\_next\_event();

   ticks++;

   if (ticks % TICK\_NUM == 0){

   print\_ticks();

   num\_a++;

   }

   if(num\_a==10)

   sbi\_shutdown();

   ```

   - 该段代码设置了一个时钟中断，每一个时钟中断后ticks计数+1，每进行100次以后调用print\_ticks()打印“100 ticks”字符串。同时打印次数+1，计数达到10次时调用sbi\_shutdown函数，关机。

3. **拓展1**:
  
   - 发生中断操作时，向stvec写入trapentry.S中的\_\_alltraps入口地址，执行SAVE\_ALL指令，将所有寄存器的值保存在结构体体中（即存储在内存中）。接着执行move a0，sp，将栈指针的地址（即该结构体的位置）存储在a0寄存器中并将其传向trap函数。trap函数处理中断类型后回到\_\_trapret，将结构体中的值恢复到寄存器中。位置是由结构体确认的。最后保存的寄存器数据最先恢复。并不是，对于一些特定的应用程序，只需要特定的寄存器状态，无需保存所有寄存器内容。

4. **拓展2**:

   - csrw sscratch, sp将栈指针寄存器sp的值保存到sscratch中，以便后续能够恢复。
   - csrrw s0, sscratch, x0将sscratch的值加载到sp中，并把sscratch的值赋0.
   - stval 寄存器存储了异常的附加信息，例如访问无效地址的地址。保存 stval 的值是为了后续的错误处理，而不需要还原 stval，因为它是一个用于异常处理的临时寄存器，不需要保持其状态。
   - scause 寄存器包含了导致异常或中断的原因代码。保存 scause 的值可以记录中断或异常的类型，但是不需要还原 scause，因为它是一个只读寄存器，不能被修改。

5. **拓展3**:

   - 代码如下
   ```yacas
   cprintf("Illegal instruction caught at 0x%016llx\n", tf->epc);

   tf->epc += 2;

   cprintf("braekpoint caught at 0x%016llx\n", tf->epc);

   tf->epc += 2;

   ```
   - 读到一个换行符时，它就计算表达式的值并打印结果。但目前这个程序只能处理单个数字字符（0-9）而不是多位数字。