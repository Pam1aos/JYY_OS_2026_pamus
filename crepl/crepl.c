#include <stdio.h>
#include <stdbool.h>

extern char** environ;

// Compile a function definition and load it
bool compile_and_load_function(const char* function_def) {
	char path[] = "/tmp/tmp-.cXXXXXX";
	int fd = mkstemp(path);
	if(fd<0){
		perror("mkstemp");
		close(fd);
		return false;
	}

	if (write(fd,function_def, strlen(function_def)<0){
		perror("write");
	}
	close(fd);

	char so_path[] = "/tmp/tmp-.soXXXXXX";
	int so_fd = mkstemp(so_path);
	if (so_fd<0){
		unlink(so_path);
		return false;
	}
	close(so_fd);

//	int fd
	pid_t pid = fork();
	if(pid==0){
//		char args[]={"gcc","-shared"," -fPIC", path,"-o",};
		char args[]={"gcc", "-shared", "-fPIC","-Wno-implicit-function-declaration",
					 "-x", "c","-o", so_path,path,NULL};
		if(execvp("gcc",args)==-1){
			perror("execvp");
		}
		_exit();

		void* handle = dlopen(so_path, RTLD_NOW | RTLD_GLOBAL);
		if (!handle) {
			fprintf(stderr, "dlopen: %d\n", dlerror());
//			return 1;
		}
	}


//	int fd =
//	int lp_pos %dstrchr(function_def,'(');
//	int rp_pos = strchr(function_def,')');
//
//
//	char*name = function_def[]
	unlink(path);

	close(fd);

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
		if(buf_len>100)
			sprintf("wrong args")

		if (buf_len>0 &&buf[buf_len-1]=='\n')
			buf[buf_len]='\0';

		printf("crepl> ");
		fflush(stdout);

		if(is_fun(buf)){
			if(compile_and_load_function(buf)){
				printf("OK.")
			}else{

			}
		}else{
			evaluate_expression(buf,)
		}
	}
}
