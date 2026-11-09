#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <ctype.h>

extern char** environ;

static const char *skip_space(const char *p)
{
	while (*p && isspace((unsigned char)*p))
		p++;
	return p;
}

static int is_fun(const char *p){
	const char*line = skip_space(p);
	if(strncmp(line,"int",3)!=0){
		sprintf(stderr,"wrong input");
		return 0;
	}

	char tmp=p[3];
	if(tmp=='\0' || isspace((unsigned char )tmp)||tmp=='(')
		return 1;

	return 0;
}

// Compile a function definition and load it
bool compile_and_load_function(const char* function_def) {
	char path[] = "/tmp/tmp-.cXXXXXX";
	int fd = mkstemp(path);
	if(fd<0){
		perror("mkstemp");
		close(fd);
		return false;
	}

//	if (write(fd,function_def, (ssize_t)(strlen(function_def)))){
//		perror("write");
//	}
	size_t n = strlen(function_def);
	if(write(fd,function_def,n)!=(ssize_t)n){
		perror("write");
	}
	close(fd);

	char so_path[] = "/tmp/tmp-XXXXXX.so";
	int so_fd = mkstemps(so_path,3);
	if (so_fd<0){
		unlink(so_path);
		return false;
	}
	close(so_fd);

//	int fd
	pid_t pid = fork();
	if(pid==0){
//		char args[]={"gcc","-shared"," -fPIC", path,"-o",};
		char *args[]={"gcc", "-shared", "-fPIC","-Wno-implicit-function-declaration",
					 "-x", "c","-o", so_path,path,NULL};
		if(execvp("gcc",args)==-1){
			unlink(path);
			unlink(so_path);
			perror("execvp");
		}
		_exit(127);
	}else if(pid<0){
		unlink(path);
		unlink(so_path);
		return false;
	}
	int status;
//	waitpid(...);

	if(waitpid(pid,&status,0)<0){
		unlink(path);
		unlink(so_path);
		return false;
	}
	// 编译失败
	if (WEXITSTATUS(status) != 0) {
		unlink(path);
		unlink(so_path);
		return false;
	}

	void* handle = dlopen(so_path, RTLD_NOW | RTLD_GLOBAL);
	if (!handle) {
		fprintf(stderr, "dlopen: %d\n", dlerror());
			return false;
	}

//	int lparent_pos = strchr(function_def,"(");
//	int start_pos=0;
//	const char*p =  skip_space((function_def)+3);
	const char *p = skip_space(function_def);
	if (strncmp(p, "int", 3) != 0) return false;
	p = skip_space(p + 3);

	const char *start=p;
	while (isalnum((unsigned char)*p) || *p=='_')  p++;
	size_t n=(size_t)(p-start);

	char tmp[128];
//	int tmp_len = strlen(p);

	if (n==0 || n>= sizeof(tmp)){
		fprintf(stderr,"len too");
		return false;
	}
//	snprintf(buf, sizeof(buf), "func: %s", name);
	snprintf(tmp,sizeof(tmp),"%.*s",(int)n,start);
//	for (int i = 0; i <tmp_len ; ++i) {
//		tmp[i] = p[i];
//	}
//	for()

	void *sym = dlsym(handle,tmp);
	if(!sym) {
		unlink(path);
		unlink(so_path);
		return false;
	}

//	void *dlsym(void *handle, const char *symbol);

//	int fd =
//	int lp_pos %dstrchr(function_def,'(');
//	int rp_pos = strchr(function_def,')');
//
//
//	char*name = function_def[]
	unlink(path);
	unlink(so_path);

	close(fd);

    return true;
}

// Evaluate an expression
bool evaluate_expression(const char* expression, int* result) {
    return false;
}


int main() {
	char buf[256];
	while (fgets(buf,sizeof (buf),stdin)){
		size_t buf_len = strlen(buf);
		if(buf_len>100)
			sprintf(stderr,"wrong args")

		if (buf_len>0 &&buf[buf_len-1]=='\n')
			buf[buf_len-1]='\0';

		printf("crepl> ");
		fflush(stdout);

		if(is_fun(buf)){
			if(compile_and_load_function(buf)){
				printf("OK.\n")
			}else{

			}
		}else{
			evaluate_expression(buf,)
		}
	}

	return 0;
}
