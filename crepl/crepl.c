#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>

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
//		sprintf(stderr,"wrong input");
		return 0;
	}

	char tmp=line[3];
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
	size_t func_len = strlen(function_def);
	if(write(fd,function_def,func_len)!=(ssize_t)func_len){
		close(fd);
		unlink(path);
		return false;
	}
	close(fd);

	char so_path[] = "/tmp/tmp-XXXXXX.so";
	int so_fd = mkstemps(so_path,3);
	if (so_fd<0){
		unlink(so_path);
		unlink(path);
		return false;
	}
	close(so_fd);

//	int fd
	pid_t pid = fork();
	if(pid==0){
//		char args[]={"gcc","-shared"," -fPIC", path,"-o",};

		int devnull = open("/dev/null", O_WRONLY);
		if (devnull >= 0) {
			dup2(devnull, STDOUT_FILENO);
			dup2(devnull, STDERR_FILENO);
			close(devnull);
		}

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
	if (!WIFEXITED(status) ||WEXITSTATUS(status) != 0) {
		unlink(path);
		unlink(so_path);
		return false;
	}

	void* handle = dlopen(so_path, RTLD_NOW | RTLD_GLOBAL);
	if (!handle) {
		fprintf(stderr, "dlopen: %s\n", dlerror());
		unlink(path);
		unlink(so_path);
		return false;
	}

//	int lparent_pos = strchr(function_def,"(");
//	int start_pos=0;
//	const char*p =  skip_space((function_def)+3);
	const char *p = skip_space(function_def);
	if (strncmp(p, "int", 3) != 0) {
		unlink(path);
		unlink(so_path);
		return false;
	}
	p = skip_space(p + 3);

	const char *start=p;
	while (isalnum((unsigned char)*p) || *p=='_')  p++;
	size_t n=(size_t)(p-start);

	char tmp[128];
//	int tmp_len = strlen(p);

	if (n==0 || n>= sizeof(tmp)){
		fprintf(stderr,"len too");
		unlink(path);
		unlink(so_path);
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

    return true;
}


// Evaluate an expression
bool evaluate_expression(const char* expression, int* result) {
//	char wrapper="__expr_wrapper_";
//	snprintf(wrapper,sizeof (wrapper),"%s",);
	static int expr_count = 0;
	char sym[128];
	snprintf(sym,sizeof (sym),"__expr__wrapper_%d",expr_count++);

	char src [1024]={0};
	int len = snprintf(src,sizeof (src),"int %s(void) {return (%s);}\n",sym,expression);
	if(len<0 ||(size_t)len>=sizeof(src)){
		fprintf(stderr,"too long\n");
		return  false;
	}

//	write(fd,)
	char path[] = "/tmp/wrap-.cXXXXXX";
//	char path="tmp\tmp"
	int fd = mkstemp(path);
	if(fd<0){
		perror("mkstemp");
		close(fd);
		return false;
	}


	size_t slen = strlen(src);
	if (write(fd, src, slen) != (ssize_t)slen) {
		close(fd);
		unlink(path);
		return false;
	}

	close(fd);

	char so_path[] = "/tmp/wrap-XXXXXX.so";
//	char path="tmp\tmp"
	int so_fd = mkstemps(so_path,3);
	if(so_fd<0){
		perror("mkstemp");
		close(so_fd);
		unlink(path);
		return false;
	}

	close(so_fd);

	pid_t pid = fork();
	if(pid ==0){

		int devnull = open("/dev/null", O_WRONLY);
		if (devnull >= 0) {
			dup2(devnull, STDOUT_FILENO);
			dup2(devnull, STDERR_FILENO);
			close(devnull);
		}

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
	if(waitpid(pid,&status,0)<0){
		unlink(path);
		unlink(so_path);
		return false;
	}
	// 编译失败
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		unlink(path);
		unlink(so_path);
		return false;
	}

	void* handle = dlopen(so_path, RTLD_NOW | RTLD_GLOBAL);
	if (!handle) {
		fprintf(stderr, "dlopen: %s\n", dlerror());
		unlink(path); unlink(so_path);
		return false;
	}

	void *res = dlsym(handle,sym);
	if(!res) {
		unlink(path);
		unlink(so_path);
		return false;
	}
	int (*fn)(void) = (int (*)(void))res;
	*result = fn();


	unlink(path);
	unlink(so_path);


	return true;
}


int main() {
	char buf[256];
	while (1){
		printf("crepl> ");
		fflush(stdout);

		if (!fgets(buf,sizeof (buf),stdin))
			break;

		const char *s = skip_space(buf);
		if (*s == '\0') continue;



		size_t buf_len = strlen(buf);
//		if(buf_len>100)
////			fprintf(stderr,"wrong args");
//			return 0;

		if (buf_len>0 &&buf[buf_len-1]=='\n')
			buf[buf_len-1]='\0';

		if(is_fun(buf)){
			if(compile_and_load_function(buf)){
				printf("OK.\n");
			}else{
				fprintf(stderr,"error\n");
			}
		}else{
			int value;
			if(evaluate_expression(buf,&value)){
				printf("= %d.\n",value);
			}else{
				fprintf(stderr,"error\n");
			}
		}
	}

	return 0;

}
