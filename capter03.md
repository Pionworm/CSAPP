# 程序机器级表示

## 程序编码

### 机器代码

使用指令集体系结构或指令集架构ISA来定义机器级程序的格式和行为，包括处理器状态、指令格式、指令作用。

可见的寄存器：

+ 程序计数器PC（x86用%rip表示）：给出下一条指令内存地址。
+ 整数寄存器：包含十六个，分别存储六十四位值，可以存储数据或地址。
+ 状态码寄存器：保存最近执行的算术或逻辑指令的状态信息，实现条件控制。
+ 向量寄存器：保存整数或浮点数值。

无论编程语言使用何种数据类型，在内存中都是按顺序进行存储，不区分存储的内容类型。

程序内存（程序运行所占用的内存）：

+ 程序可执行机器代码。
+ 操作系统所需信息。
+ 管理过程调用和返回的运行时栈。
+ 用户分配的内存块。

### 代码案例

编写mstore.c：

```c
long mult2(long, long);

void multstore(long x, long y, long *dest)
{
    long t = mult2(x, y);
    *dest = t;
}
```

`gcc -Og -S mstore.c`编译，GCC即GCC C编译器，-Og表示让编译器用符合原始C代码整体结构的优化等级，避免过度代码优化导致代码结构严重变形从而难以理解生成的机器代码，-S使得汇编代码可见，生成mstore.s：

```s
multstore:
    # 将rbx寄存器内容压入用户栈
    pushq %rbx
    # 将rdx寄存器内容送至rbx
    movq %rdx, %rbx
    # 调用mult2函数
    call mult2@PLT
    # 将rbx保存内容对应地址的内容送入rax寄存器
    movq %rax, (%rbx)
    # 将rbx寄存器内容弹出
    popq %rbx
    # 返回用户栈内容return
    ret
```

其中去掉了所有以.开头的伪指令，这些指令用来指导汇编器和链接器工作，可以忽略。

如果使用`gcc -Og -c mstore.c`，则会编译并汇编生成mstore.o的二进制文件。

继续使用`objdump -d mstore.o`来将机器文件变为字节序列（反汇编）：

```txt
mstore.o：     文件格式 elf64-x86-64


Disassembly of section .text:

0000000000000000 <multstore>:
   0:   f3 0f 1e fa             endbr64 
   4:   53                      push   %rbx
   5:   48 89 d3                mov    %rdx,%rbx
   8:   e8 00 00 00 00          call   d <multstore+0xd>
   d:   48 89 03                mov    %rax,(%rbx)
  10:   5b                      pop    %rbx
  11:   c3                      ret   
```

一共11（即16+1=17）个十六进制字节值，分为若干组。

+ x86的指令长度从1到15个字节。
+ 设计指令格式方式：从给定位置开始，可以将字节唯一解码为机器指令。
+ 反汇编只是基于机器代码中的字节序列确定汇编代码，不需要访问程序源代码或汇编代码。
+ 反汇编器使用的指令命名规则与GCC生成的汇编代码命名有一定差别，比如将指令后的q省略。

如果要生成可执行的代码需要对目标代码文件运行链接器，且这个文件必须包含一个main函数作为开始入口，假定main.c有这样的函数：

```c
#include <stdio.h>

void multstore(long, long, long *);

int main()
{
    long d;
    multstore(2, 3, &d);
    printf("2 * 3 --> %ld\n", d);
    return 0;
}

long mult2(long a, long b)
{
    long s = a * b;
    return s;
}
```

然后生成可执行文件`gcc -Og -o prog main.c mstore.c`，将main.c也添加进入编译文件，将生成的目标文件命名为prog。

此时生成这个文件就有很多个字节，这是因为它不仅与两个文件的代码，还引入了用来启动和终止程序的代码以及用来和操作系统交互的代码。反汇编prog文件`objdump -d prog`：

```txt
00000000000011d8 <multstore>:
    11d8:       f3 0f 1e fa             endbr64 
    11dc:       53                      push   %rbx
    11dd:       48 89 d3                mov    %rdx,%rbx
    11e0:       e8 e7 ff ff ff          call   11cc <mult2>
    11e5:       48 89 03                mov    %rax,(%rbx)
    11e8:       5b                      pop    %rbx
    11e9:       c3                      ret   
```

基本上和mstore.c反汇编产生的代码一样，只是右边的地址不同，这是因为链接器将这段代码地址偏移到不同的地址了，还有call调用时找到了所需函数mult2的地址，这是因为链接器的作用之一就是用来链接各个所需函数的可执行代码到一个整体。此外还可能出现nop指令，用来将指令填充为十六字节的整数倍，这里没有进行填充。

### ATT与Intel

ATT也称为AT&T，是贝尔实验室格式汇编代码格式，是GCC等常用工具的默认格式，而来自Intel公司的汇编格式则是Intel。

如使用`gcc -Og -S -masm=intel mstore.c`可以得到Intel格式的汇编代码：

```s
multstore:
    endbr64
    push rbx
    mov rbx, rdx
    call mult2@PLT
    mov QWORD PTR [rbx], rax
    pop rbx
    ret
```

不同|Intel|ATT
:--:|:--:|:--:
省略指示大小后缀|push|pushq
省略寄存器前缀|rbx|%rbx
描述内存位置方式不同|QWORD PTR [rbx]|(%rbx)
操作数顺序相反|mov rbx, rdx|movq %rdx, %rbx

## 数据格式

最开始计算机系统是16位的，所以将16位称为一个字，32位就是双字，64位就是四字，标准int使用32位保存，指针为64位（因为机器是64位的）。

C声明|Intel数据类型|汇编代码后缀|字节大小
:--:|:----------:|:--------:|:----:
char|字节|b|1
short|字|w|2
int|双字|l|4
long|四字|q|8
char*|四字|q|8
float|单精度|s|4
double|双精度|l|8

GCC生成的汇编代码指令都有一个指定操作数字长的后缀，如movb、movw、movl、movq。

C程序中可以使用long double来指定10字节浮点格式进行全套浮点运算，但是不建议，因为不能移植且实现硬件低效。

## 访问信息

一个x86-64的中央处理单元CPU包含一组16个存储64位值的通用寄存器用来存储数据和指针。名字都以%r开头。

+ 8086：八个16位寄存器。
+ IA32：八个32位寄存器。
+ x86-64：八个64位寄存器。添加八个新寄存器，%r8~%r15。

作用|低8位|低16位|低32位|全64位
:-:|:---:|:---:|:----:|:---:
返回值|al%|ax%|eax%|rax%
保存被调用者|%bl|%bx|%ebx|%rbx
第四个参数|%cl|cx%|ecx%|%rcx
第三个参数|%dl|%dx|%edx|%rdx
第二个参数|%sil|%si|%esi|%rsi
第一个参数|%dil|%di|%edi|%rdi
保存被调用者|%bpl|%bp|%ebp|%rbp
栈指针|%spl|%sp|%esp|%rsp
第五个参数|%r8b|%r8w|%r8d|%r8
第六个参数|%r9b|%r9w|%r9d|%r9
保存调用者|%r10b|%r10w|%r10d|%r10
保存调用者|%r11b|%r11w|%r11d|%r11
保存被调用者|%r12b|%r12w|%r12d|%r12
保存被调用者|%r13b|%r13w|%r13d|%r13
保存被调用者|%r14b|%r14w|%r14d|%r14
保存被调用者|%r15b|%r15w|%r15d|%r15

栈指针用于保存运行时栈的结束位置，其他寄存器应用会更灵活。

### 操作数指示符
