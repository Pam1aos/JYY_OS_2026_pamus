#include <stdio.h>
#include <stdbool.h>

// Compile a function definition and load it
bool compile_and_load_function(const char* function_def) {
	int lp_pos = strchr(function_def,'(');
	int rp_pos = strchr(function_def,')');

	char*name = function_def[]
    return false;
}

// Evaluate an expression
bool evaluate_expression(const char* expression, int* result) {
    return false;
}

static const char *skip_space(const char *p)
{
	while (*p && isspace((unsigned char)*p))
		p++;
	return p;
}

static int is_fun(const char *p){
	const char*line = skip_space(buf);
	if(strncmp(line,"int",3)!=0){
		sprintf("wrong input");
		return 0;
	}

	char tmp=p[3];
	if(tmp=='\0' || isspace((unsigned char )tmp)||tmp=='(')
		return 1;

	return 0;
}

int main() {
	char buf[256];
	while (fgets(buf,sizeof (buf),stdin)){
		size_t buf_len = strlen(buf);
		if (buf_len>0 &&buf[buf_len-1]=='\n')
			buf[buf_len]='\0';

		if(is_fun(buf)){
			compile_and_load_function(buf);
		}else{
			evaluate_expression(buf,)
		}
	}
}
