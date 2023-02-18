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
    pushq   %rbx
    # 将rdx寄存器内容送至rbx
    movq    %rdx, %rbx
    # 调用mult2函数
    call    mult2@PLT
    # 将rbx保存内容对应地址的内容送入rax寄存器
    movq    %rax, (%rbx)
    # 将rbx寄存器内容弹出
    popq    %rbx
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
    push    rbx
    mov     rbx, rdx
    call    mult2@PLT
    mov     QWORD PTR [rbx], rax
    pop     rbx
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

GCC生成的汇编代码指令都有一个指定操作数字长的后缀，如movb、movw、movl、movq：

字符|字长
:-:|:-:
b|字节
w|字
l|双字
q|四字

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

操作数类型：

+ 立即数：表示常数值。以$开头。
+ 寄存器：表示寄存器的内容。使用rx来表示寄存器x，R[rx]表示寄存器x的值。
+ 内存引用：表示存储值的有效地址。My[x]表示x内存地址开始一共y个字节保存的值。

其中x(ry,rz,s)表示立即数偏移x，基址寄存器ry，变址寄存器rz，比例因子s。其中s必须为1、2、4、8，有效地址计算形式：x+R[ry]+R[rz]*s。

名称|类型|格式|操作数值|实例
:-:|:--:|:-:|:-----:|:-:
立即寻址|立即数|$x|x|$0x108
寄存器寻址|寄存器|rx|R[rx]|%rax
绝对寻址|存储器|x|M[x]|0x108
间接寻址|存储器|(rx)|M\[R[rx]]|(%rax)
基址选址|存储器|x(ry)|M[x+R[ry]]|4(%rax)
变址选址|存储器|(rx,ry)|M[R[rx]+R[ry]]|(%rax,%rdx)
变址选址|存储器|x(ry,rz)|M[x+R[ry]+R[rz]]|4(%rax,%rdx)
比例变址寻址|存储器|(,rx,y)|M[R[rx]*y]|(,%rax,4)
比例变址寻址|存储器|x(,ry,z)|M[x+R[ry]*z]|4(,%rax,4)
比例变址寻址|存储器|(rx,ry,z)|M[R[rx]+R[ry]*z]|(%rax,%rbx,4)
比例变址寻址|存储器|x(ry,rz,s)|M[x+R[ry]+R[rz]*s]|6(%rax,%rbx,4)

### 数据传送指令

指令|效果|描述
:-:|:--:|:-:
mov a, b|b<-a|传送
movb|
movw|
movl|
movq|
movabsq||传输绝对四字

+ mov指令只会更新目的操作数指定寄存器字节或内存位置。唯一例外是movl指令以寄存器作为目的时，会将寄存器高4位置0，这是惯例。
+ a可以是立即数、寄存器、内存地址，b可以是寄存器、内存地址。
+ x86-64限制了ab不能都指向内存地址。所以从内存到内存应该以寄存器为中介。
+ movq指令和movabsq都是将四字立即数作为原操作数。但是movq只能使用32位补码立即数，然后符号扩展到64位；而movabsq能以任何64位立即数，但是目的地址只能为寄存器。

操作数类型|实例
:------:|:--:
立即数-寄存器|movl $0x4080,%eax
立即数-内存|movb $-16,(%rsp)
寄存器-寄存器|movw %ax,%bx
内存-寄存器|movb (%rdi,%rcx),%al
寄存器-内存|movq %rax,-12(%rbp)

当较小的数据复制到较大目的时，有零扩展MOVZ和符号扩展MOVS两种处理方式，指令最后有两个字符，第一个字符为源大小，第二个字符为目的大小：

指令|效果|描述
:-:|:--:|:-:
movz a, b|b<-zero(a)|零扩展传输
movzbw|
movzbl|
movzwl|
movzbq|
movzwq|

+ 没有movzlq，但是可以通过movl指令来实现，这是因为默认会将高4位置0。
+ a可以是寄存器或内存，b只能是寄存器。

指令|效果|描述
:-:|:--:|:-:
movs a, b|b<-signal(a)|符号扩展传输
movsbw|
movsbl|
movswl|
movsbq|
movswq|
movslq|
cltq|%rax<-signal(%eax)|寄存器自填充四字

+ a可以是寄存器或内存，b只能是寄存器。
+ cltq指令没有操作数，只作用于%eax到%rax。就是movslq %eax,%rax功能一致。

编写exchange.c文件：

```c
long exchange(long *xp, long y)
{
    long x = *xp;
    *xp = y;
    return x;
}
```

`gcc -Og -S exchange.c`编译：

```s
# xp存储%rdi，y存储在%rsi
exchange:
    # 读出xp并存储到rax
    movq    (%rdi), %rax
    # 将%rsi值传送到rdi中
    movq    %rsi, (%rdi)
    # 存储值在寄存器低位返回
    ret
```

### 栈操作指令

指令|效果|描述
:-:|:--:|:-:
pushq a|R[%rsp]<-R[%rsp]-8</br>M[R[%rsp]]<-a|压入栈
popq a|a<-M[R[%rsp]]</br>R[%rsp]<-R[%rsp]+8|弹出栈

+ x86-64中，程序栈在内存中，且向下增长，栈顶元素在低位，栈底元素在高位，栈底地址不变，所以栈操作不用担心越界问题。
+ %rsp保存栈顶指针。
+ 默认都是对一个机器字长的数据进行操作。

所以pushq %rbp指令等价于subq $8, %rsp，movq %rbp, (%rsp)。

popq %rax指令等价于movq (%rsp), %rax，addq $8, %rsp。

## 算术逻辑操作

### 加载有效地址

指令|效果|描述
:-:|:--:|:-:
lea a, b|b<-&a|加载有效地址
leaw|
leal|
leaq|

+ 实际是mov指令的变形。主要为了计算偏移地址。
+ 从内存读数据并计算传输到寄存器。目的只能是寄存器。
+ 实际上并没有引用内存，而是将地址写入目的操作数。

操作数|MOV指令|LEA指令
:-:|:----:|:----:
变量|取值|取地址
[变量]|取值|取地址
寄存器|取值|取地址
[寄存器]|取地址|取值

### 一元操作

指令|效果|描述
:-:|:--:|:-:
inc a|a<-a+1|自增1
incb|
incw|
incl|
incq|
dec a|a<-a-1|自减1
decb|
decw|
decl|
decq|
neg a|a<- -a|取负
negb|
negw|
negl|
negq|
not a|a<-!a|取反
notb|
notw|
notl|
notq|

+ 唯一操作数既是源又是目的。
+ 操作数可以是寄存器也可以是内存。

### 二元操作

指令|效果|描述
:-:|:--:|:-:
add a, b|b<-b+a|加
addb|
addw|
addl|
addq|
sub a, b|b<-b-a|减
subb|
subw|
subl|
subq|
imul a, b|b<-b*a|乘
imulb|
imulw|
imull|
imulq|
xor a, b|b<-b^a|异或
xorb|
xorw|
xorl|
xorq|
or a, b|b<-b\|a|或
orb|
orw|
orl|
orq|
and a, b|b<-b&a|与
andb|
andw|
andl|
andq|

+ 第二个操作数既是源又是目的。
+ 第一个操作数可以是立即数、寄存器、内存。
+ 第二个操作数可以是寄存器、内存。当第二个操作数为内存时必须先从内存读出再写回。

### 移位操作

指令|效果|描述
:-:|:--:|:-:
sal a, b|b<-b<<a|左移
salb|
salw|
sall|
salq|
shl a, b|b<-b<<a|左移
shlb|
shlw|
shll|
shlq|
sar a, b|b<-b>>a|算术左移
sarb|
sarw|
sarl|
sarq|
shr a, b|b<-b>>a|逻辑左移
shrb|
shrw|
shrl|
shrq|

+ 第一个操作数为移位量。可以是立即数或存放在指定寄存器%cl中。
+ 移位量由%cl低m位决定，高位忽略，如%cl的值为0xFF时，指令salb移位7位，salw移位15位，sall移位31位，salq移位63位。
+ 第二个操作数是要移位数据。可以是寄存器或内存。
+ 左移指令SAL和SHL作用一样。

编写shift_left4_rightn.c：

```c
long shift_left4_rightn(long x, long n)
{
    x <<= 4;
    x >>= n;
    return x;
}
```

`gcc -Og -S shift_left4_rightn.c`汇编：

```s
# x在%rdi中，n在%rsi中
shift_left4_rightn:
    movq    %rdi, %rax
    salq    $4, %rax
    movl    %esi, %ecx
    sarq    %cl, %rax
    ret
```

### 特殊算术操作

指令|效果|描述
:-:|:--:|:-:
imulq a|[R[%rdx]:R[%rax]]<-a*R[%rax]|有符号全乘法
mulq a|[R[%rdx]:R[%rax]]<-a*R[%rax]|无符号全乘法
clto|[R[%rdx]:R[%rax]<-singal[R[%rax]]|符号拓展为8位

+ imulq a, b就是两个64位操作数相乘乘积为64位的乘法指令。由于保存到b中，所以不能完全保存结果。
+ 而imulq a和mulq a使用单操作数，分别实现无符号和补码乘法，使用%rdx保存高64位，%rax保存低64位，从而完全保存了结果。

新建一个store_uprod.c：

```c
// int类型拓展，可以提供64位的值
#include <inttypes.h>

// 由于这个标准没有提供128位的标准，所以依赖GCC提供的128位整数支持
typedef unsigned __int128 uint128_t;

void store_uprod(uint128_t *dest, uint64_t x, uint64_t y)
{
    *dest = x * (uint128_t)y;
}
```

`gcc -Og -S store_uprod.c`汇编：

```s
# x在%rsi中，y在%rdx中
store_uprod:
    # 将x移动到%rax中
    movq    %rsi, %rax
    # %rdx*%rax
    mulq    %rdx
    # %rdi保存dest指针
    # 将%rax保存的低位移动到%rdi中
    movq    %rax, (%rdi)
    # 将%rdx保存的高位移动到%rdi的高八字节中
    movq    %rdx, 8(%rdi)
    ret
```

指令|效果|描述
:-:|:--:|:-:
idivq a|R[%rdx]<-[R[%rdx]:R[%rax]]mod a</br>R[%rax]<-[R[%rdx]:R[%rax]]/a|有符号除法
divq a|R[%rdx]<-[R[%rdx]:R[%rax]]mod a</br>R[%rax]<-[R[%rdx]:R[%rax]]/a|无符号除法

+ 如果被除数是128位，除法将寄存器%rdx作为被除数的高64位，%rax作为被除数的低64位，除数作为指令的操作数给出，将商存储在%rax中，将余数存储在%rdx中。
+ 如果被除数是64位，除数也往往是64位，应该存放在%rax中，%rdx的位数应该置为%rax的符号位，所以这个操作就需要使用cqto（如果是Intel就是cqo）指令完成，将%rax的符号位也扩充到%rdx中。

新建一个remdiv.c：

```c
void remdiv(long x, long y, long *qp, long *rp)
{
    long q = x / y;
    long r = x % y;
    *qp = q;
    *rp = r;
}
```

`gcc -Og -S remdiv.c`汇编：

```s
# x在%rdi，y在%rsi，qp在%rdx，rp在%rcx
remdiv:
    # 将x送到%rax作为被除数
    movq    %rdi, %rax
    # 由于除法操作需要使用%rdx保存余数，所以要转移pq到寄存器中
    movq    %rdx, %r8
    # %rax拓展到%rdx
    cqto
    # 将[%rdx:%rad]除以%rsi，即x/y
    # 此时%rax为商，%rdx为余数
    idivq   %rsi
    movq    %rax, (%r8)
    movq    %rdx, (%rcx)
    ret
```

## 控制

### 状态码

CPU还维护状态码寄存器，用于描述最近操作的属性，可以通过这个来进行条件控制。

+ CF：进位标志。是否进位。检查无符号操作溢出。
+ ZF：零标志。是否为0。检查结果是否为0。
+ SF：符号标志。结果符号位。检查是否得到了负数。
+ OF：溢出标志。补码是否溢出。检查补码数是否溢出。

当计算t=a+b时：

+ CF：(unsigned) t < (unsigned) a。无符号溢出。
+ ZF：(t == 0)。结果零。
+ SF：（t < 0）。负数。
+ OF：(a < 0 == b < 0) && (t < 0 != a < 0)。有符号溢出。

算术和逻辑运算指令会改变条件码，还有两种命令会改变状态码而不改变其他寄存器内容。

指令|效果|描述
:-:|:--:|:-:
cmp a, b|b-a|比较大小
cmpb|
cmpw|
cmpl|
cmpq|
test a, b|a&b|测试
testb|
testw|
testl|
testq|

+ cmp指令行为跟sub指令一样，根据两个操作数之差设置状态码但是不设置其他寄存器。
+ test指令行为跟and指令一样，对两个操作数进行与操作设置状态码但是不设置其他寄存器。用来判断两个操作数是否一致，或者一个操作数是掩码来指示哪些位需要被测试。

### 访问状态码

状态码不直接读取：

1. 根据状态码组合，将一个字节设置为0或1。
2. 根据状态跳转到程序其他部分。
3. 根据条件判断是否传输数据。

指令|同义名|效果|设置条件
:-:|:---:|:-:|:------:
sete a|setz|a<ZF|相等/零
setne a|setnz|a<-~ZF|不等/非零
sets a||a<-SF|负数
setns a||a<-~ZF|非负
setg a|setnle|a<-~(SF^OF)&~ZF|有符号大于
setg a|setnle|a<-~(SF^OF)&~ZF|有符号大于
setge a|setnl|a<-~(SF^OF)|有符号大于等于
setl a|setnge|a<-SF^OF|有符号小于
setle a|setng|a<-(SF^OF)\|ZF|有符号小于等于
seta a|setnbe|a<-~CF&~ZF|无符号大于
setae a|setnb|a<-~CF|无符号大于等于
setb a|setnae|D<-CF|无符号小于
setbe a|setna|a<-CF\|ZF|无符号小于等于

+ set指令通过t=a-b来实现。
+ 对于sete，当a=b时，t=a-b=0，此时ZF=0。
+ 对于setl，若没有溢出则OF=0，a-b<0，SF=1，a-b>0，SF=0，若有溢出则OF=1，a-b>0即负溢出SF=0，则a<b，a-b<0时正溢出SF=1，则a>b，从而OF=1时OF=0时a<b。

### 跳转指令

跳转指令能改变顺序执行代码的顺序，跳转目的地往往需要用标记a指明跳转目的地。

+ 直接跳转：跳转目标在指令编码中。
+ 间接跳转：跳转目标在寄存器或内存位置读出。

指令|同义名|跳转条件|描述
:-:|:---:|:-----:|:-:
jmp a||1|直接跳转
jmp *a||1|间接跳转
je a|jz|ZF|相等/零
jne a|jnz|~ZF|不等/非零
js a||SF|负数
jns a||~SF|非负
jg a|jnle|~(SF^OF)&~ZF|有符号大于
jge a|jnl|~(SF^OF)|有符号大于等于
jl a|jnge|SF^OF|有符号小于
jle a|jng|(SF^OF)\|ZF|有符号小于等于
ja a|jnbe|~CF&~ZF|无符号大于
jae a|jnb|~CF|无符号大于等于
jb a|jnae|CF|无符号小于
jbe|jna|CF\|ZF|无符号小于等于

指令寻址有不同寻址方式。最常用的是PC相对选址，即目标指令地址和当前指令地址的后一条指令地址之差在当前指令中给出，然后就是绝对地址。

### 条件控制

最常用的就是使用有条件跳转和无条件跳转的条件控制来实现条件分支。

编写一个计算差绝对值的C语言文件。if-else版本absdiff_se.c：

```c
long lt_cnt = 0;
long ge_cnt = 0;

long absdiff_se(long x, long y)
{
    long result;
    if (x < y)
    {
        lt_cnt++;
        result = y - x;
    }
    else
    {
        ge_cnt++;
        result = x - y;
    }
    return result;
}
```

然后调用`gcc -Og -S absdiff_se.c`：

```s
absdiff_se:
    cmpq    %rsi, %rdi
    jge .L2
    addq    $1, lt_cnt(%rip)
    movq    %rsi, %rax
    subq    %rdi, %rax
    ret
.L2:
    addq    $1, ge_cnt(%rip)
    movq    %rdi, %rax
    subq    %rsi, %rax
    ret
```

如果使用汇编，然后再将汇编转为C语言会发现将其转换为了goto语句gotodiff_se.c：

```c
long lt_cnt = 0;
long ge_cnt = 0;

long gotodiff_se(long x, long y)
{
    long result;
    if (x >= y)
        goto x_ge_y;
    lt_cnt++;
    result = y - x;
    return result;
x_ge_y:
    ge_cnt++;
    result = x - y;
    return result;
}
```

所以对于C语言的if-else语句：

```txt
if 测试条件
    成功语句
else
    失败语句
后续语句
```

如果将其转换为汇编语句，则会使用类似goto语句的控制流进行实现：

```txt
    if(!测试条件)
        goto 失败标签
    成功语句
    goto 后续标签
失败标签:
    失败语句
后续标签:
    后续语句
```

### 条件传送

可以使用数据控制的条件传送实现条件分支。

默认使用控制的if-else语句沿着执行路径执行，当不满足条件再进行分支，虽然简单但是低效。

计算一个条件的两种结果，根据条件是否满足选取一个。只有在部分受限的情况这种策略才有效，可以只用条件传送指令来实现。

此时给定一个absdiff.c，简单返回计算差值，而不改变全局变量：

```c
long absdiff(long x, long y)
{
    long result;
    if (x < y)
        result = y - x;
    else
        result = x - y;
    return result;
}
```

`gcc -Og -S absdiff.c`：

```s
absdiff:
    cmpq    %rsi, %rdi
    jge .L2
    movq    %rsi, %rax
    subq    %rdi, %rax
    ret
.L2:
    movq    %rdi, %rax
    subq    %rsi, %rax
    ret
```

与cmovdiff有相同的形式：

```c
long cmovdiff(long x, long y)
{
    long rval = y - x;
    long eval = x - y;
    long ntest = x >= y;
    if (ntest)
        rval = eval;
    return rval;
}
```

计算两个数值的大小，除了相等的情况，则必有一个正一个负，然后判断xy大小关系，最后将正数值返回。

### 性能比较
