#include "cpu.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define REGISTER_COUNT 4

struct cpu {
    enum cpu_status status;
    int32_t registers[REGISTER_COUNT];
    int32_t stack_size;
    int32_t instruction_pointer;
    int32_t *memory;
    int32_t *stack_bottom;
    int32_t *stack_limit;
};

static void set_registers_to_zero(struct cpu *cpu) {
    assert(cpu != NULL);
    for (int reg = REGISTER_A; reg < REGISTER_COUNT ; reg++) {
        cpu_set_register(cpu, reg, 0);
    }
}

int32_t *cpu_create_memory(FILE *program, size_t stack_capacity, int32_t **stack_bottom) {
    assert(program != NULL);
    assert(stack_bottom != NULL);
    size_t capacity = 0;
    size_t size = 0;
    char *buffer = NULL;
    int c;

    while ((c = fgetc(program)) != EOF) {
        if (size >= capacity) {
            capacity += 4096;
            char *tmp = realloc(buffer, capacity);
            if (tmp == NULL) {
                free(buffer);
                return NULL;
            }
            buffer = tmp;
        }

        buffer[size] = c;
        size++;
    }

    if (size % sizeof(int32_t) != 0) {
        free(buffer);
        return NULL;
    }

    size_t stack_memory_size = stack_capacity * sizeof(int32_t);
    int32_t *memory = (int32_t *) buffer;

    if (size + stack_memory_size > capacity) {
        while (size + stack_memory_size > capacity)
            capacity += 4096;

        memory = (int32_t *) realloc(buffer, capacity);
    }

    if (memory == NULL) {
        free(buffer);
        return NULL;
    }

    *stack_bottom = memory + (capacity / sizeof(int32_t)) - 1;
    memset(memory + size / sizeof(int32_t), 0, (capacity - size));

    return memory;
}


struct cpu *cpu_create(int32_t *memory, int32_t *stack_bottom, size_t stack_capacity) {
    assert(stack_bottom != NULL);
    assert(memory != NULL);
    struct cpu *cpu = calloc(1, sizeof(struct cpu));

    if (cpu == NULL) return NULL;

    cpu->memory = memory;
    cpu->stack_bottom = stack_bottom;
    cpu->stack_limit = stack_bottom - stack_capacity;
    return cpu;
}

int32_t cpu_get_register(struct cpu *cpu, enum cpu_register reg) {
    assert(cpu != NULL);
    assert(reg >= REGISTER_A && reg <= REGISTER_D);
    return (int32_t) cpu->registers[reg];
}

void cpu_set_register(struct cpu *cpu, enum cpu_register reg, int32_t value) {
    assert(cpu != NULL);
    assert(reg >= REGISTER_A && reg <= REGISTER_D);
    cpu->registers[reg] = value;
}

enum cpu_status cpu_get_status(struct cpu *cpu) {
    assert(cpu != NULL);
    return cpu->status;
}

int32_t cpu_get_stack_size(struct cpu *cpu) {
    assert(cpu != NULL);
    return cpu->stack_size;
}

void cpu_destroy(struct cpu *cpu) {
    assert(cpu != NULL);
    free(cpu->memory);
    memset(cpu, 0, sizeof(struct cpu));
}

void cpu_reset(struct cpu *cpu) {
    assert(cpu != NULL);
    set_registers_to_zero(cpu);
    cpu->status = CPU_OK;
    cpu->stack_size = 0;
    cpu->instruction_pointer = 0;
    memset(cpu->stack_limit + 1, 0, (cpu->stack_bottom - cpu->stack_limit) * sizeof(int32_t));
}

static int32_t get_register(struct cpu *cpu) {
    assert(cpu != NULL);
    int32_t reg = cpu->memory[cpu->instruction_pointer];
    if (reg < REGISTER_A || reg > REGISTER_D) {
        cpu->status = CPU_ILLEGAL_OPERAND;
        return 0;
    }
    return reg;
}

static int nop(struct cpu *cpu) {
    cpu->instruction_pointer++;
    return 1;
}

static int halt(struct cpu *cpu) {
    cpu->status = CPU_HALTED;
    return 0;
}

typedef int32_t (*operation_func)(int32_t a, int32_t b);
int32_t op_add(int32_t a, int32_t b) { return a + b; }
int32_t op_sub(int32_t a, int32_t b) { return a - b; }
int32_t op_mul(int32_t a, int32_t b) { return a * b; }
int32_t op_div(int32_t a, int32_t b) { return b != 0 ? a / b : 0; }

typedef int32_t (*unary_op_func)(int32_t);
int32_t op_inc(int32_t x) { return x + 1; }
int32_t op_dec(int32_t x) { return x - 1; }

static int operate(struct cpu *cpu, operation_func op) {
    assert(cpu != NULL);
    cpu->instruction_pointer++;

    int32_t reg = get_register(cpu);
    int32_t value = cpu_get_register(cpu, reg);
    int32_t curr_value = cpu_get_register(cpu, REGISTER_A);

    if (op == op_div && value == 0) {
        cpu->status = CPU_DIV_BY_ZERO;
    } else {
        cpu_set_register(cpu, REGISTER_A, op(curr_value, value));
    }

    cpu->instruction_pointer++;
    return 1;
}

static int unary_op(struct cpu *cpu, unary_op_func op) {
    assert(cpu != NULL);
    cpu->instruction_pointer++;

    int32_t reg = get_register(cpu);
    int32_t curr_value = cpu_get_register(cpu, reg);

    cpu_set_register(cpu, reg, op(curr_value));

    cpu->instruction_pointer++;
    return 1;
}

static int add(struct cpu *cpu) {
    return operate(cpu, op_add);
}

static int sub(struct cpu *cpu) {
    return operate(cpu, op_sub);
}

static int mul(struct cpu *cpu) {
    return operate(cpu, op_mul);
}

static int division(struct cpu *cpu) {
    return operate(cpu, op_div);
}

static int inc(struct cpu *cpu) {
    return unary_op(cpu, op_inc);
}

static int dec(struct cpu *cpu) {
    return unary_op(cpu, op_dec);
}

static int loop(struct cpu *cpu) {
    cpu->instruction_pointer++;
    int32_t value = cpu->memory[cpu->instruction_pointer];
    if (cpu_get_register(cpu, REGISTER_C) != 0) {
        cpu->instruction_pointer = value;
    }
    else {
        cpu->instruction_pointer++;
    }
    return 1;
}

static int movr(struct cpu *cpu) {
    cpu->instruction_pointer++;
    int32_t reg = get_register(cpu);
    cpu->instruction_pointer++;
    int32_t value = cpu->memory[cpu->instruction_pointer];
    cpu_set_register(cpu, reg, value);
    cpu->instruction_pointer++;
    return 1;
}

static int32_t *stack_addr(struct cpu *cpu, int32_t offset) {
    int32_t abs = -cpu->stack_size + 1 + cpu_get_register(cpu, REGISTER_D) + offset;
    if (cpu->stack_size == 0 || abs < -(cpu->stack_size - 1) || abs > 0) {
        return NULL;
    }
    return &cpu->stack_bottom[abs];
}

static int load(struct cpu *cpu) {
    cpu->instruction_pointer++;
    int32_t reg = get_register(cpu);
    cpu->instruction_pointer++;
    int32_t num = cpu->memory[cpu->instruction_pointer];

    int32_t *slot = stack_addr(cpu, num);
    if (slot == NULL) {
        cpu->status = CPU_INVALID_STACK_OPERATION;
        return 0;
    }
    cpu_set_register(cpu, reg, *slot);
    cpu->instruction_pointer++;
    return 1;
}

static int store(struct cpu *cpu) {
    cpu->instruction_pointer++;
    int32_t reg = get_register(cpu);
    cpu->instruction_pointer++;
    int32_t num = cpu->memory[cpu->instruction_pointer];

    int32_t *slot = stack_addr(cpu, num);
    if (slot == NULL) {
        cpu->status = CPU_INVALID_STACK_OPERATION;
        return 0;
    }
    *slot = cpu_get_register(cpu, reg);
    cpu->instruction_pointer++;
    return 1;
}

static int in(struct cpu *cpu) {
    cpu->instruction_pointer++;
    int32_t reg = get_register(cpu);
    int32_t value;

    int result = scanf("%d", &value);

    if (result == EOF) {
        cpu_set_register(cpu, REGISTER_C, 0);
        cpu_set_register(cpu, reg, -1);
        cpu->instruction_pointer++;
        return 1;
    }
    if (result != 1) {
        cpu->status = CPU_IO_ERROR;
        return 0;
    }

    cpu_set_register(cpu, reg, value);
    cpu->instruction_pointer++;
    return 1;
}

static int get(struct cpu *cpu) {
    cpu->instruction_pointer++;
    int32_t reg = get_register(cpu);
    int ch = getchar();
    if (ch == EOF) {
        cpu_set_register(cpu, REGISTER_C, 0);
        cpu_set_register(cpu, reg, -1);
    }
    else {
        cpu_set_register(cpu, reg, ch);
    }
    cpu->instruction_pointer++;
    return 1;
}

static int out(struct cpu *cpu) {
    cpu->instruction_pointer++;
    int32_t reg = get_register(cpu);
    int32_t value = cpu_get_register(cpu, reg);
    printf("%d", value);
    cpu->instruction_pointer++;
    return 1;
}

static int put(struct cpu *cpu) {
    cpu->instruction_pointer++;
    int32_t reg = get_register(cpu);
    int32_t value = cpu_get_register(cpu, reg);
    if (value >= 0 && value <= 255) {
        putchar(value);
    }
    else {
        cpu->status = CPU_ILLEGAL_OPERAND;
        return 0;
    }
    cpu->instruction_pointer++;
    return 1;
}


static int swap(struct cpu *cpu) {
    cpu->instruction_pointer++;
    int32_t reg1 = get_register(cpu);
    cpu->instruction_pointer++;
    int32_t reg2 = get_register(cpu);
    int32_t val1 = cpu_get_register(cpu, reg1);
    int32_t val2 = cpu_get_register(cpu, reg2);
    cpu_set_register(cpu, reg1, val2);
    cpu_set_register(cpu, reg2, val1);
    cpu->instruction_pointer++;
    return 1;
}

static int cpu_push(struct cpu *cpu, int32_t value)
{
    if (cpu->stack_bottom - cpu->stack_limit == cpu->stack_size) {
        return 0;
    }
    cpu->stack_bottom[-cpu->stack_size] = value;
    cpu->stack_size++;
    return 1;
}

static int cpu_pop(struct cpu *cpu, int32_t *value)
{
    assert(cpu != NULL);
    if (cpu->stack_size == 0) {
        return 0;
    }
    *value = cpu->stack_bottom[-cpu->stack_size + 1];
    cpu->stack_size--;
    cpu->stack_bottom[-cpu->stack_size] = 0;
    return 1;
}

static int push(struct cpu *cpu)
{
    cpu->instruction_pointer++;
    int32_t reg = get_register(cpu);
    if (!cpu_push(cpu, cpu_get_register(cpu, reg))) {
        cpu->status = CPU_INVALID_STACK_OPERATION;
        return 0;
    }
    cpu->instruction_pointer++;
    return 1;
}

static int pop(struct cpu *cpu)
{
    cpu->instruction_pointer++;
    int32_t reg = get_register(cpu);
    int32_t value;
    if (!cpu_pop(cpu, &value)) {
        cpu->status = CPU_INVALID_STACK_OPERATION;
        return 0;
    }
    cpu_set_register(cpu, reg, value);
    cpu->instruction_pointer++;
    return 1;
}

typedef int (*instruction_handler)(struct cpu *cpu);
instruction_handler instructions[] = {
    nop,
    halt,
    add,
    sub,
    mul,
    division,
    inc,
    dec,
    loop,
    movr,
    load,
    store,
    in,
    get,
    out,
    put,
    swap,
    push,
    pop
};

 int cpu_step(struct cpu *cpu) {
     assert(cpu != NULL);
    if (cpu->status != CPU_OK) {
         return 0;
     }
    if (cpu->instruction_pointer < 0) {
        cpu->status = CPU_INVALID_ADDRESS;
        return 0;
    }
     size_t address = cpu->instruction_pointer;
     size_t memory_size = cpu->stack_limit - cpu->memory + 1;
     if (address >= memory_size) {
         cpu->status = CPU_INVALID_ADDRESS;
         return 0;
     }

     int32_t instruction = cpu->memory[address];

     if (instruction >= 0 && instruction < (int32_t)(sizeof(instructions) / sizeof(instructions[0]))) {
         return instructions[instruction](cpu);
     }
     cpu->status = CPU_ILLEGAL_INSTRUCTION;
     return 0;
 }

long long cpu_run(struct cpu *cpu, size_t steps) {
    assert(cpu != NULL);
    size_t executed_steps = 0;
    int ret = 1;
    if (cpu->status != CPU_OK) {
        return 0;
    }
    while (executed_steps < steps && ret == 1) {
        ret = cpu_step(cpu);
        executed_steps++;
        if (cpu->status != CPU_HALTED && cpu->status != CPU_OK) {
            return -executed_steps;
        }
    }
    return executed_steps;
}
