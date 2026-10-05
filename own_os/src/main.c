#include "includes/semihost.h"

#define BUF_SIZ 16

typedef struct {
	char alias;
	int (*fun)(int, int);
} operation;

int no_op(int a __attribute__((unused)), int b __attribute__((unused))) {
	sh_printf("No such operation\n");
	return 0;
}

int add(int a, int b) { return a + b; }

int sub(int a, int b) { return a - b; }

int mul(int a, int b) { return a * b; }

int div(int a, int b) { return a / b; }

operation default_ops[] = {
	{
		.alias = '\0',
		.fun = no_op 
	},
	{
		.alias = '+',
		.fun = add
	},
	{
		.alias = '-',
		.fun = sub 
	},
	{
		.alias = '*',
		.fun = mul 
	},
	{
		.alias = '/',
		.fun = div 
	}
};

#define NUM_OPS ((sizeof(default_ops)) / sizeof(operation))

operation find_op(char c) {
	for(int i = 0; i < NUM_OPS; i++) {
		operation op = default_ops[i];
		if(op.alias == c) return op;
	}

	return default_ops[0]; // no op
}

int main(void) {
	int a, b;
	char buf[BUF_SIZ];

	for(;;) {
		sh_printf("Enter the first number: ");
		sh_gets(buf, BUF_SIZ);
		a = atoi(buf);
		
		sh_printf("Enter the second number: ");
		sh_gets(buf, BUF_SIZ);
		b = atoi(buf);

		sh_printf("Enter the operation: ");
		sh_gets(buf, 2);
		char op_code = buf[0];

		operation op = find_op(op_code);
		int r = op.fun(a, b);

		sh_printf("Result is: %i\n\n", r);
	}

	return 0;
}
