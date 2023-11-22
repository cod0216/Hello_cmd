#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <utime.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>



#define SZ_FILE_BUF 1024 
#define SZ_STR_BUF 256 

char *cmd;
char *argv[100];
char *optv[10];
int argc, optc;
char cur_work_dir[SZ_STR_BUF]; 
#define AC_LESS_1 -1 
#define AC_ANY -100 
#define EQUAL(_s1, _s2) (strcmp(_s1, _s2) == 0) 
#define NOT_EQUAL(_s1, _s2) (strcmp(_s1, _s2) != 0) 
#define PRINT_ERR_RET() do{ perror(cmd); return; }while(0)

static void print_usage(char *msg, char *cmd, char *opt, char *arg) {
	printf("%s	%s", msg, cmd);
	if (NOT_EQUAL(opt, ""))
		printf("	%s", opt);
	printf("	%s", arg);
	printf("\n");
}

static int check_arg(int count) {
	if (count < 0) {
		count = -count;
		if(argc <= count)
			return(0);
	}
	
	if (argc == count)
		return 0;

	if (argc > count)
		printf("불필요한 명령어 인자가 있습니다.\n");
	
	else 
		printf("명령어 인자의 수가 부족합니다.\n");
	return -1;

}

static int check_opt(char *opt) {
	int i, err = 0;
	for (i = 0; i < optc; ++i) {
		if( NOT_EQUAL(opt, optv[i])) {
			printf("지원되지 않는 명령어 옵션(%s)입니다. \n", optv[i]);
			err=-1;
		}
	}
	return(err);
}

static char *get_argv_optv(char *cmd_line) {
	char *tok;
	argc = optc = 0;

	if ((cmd = strtok(cmd_line, " \t\n")) == NULL ) 
		return (NULL);

	for ( ; (tok = strtok(NULL, " \t\n")) != NULL; ) {
		if(tok[0] == '-')
			optv[optc++] = tok;
		else
			argv[argc++] = tok;
	}
	return(cmd);
}
static void print_attr(char *path, char *fn) {
	struct passwd *pwp;
	struct group *grp;
	struct stat st_buf;
	char full_path[SZ_STR_BUF], buf[SZ_STR_BUF], c;
	char time_buf[13];
	struct tm *tmp;

	sprintf(full_path, "%s/%s", path, fn);
	if(lstat(full_path, &st_buf) < 0)
			PRINT_ERR_RET();
	if	(S_ISREG(st_buf.st_mode)) c = '-';
	else if	(S_ISDIR(st_buf.st_mode)) c = 'd';
	else if	(S_ISCHR(st_buf.st_mode)) c = 'c';
	else if	(S_ISBLK(st_buf.st_mode)) c = 'b';
	else if	(S_ISFIFO(st_buf.st_mode)) c = 'f';
	else if	(S_ISLNK(st_buf.st_mode)) c = 'l';
	else if	(S_ISSOCK(st_buf.st_mode)) c = 's';
	buf[0] = c;
	buf[1] = (st_buf.st_mode & S_IRUSR)? 'r': '-';
	buf[2] = (st_buf.st_mode & S_IWUSR)? 'w': '-';
	buf[3] = (st_buf.st_mode & S_IXUSR)? 'x': '-';
	buf[4] = (st_buf.st_mode & S_IRGRP)? 'r': '-';
	buf[5] = (st_buf.st_mode & S_IWGRP)? 'w': '-';
	buf[6] = (st_buf.st_mode & S_IXGRP)? 'x': '-';
	buf[7] = (st_buf.st_mode & S_IROTH)? 'r': '-';
	buf[8] = (st_buf.st_mode & S_IWOTH)? 'w': '-';
	buf[9] = (st_buf.st_mode & S_IXOTH)? 'x': '-';
	buf[10] = '\0'; 
	pwp = getpwuid(st_buf.st_uid);
	grp = getgrgid(st_buf.st_gid);
	tmp = localtime(&st_buf.st_mtime);
	strftime(time_buf, 13, "%b %d %H %M", tmp);
	sprintf(buf+10, " %3ld %-8s %-8s %8ld %s %s", st_buf.st_nlink, pwp->pw_name, grp->gr_name, st_buf.st_size, time_buf, fn);
	if (S_ISLNK(st_buf.st_mode)) {
		int len, bytes;
		strcat(buf, " -> ");
		len = strlen(buf);
		bytes = readlink(full_path, buf+len, SZ_STR_BUF-len);
		buf[len+bytes] = '\0';
	}
	printf("%s\n", buf);
}

static void print_detail(DIR *dp, char *path) {
	struct dirent *dirp;

	while ((dirp = readdir(dp)) != NULL) 
		print_attr(path, dirp->d_name);	
}

static void get_max_name_len(DIR *dp, int *p_max_name_len, int *p_num_per_line) {
	struct dirent *dirp;

	int max_name_len = 0;

	while((dirp = readdir(dp)) != NULL) {
		int name_len = strlen(dirp->d_name);
		if (name_len > max_name_len)
			max_name_len = name_len;
	}
	rewinddir(dp);
	max_name_len +=4;
	*p_num_per_line = 80 / max_name_len;
	*p_max_name_len = max_name_len;
}

static void print_name(DIR *dp) {
	struct dirent *dirp;
	int max_name_len, num_per_line, cnt = 0;
	
	get_max_name_len(dp, &max_name_len, &num_per_line);
	
	while((dirp=readdir(dp)) != NULL) {
		printf("%-*s", max_name_len, dirp->d_name);
		if((++cnt % num_per_line) == 0)
			printf("\n");
	}
	if((cnt % num_per_line) != 0)
		printf("\n");
}

void help();

void cat(void) {
	int fd, len;
	char buf[SZ_FILE_BUF];

	if((fd = open(argv[0], O_RDONLY)) < 0)
		PRINT_ERR_RET();
	while ((len = read(fd, buf, SZ_FILE_BUF)) > 0) {
		if (write(STDOUT_FILENO, buf, len) != len) {
			len = -1;
			break;
		}
	}
	if (len < 0 )
		perror(cmd);
	close(fd);
}

void cd(void) {
	if(argc == 0) {
		struct passwd *pwp;
		pwp = getpwuid(getuid());
		argv[0] = pwp->pw_dir;
	}
	if(chdir(argv[0]) <0)
		PRINT_ERR_RET();
	else
		getcwd(cur_work_dir, SZ_STR_BUF);

}

void changemod(void) {
	int mode;
	sscanf(argv[0], "%o", &mode);
	if(chmod(argv[1], (mode_t)mode) < 0)
		PRINT_ERR_RET();

}

void cp(void) {
	int rfd, wfd, len;
	char buf[SZ_FILE_BUF];
	struct stat st_buf;

	if ((stat(argv[0], &st_buf) < 0) || ((rfd = open(argv[0], O_RDONLY)) < 0))
		PRINT_ERR_RET();
	if((wfd = creat(argv[1], st_buf.st_mode)) < 0) {
			perror(cmd);
			close(rfd);
			return;
	}
	while ((len = read(rfd, buf, SZ_FILE_BUF)) > 0 ) {
		if(write(wfd, buf, len) != len) {
			len = -1;
			break;
		}
	}
	if (len < 0 )
		perror(cmd);
	close(rfd);
	close(wfd);
}
	
void date(void) {
	char stm[128];
	time_t ttm = time(NULL);
	struct tm *ltm = localtime(&ttm);
	char *atm = asctime(ltm);
	char *ctm = ctime(&ttm);
	strftime(stm, 128, "stm : %a %b %d %H:%M:%S %Y", ltm);
	printf("atm : %s", atm);
	printf("ctm : %s", ctm);
	puts(stm); 
}

void echo(void) {
	int i;
	for(i = 0; i < argv[i]; i++)
		printf("%s ", argv[i]);
	printf("\n");

}

void hostname(void){
	char hostname[SZ_STR_BUF];
	gethostname(hostname, SZ_STR_BUF);
	printf("%s \n", hostname);

}

void id() {
	struct passwd *pwp;
	struct group *grp;

	/*if (argc == 1)
		pwp = getpwnam(argv[0]);

	else
		pwp = getpwuid(getuid());

	if ( pwp == NULL )	{
		printf( " id : 잘못된 사용자 이름 :  \" %s \" \n", argv[0]);
		return ;
	}

	grp = getgrgid(pwp->pw_gid);
	if (grp == NULL) {
		printf( " id : 잘못된 사용자 이름 :  \" %s \" \n", argv[0]);
		return ;
	}
*/

	pwp = ((argc == 1) ? getpwnam(argv[0]) : getpwuid(getuid()));
	
	if (( pwp == NULL) || ( (grp = getgrgid(pwp->pw_gid)) == NULL)) {
		printf( " id : 잘못된 사용자 이름 :  \" %s \" \n", argv[0]);
		return ;
	}
	printf(" uid=%d(%s) gid=%d(%s) 그룹들=%d(%s) \n", pwp->pw_uid, pwp->pw_name, grp->gr_gid, grp->gr_name, grp->gr_gid, grp->gr_name);

}

void ls(void) {
	char *path;
	DIR *dp;

	path = (argc == 0) ? "." : argv[0];

	if ((dp = opendir(path)) == NULL)
		PRINT_ERR_RET();
	if(optc == 0)
		print_name(dp);
	else
		print_detail(dp, path);
	closedir(dp);
}

void pwd(void) {
	printf("%s \n", cur_work_dir); 
}

void rm(void) {
	
	struct stat buf;
/*
	if (lstat(argv[0], &buf) < 0)
		PRINT_ERR_RET();

	int ret;
	if(S_ISDIR(buf.st_mode) ? ret = rmdir(argv[0]) :(ret = unlink(argv[0])) < 0 )
		PRINT_ERR_RET();
*/
	int ret;
	if ( (lstat(argv[0], &buf)) || (S_ISDIR(buf.st_mode) ? (ret =rmdir(argv[0])) : (ret = unlink(argv[0]) < 0)))
		PRINT_ERR_RET();

}

void removedir(void) {
if (rmdir(argv[0]) < 0)
		PRINT_ERR_RET();

}

void quit(void) {
	exit(0);
}

void Sleep() {
	int sec;
	if(sscanf(argv[0], "%d", &sec) < 1)
		PRINT_ERR_RET();
	else
		sleep(sec);

}

void makedir() {
	if (mkdir(argv[0], 0755) < 0 )
		PRINT_ERR_RET();
}
void mv(void) {
	if((link(argv[0], argv[1]) < 0) || (unlink(argv[0]) < 0))
		PRINT_ERR_RET();
	
}

void touch(void) {
	
	int fd;

	if((utime(argv[0], NULL)) == 0)
		return;

	if (errno != ENOENT) {
		PRINT_ERR_RET();
	}

	if((fd = creat(argv[0], 0644)) < 0)
		PRINT_ERR_RET();

	close(fd);

}

void whoami(void) {
/*char *username;
	username = getlogin();
	if (username == NULL)
		printf("터미널 장치가 아니라서 사용자 계정정보를 구할 수 없습니다.\n");
	else
		printf("%s \n", username);
*/

	struct passwd *pwp;
	pwp = getpwuid(getuid());
	printf("%s \n", pwp->pw_name);

}

void unixname() {
	struct utsname un;
	uname(&un);
	if ( optc == 1)
		printf("%s %s %s %s %s \n", un.sysname, un.nodename, un.release, un.version, un.machine);
	else printf("%s \n", un.sysname);
}	

typedef struct {
	char *cmd;				
	void (*func)(void);	
	int argc;						
	char *opt;				
	char *arg;					
} cmd_tbl_t;

cmd_tbl_t cmd_tbl [] = {
	{ "cat",	cat,	1,	"", "파일이름" },
	{ "cd",	cd,	AC_LESS_1,	"",	"[디렉토리이름]" },
	{ "chmod", changemod, 2, "", "8진수모드값 파일이름"},
	{ "cp",	cp,	2,	"",	"원본파일 복사된 파일" },
	{ "date", date, 0, "" , "" },
	{ "echo",	echo,	AC_ANY,	"",	"[에코할 문장]" },
	{ "help", help, 0, "", "" },
	{ "hostname", hostname, 0, "", "" },
	{ "id", id, AC_LESS_1, "", "[계정이름]" },
	{ "ls",	ls,	AC_LESS_1,	"-l",	"[디렉토리이름]" },
	{ "pwd",	pwd,	0,	"",	"" },
	{ "exit", quit, 0, "", "" },
	{ "rm",	rm,	1,	"", "파일이름" },
	{ "sleep",	Sleep,	1,	"", "초단위시간" },
	{ "mkdir",	makedir,	1,	"", "디렉토리이름" },
	{ "mv", mv, 2, "", "원본파일 바뀐이름" },
	{ "touch",	touch,	1,	"", "파일이름" },
	{ "whoami", whoami, 0, "", "" },
	{ "uname", unixname, AC_LESS_1, "-a", ""},
	{ "rmdir", removedir, 1, "", "디렉토리이름"},	
};

int num_cmd = sizeof(cmd_tbl) / sizeof(cmd_tbl[0]); 

void help() {
	int k;
	printf( "현재 지원되는 명령어 종류입니다. \n");
	for (k = 0; k < num_cmd; ++k)
		print_usage("	", cmd_tbl[k].cmd, cmd_tbl[k].opt, cmd_tbl[k].arg);
	printf("\n");
}

void run_cmd() {
	pid_t pid;
	if((pid = fork()) < 0)
		PRINT_ERR_RET();

	else if (pid == 0) {
		int i, cnt = 0;
		char *nargv[100];
		nargv[cnt++] = cmd;
		for (i = 0; i< optc; ++i)
			nargv[cnt++] = optv[i];
		for (i = 0; i< argc; ++i)
			nargv[cnt++] = argv[i];
		nargv[cnt++] = NULL;

		if ( execvp(cmd,nargv) < 0) {
			perror(cmd);
			exit(1);
		}
	}
	else {
		if ( waitpid(pid, NULL, 0) < 0 )
			PRINT_ERR_RET();
	}
}

void proc_cmd() {
	int k;
	for (k = 0; k < num_cmd; ++k) { 
		if (EQUAL(cmd, cmd_tbl[k].cmd)) {
			if ((check_arg(cmd_tbl[k].argc) < 0) || (check_opt(cmd_tbl[k].opt) < 0))
				print_usage("사용법 : " , cmd_tbl[k].cmd, cmd_tbl[k].opt, cmd_tbl[k].arg);
				
			else
				cmd_tbl[k].func();
			return;
		}
	}
	run_cmd();
}
	
int main(int argc, char *argv[]) {
	int cmd_count = 1;
	char cmd_line[SZ_STR_BUF];
	
	setbuf(stdout, NULL); 
	setbuf(stderr, NULL); 
	
	help();
	getcwd(cur_work_dir, SZ_STR_BUF);
	
	for( ; ; ) {
		printf("<%s> %d : ", cur_work_dir, cmd_count);
		fgets(cmd_line, SZ_STR_BUF, stdin); 
		if ( get_argv_optv(cmd_line) !=NULL) {
			proc_cmd();
			cmd_count++;
		}
	}
}





