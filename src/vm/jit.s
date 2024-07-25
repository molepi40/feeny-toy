#define HOLE 0xcafebabecafebabe
    .global top_of_heap
    .global heap_pointer
    .global frame_base_pointer
    .global operand_pointer

    .global call_cfeeny
    .global call_cfeeny_end

    .global label_ins
    .global label_ins_end
    .global goto_ins
    .global goto_ins_end    
    .global branch_ins
    .global branch_ins_end

    .global lit_ins
    .global lit_ins_end
    .global get_local_ins
    .global get_local_ins_end
    .global set_local_ins
    .global set_local_ins_end
    .global get_global_ins
    .global get_global_ins_end
    .global set_global_ins
    .global set_global_ins_end
    .global drop_ins
    .global drop_ins_end

    .global return_ins
    .global return_ins_end

    .global call_ins
    .global call_ins_end

    .global object_ins
    .global object_ins_end

    .global ADDRESS_OF_IP
    .global BYTECODE_INSTR

    .global array_ins
    .global array_ins_end

    .global call_slot_ins
    .global call_slot_ins_end
    .global primitive_int_op
    .global primitive_array_op
    .global jump_opcode

    .global trap_ins
    .global trap_ins_end

    .global trap_to_c
    .global trap_to_c_end

    .global set_up_call_slot
    .global set_up_call_slot_end

    .global slot_ins
    .global slot_ins_end
    .global set_slot_ins
    .global set_slot_ins_end


call_cfeeny:
    pushq %rbx
    movq %rdi, %rax
    movq top_of_heap(%rip), %rdi
    movq heap_pointer(%rip), %rsi
    movq operand_pointer(%rip), %rdx
    movq frame_base_pointer(%rip), %rcx
    movq frame_pointer(%rip), %rbx
    callq *%rax
    movq %rsi, heap_pointer(%rip)
    movq %rdx, operand_pointer(%rip)
    movq %rcx, frame_base_pointer(%rip)
    movq %rbx, frame_pointer(%rip)
    popq %rbx
    ret
call_cfeeny_end:


label_ins:
label_ins_end:

goto_ins:
    movq $0xcafebabecafebabe, %rax
    jmp *%rax 
goto_ins_end:

branch_ins:
    movq $0xcafebabecafebabe, %rax
    # # pop from operand stack
    addq $-8, %rdx
    movq 0(%rdx), %r8
    # check if it equals null object
    cmpq $0x2, %r8
    # not equal null object, branch
    je branch_ins_end
    jmp *%rax
branch_ins_end:

lit_ins:
    movq $0xcafebabecafebabe, %rax
    movq %rax, 0(%rdx)
    addq $0x8, %rdx
lit_ins_end:

get_local_ins:
    # local slot index
    mov $0xcafebabecafebabe, %rax
    # get slot from local frame
    movq 24(%rcx, %rax, 8), %rax
    # add to operand stack
    movq %rax, (%rdx)
    addq $0x8, %rdx
get_local_ins_end:

set_local_ins:
    # local slot index
    mov $0xcafebabecafebabe, %rax
    # get from operand stack peek
    movq -8(%rdx), %r8
    # set local frame
    movq %r8, 24(%rcx, %rax, 8)   
set_local_ins_end:

get_global_ins:
    # global slot index
    movq $0xcafebabecafebabe, %rax
    # get slot from global slots
    movq $0xcafebabecafebabe, %r8
    movq 16(%r8, %rax, 8), %rax
    # add to operand stack
    movq %rax, (%rdx)
    addq $0x8, %rdx
get_global_ins_end:

set_global_ins:
    # global slot index
    movq $0xcafebabecafebabe, %rax
    # get slot addr
    movq $0xcafebabecafebabe, %r8
    # get from operand stack peek
    movq -8(%rdx), %r9   
    # set global slot
    movq %r9, 16(%r8, %rax, 8)
set_global_ins_end:

drop_ins:
    addq $-8, %rdx
drop_ins_end:


return_ins:
    # parent frame
    movq (%rcx), %r8
    # check if parent frame is NULL
    cmpq $0x0, %r8
    jne not_exit
    # parent frame is NULL, exit program
    movq $0x0, %rax
    ret

    not_exit:
    # return address
    movq 8(%rcx), %r9
    # set frame
    movq %rcx, %rbx
    movq %r8, %rcx
    # jmp to ra
    jmp *%r9
return_ins_end:


call_ins:
    # movq ops
    movq $0xcafebabecafebabe, %r8  # ra
    movq $0xcafebabecafebabe, %r9  # nargs
    movq $0xcafebabecafebabe, %r10 # nlocals
    movq $0xcafebabecafebabe, %r11 # call addr
    # create new frame
    # parent frame
    movq %rcx, (%rbx)
    movq %rbx, %rcx
    # return address
    movq %r8, 8(%rcx) 
    # nslots
    addq %r9, %r10
    movq %r10, 16(%rcx)
    # update frame pointer
    leaq 24(%rcx, %r10, 8), %rbx
    
    # mov arguments
    movq %r9, %rax
    xorq $-1, %rax
    addq $0x1, %rax           # flip rax  
    leaq (%rdx, %rax, 8), %r8 # start of args in operand stack
    pushq %r8                 # store new operand stack pointer
    leaq 24(%rcx), %r10       # start of args in local frame
    pushq %r12                # store original r12
    andq $0x0, %r12           # loop counter
    
    loop_test_0:
    cmpq %r12, %r9 # nargs - i
    jle loop_end_0
    movq (%r8), %rax
    movq %rax, (%r10)
    addq $0x8, %r8
    addq $0x8, %r10
    addq $0x1, %r12
    jmp loop_test_0
    loop_end_0:
    # restore r12
    popq %r12
    # set operand stack pointer
    popq %rdx
    # jmp to new funtion
    jmp *%r11
call_ins_end:


object_ins:
    movq $0xcafebabecafebabe, %r8  # nslots
    # test heap
    leaq 24(%rsi, %r8, 8), %rax
    cmpq %rax, %rdi
    jae make_object
    # out of range of heap, trap to launch GC
    pushq %rdi
    movq after_trap_0(%rip), %rdi
    movq $0xcafebabecafebabe, %rax
    call *%rax
    popq %rdi
    movq $0x1, %rax
    ret

    after_trap_0:
    movq $0xcafebabecafebabe, %r8  # nslots

    make_object:
    movq $0xcafebabecafebabe, %r9  # class
    # set object tag
    mov $0x4, %eax
    mov %eax, (%rsi)
    # set size
    movq $24, %rax
    leaq (%rax, %r8, 8), %rax
    mov %eax, 4(%rsi)
    # set class
    movq %r9, 8(%rsi)
    # parent object and slots value in operand stack
    movq %r8, %rax
    xorq $-1, %rax
    addq $0x1, %rax
    leaq (%rdx, %rax, 8), %r10  
    addq $-8, %r10
    # set parent
    movq (%r10), %rax
    movq %rax, 16(%rsi)
    # store new operand stack pointer   
    pushq %r10

    # set slots
    addq $0x8, %r10 # start of args in operand stack
    leaq 24(%rsi), %r9  # start of object slots
    andq $0, %r11

    loop_test_1:
    cmpq %r11, %r8  # nargs - i
    jle loop_end_1
    movq (%r10), %rax
    movq %rax, (%r9)
    addq $0x8, %r10
    addq $0x8, %r9
    addq $0x1, %r11
    jmp loop_test_1

    loop_end_1:
    # set operand stack pointer
    popq %rdx
    # make tagged object and push object to operand stack
    movq %rsi, %rax
    orq $0x1, %rax
    movq %rax, (%rdx)
    addq $0x8, %rdx
    # update heap pointer
    movq %r9, %rsi    
object_ins_end:

array_ins:
    movq -16(%rdx), %rax
    sarq $0x3, %rax # get length int
    leaq 16(%rsi, %rax, 8), %rax
    cmpq %rax, %rdi
    jae make_array

    # out of range of heap, trap to launch GC
    pushq %rdi
    leaq after_trap_1(%rip), %rdi
    movq $0xcafebabecafebabe, %rax
    call *%rax
    popq %rdi
    movq $0x1, %rax
    ret
    after_trap_1:

    make_array:
    movq -16(%rdx), %r8
    sarq $0x3, %r8  # get length int
    movq -8(%rdx), %r9
    addq $-16, %rdx
    # set obj tag
    mov $0x3, %eax
    mov %eax, (%rsi)
    # set size
    movq $0x10, %rax
    leaq (%rax, %r8, 8), %rax
    mov %eax, 4(%rsi)
    # set length
    mov %r8d, 8(%rsi)
    # start of slots of array
    leaq 16(%rsi), %r10

    # set initials of array
    andq $0x0, %r11 # counter
    loop_test_2:
    cmpq %r11, %r8
    jle loop_end_2
    movq %r9, (%r10)
    addq $0x8, %r10
    addq $0x1, %r11
    jmp loop_test_2
    loop_end_2:

    # # push array to operand stack 
    movq %rsi, %rax
    orq $0x1, %rax
    movq %rax, (%rdx)
    addq $0x8, %rdx
    # set heap pointer
    movq %r10, %rsi
array_ins_end:


call_slot_ins:
    # get receiver
    movq $0xcafebabecafebabe, %r8    # opcode
    movq $0xcafebabecafebabe, %rax   # nargs, receiver included
    xorq $-1, %rax
    addq $0x1, %rax
    movq (%rdx, %rax, 8), %rax   # primitive
    # check its type
    movq %rax, %r9
    andq $0x7, %r9
    # check int
    cmpq $0x0, %r9
    jne check_object
    movq $0xcafebabecafebabe, %r10  # primitive int op, absolute addr
    call *%r10
    jmp call_slot_ins_end    # jmp to end

    check_object:
    cmpq $0x1, %r9
    je primit_instance_op
    # invalid primitive tag
    jmp exit_program

    primit_instance_op:
    # get pointer
    andq $0xfffffffffffffff8, %rax
    # check tag
    mov (%rax), %r9d
    # check array tag
    cmp $0x3, %r9d
    jne check_instance
    movq $0xcafebabecafebabe, %r10  # primitive array op, absolute addr
    call *%r10
    jmp call_slot_ins_end    # jmp to end

    check_instance:
    cmp $0x4, %r9d
    je instance_call_cache_check

    exit_program:
    # invalid receiver
    movl $0x1, %edi
    call exit@PLT

    instance_op_trap:
    # prelude
    pushq %rdi
    pushq %rsi
    # store pc
    leaq call_slot_ins_end(%rip), %rdi  
    # store instruction
    movq $0xcafebabecafebabe, %rsi
    # call trap_to_c
    movq $0xcafebabecafebabe, %rax
    call *%rax
    # ending
    popq %rsi
    popq %rdi
    # operation code
    movq $0xcafebabecafebabe, %rax    
    ret
    
    instance_call_cache_check:
    movq 8(%rax), %r8
    leaq call_slot_ins_end(%rip), %r9
    movq -8(%r9), %r10
    # check if cached
    cmpq $-1, %r10
    je instance_op_trap
    cmpq %r10, %r8
    jne instance_op_trap
    movq -16(%r9), %r10
    jmp *%r10
    jmp call_slot_ins_end

.quad -1
.quad -1
call_slot_ins_end:


primitive_array_op:

    jmp check_opcode    

primitive_int_op:
    # operand 0 in %rax
    sarq $0x3, %rax  # operand 0
    addq $-8, %rdx   # top of receiver
    movq (%rdx), %r9
    sarq $0x3, %r9   # operand 1

    check_opcode:
    leaq jmp_opcode(%rip), %r11
    leaq (%r11, %r8, 2), %r10
    jmp *%r10
    
    jmp_opcode:
    jmp primitive_int_add_op
    jmp primitive_int_sub_op
    jmp primitive_int_mul_op
    jmp primitive_int_div_op
    jmp primitive_int_mod_op
    jmp primitive_int_lt_op
    jmp primitive_int_gt_op
    jmp primitive_int_le_op
    jmp primitive_int_ge_op
    jmp primitive_int_eq_op
    jmp primitive_array_get_op
    jmp primitive_array_set_op
    jmp primitive_array_length_op

    primitive_int_add_op:
    # int add
    addq %r9, %rax
    jmp make_primitive_int

    primitive_int_sub_op:
    # int sub
    subq %r9, %rax
    jmp make_primitive_int

    primitive_int_mul_op:
    # int mul
    pushq %rdx
    imulq %r9
    popq %rdx
    jmp make_primitive_int

    primitive_int_div_op:
    # int div
    pushq %rdx
    cqto
    idivq %r9
    popq %rdx
    jmp make_primitive_int

    primitive_int_mod_op:
    # int mod
    pushq %rdx
    cqto
    idivq %r9
    movq %rdx, %rax
    popq %rdx
    jmp make_primitive_int

    primitive_int_lt_op:
    # int lt
    cmpq %r9, %rax
    setl %al
    jmp check_comparasion

    primitive_int_gt_op:
    # int gt
    cmpq %r9, %rax
    setg %al
    jmp check_comparasion
    
    primitive_int_le_op:
    # int le
    cmpq %r9, %rax
    setle %al
    jmp check_comparasion
    
    primitive_int_ge_op:
    # int ge
    cmpq %r9, %rax
    setge %al
    jmp check_comparasion
    
    primitive_int_eq_op:
    # int eq
    cmpq %r9, %rax
    sete %al
    jmp check_comparasion

    # array get
    primitive_array_get_op:
    addq $-8, %rdx
    movq (%rdx), %r9
    sarq $0x3, %r9
    movq 16(%rax, %r9, 8), %rax
    jmp store_stack

    # array set
    primitive_array_set_op:
    movq -8(%rdx), %r10 # value
    addq $-16, %rdx
    movq (%rdx), %r9    
    sarq $0x3, %r9      # index
    movq %r10, 16(%rax, %r9, 8)
    movq $0x2, %rax     # push null to operand stack
    jmp store_stack

    # array length
    primitive_array_length_op:
    mov 8(%rax), %eax
    cltq
    jmp make_primitive_int

    check_comparasion:
    cmpb $0x0, %al
    jne make_primitive_int
    movq $0x2, %rax
    jmp store_stack
    
    make_primitive_int:
    salq $0x3, %rax

    store_stack:
    movq %rax, -8(%rdx)
    ret

primitive_int_op_end:

trap_ins:
    # prelude
    pushq %rdi
    pushq %rsi
    # store pc
    leaq trap_ins_end(%rip), %rdi  
    # store instruction
    movq $0xcafebabecafebabe, %rsi
    # call trap_to_c
    movq $0xcafebabecafebabe, %rax
    call *%rax
    # ending
    popq %rsi
    popq %rdi
    # operation code
    movq $0xcafebabecafebabe, %rax    
    ret
trap_ins_end:

trap_to_c:
    movq %rdi, ADDRESS_OF_IP(%rip)
    movq %rsi, BYTECODE_INSTR(%rip)
    ret
trap_to_c_end:


set_up_call_slot:
    # movq ops
    movq $0xcafebabecafebabe, %r8  # ra
    movq $0xcafebabecafebabe, %r9  # nargs
    movq $0xcafebabecafebabe, %r10 # nlocals
    # create new frame
    # parent frame
    movq %rcx, (%rbx)
    movq %rbx, %rcx
    # return address
    movq %r8, 8(%rcx) 
    # nslots
    addq %r9, %r10
    movq %r10, 16(%rcx)
    # update frame pointer
    leaq 24(%rcx, %r10, 8), %rbx
    
    # mov arguments
    movq %r9, %rax
    xorq $-1, %rax
    addq $0x1, %rax           # flip rax  
    leaq (%rdx, %rax, 8), %r8 # start of args in operand stack
    pushq %r8                 # store new operand stack pointer
    leaq 24(%rcx), %r10       # start of args in local frame
    andq $0x0, %r11           # loop counter
    
    loop_test_3:
    cmpq %r11, %r9 # nargs - i
    jle loop_end_3
    movq (%r8), %rax
    movq %rax, (%r10)
    addq $0x8, %r8
    addq $0x8, %r10
    addq $0x1, %r11
    jmp loop_test_3
    loop_end_3:

    # set operand stack pointer
    popq %rdx

    # jmp to method code
    movq $0xcafebabecafebabe, %rax
    jmp *%rax

set_up_call_slot_end:

slot_ins:
    # check ever cached
    leaq slot_ins_end(%rip), %r8
    movq -8(%r8), %rax
    cmpq $-1, %rax
    je slot_ins_trap
    
    # get instance pointer
    movq -8(%rdx), %r9
    andq $0xfffffffffffffff8, %r9
    # get class
    movq 8(%r9), %r10
    # check if cached
    movq -8(%r8), %rax
    cmpq %rax, %r10
    jne slot_ins_trap

    # get slot layer
    movq -16(%r8), %r10
    andq $0x0, %r11
    loop_test_4:
    cmpq %r11, %r10
    jle loop_end_4
    movq 16(%r9), %r9   # parent object
    andq $0xfffffffffffffff8, %r9
    addq $0x1, %r11
    jmp loop_test_4
    loop_end_4:

    # load cached index
    movq -24(%r8), %r10
    movq 24(%r9, %r10, 8), %rax
    movq %rax, -8(%rdx)
    jmp slot_ins_end

    slot_ins_trap:
    # prelude
    pushq %rdi
    pushq %rsi
    # store pc
    leaq slot_ins_end(%rip), %rdi  
    # store instruction
    movq $0xcafebabecafebabe, %rsi
    # call trap_to_c
    movq $0xcafebabecafebabe, %rax
    call *%rax
    # ending
    popq %rsi
    popq %rdi
    # operation code
    movq $0xcafebabecafebabe, %rax    
    ret

.quad -1
.quad -1
.quad -1
slot_ins_end:


set_slot_ins:
    # check ever cached
    leaq slot_ins_end(%rip), %r8
    movq -8(%r8), %rax
    cmpq $-1, %rax
    je set_slot_ins_trap
    
    # get instance pointer
    movq -16(%rdx), %r9
    andq $0xfffffffffffffff8, %r9
    # get class
    movq 8(%r9), %r10
    # check if cached
    movq -8(%r8), %rax
    cmpq %rax, %r10
    jne set_slot_ins_trap

    # get slot layer
    movq -16(%r8), %r10
    andq $0x0, %r11
    loop_test_5:
    cmpq %r11, %r10
    jle loop_end_5
    movq 16(%r9), %r9   # parent object
    andq $0xfffffffffffffff8, %r9
    addq $0x1, %r11
    jmp loop_test_5
    loop_end_5:

    # load cached index
    movq -24(%r8), %r10
    # set slot
    addq $-8, %rdx
    movq (%rdx), %rax
    movq %rax, 24(%r9, %r10, 8)
    movq %rax, -8(%rdx)
    jmp slot_ins_end

    set_slot_ins_trap:
    # prelude
    pushq %rdi
    pushq %rsi
    # store pc
    leaq set_slot_ins_end(%rip), %rdi  
    # store instruction
    movq $0xcafebabecafebabe, %rsi
    # call trap_to_c
    movq $0xcafebabecafebabe, %rax
    call *%rax
    # ending
    popq %rsi
    popq %rdi
    # operation code
    movq $0xcafebabecafebabe, %rax    
    ret

.quad -1
.quad -1
.quad -1
set_slot_ins_end:
