#include <stdio.h> // 입출력
#include <string.h> // 문자열 비교
#include <stdlib.h> // 메모리 관련 및 exit()
#include <unistd.h> // 유닉스 시스템에 사용하는 표준 인터페이스
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <uuid/uuid.h>


#define SZ_STR_BUF 256 // 일반 문자열 배열의 길이

char *cmd;
char *argv[100];
char *optv[10];
int argc, optc;
char cur_work_dir[SZ_STR_BUF]; // 현재 디렉토리 위치 저장용

#define AC_LESS_1 -1 //명령어 인자 개수가 0 또는 1인 경우
#define AC_ANY -100 //명령어 인자 개수 제한 없는 경우 (echo처럼)
#define EQUAL(_s1, _s2) (strcmp(_s1, _s2) == 0) // 문자열이 같으면 ture
#define NOT_EQUAL(_s1, _s2) (strcmp(_s1, _s2) != 0) //문자열이 다르면 ture
#define PRINT_ERR_RET() do{ perror(cmd); return; }while(0)

static void print_usage(char *msg, char *cmd, char *opt, char *arg) {
	//msg 는 "사용법 : " 글을 받음 proc_cmd() 함수 정의 보셈
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

	if ((cmd = strtok(cmd_line, " \t\n")) == NULL ) //첫 토큰 잘라 냄
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
	if((++cnt % num_per_line) != 0)
		printf("\n");
}

void help();

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
	printf("현재 이 명령어는 구현되지 않았습니다.\n"); 
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
	exit(0);// exit(0)함수는 unix가 제공해준다
}

void makedir() {
	if (mkdir(argv[0], 0755) < 0 )
		PRINT_ERR_RET();
}
void mv(void) {
	if((link(argv[0], argv[1]) < 0) || (unlink(argv[0]) < 0))
		PRINT_ERR_RET();
	
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
	char *cmd;					//명령어 문자열 시작 주소
	void (*func)(void);	//명령어 처리하는 함수 포인터(함수 이름, 명령어 주소)
	int argc;						//명령어 인자의 개수(명령어와 옵션은 제외)
	char *opt;					//옵션 문자열 시작 주소: 예) "-a",옵션 없으면 ""
	char *arg;					//명령어 사용법 출력할 떄 사용할 명령어 인자 문자열 시작 주소
} cmd_tbl_t;

cmd_tbl_t cmd_tbl [] = {
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
	{ "mkdir",	makedir,	1,	"", "디렉토리이름" },
	{ "mv", mv, 2, "", "원본파일 바뀐이름" },
	{ "whoami", whoami, 0, "", "" },
	{ "uname", unixname, AC_LESS_1, "-a", ""},
	{ "rmdir", removedir, 1, "", "디렉토리이름"},	
};

int num_cmd = sizeof(cmd_tbl) / sizeof(cmd_tbl[0]); // 전체크기 vs 한칸의 크기

void help() {
	int k;
	printf( "현재 지원되는 명령어 종류입니다. \n");
	for (k = 0; k < num_cmd; ++k)
		print_usage("	", cmd_tbl[k].cmd, cmd_tbl[k].opt, cmd_tbl[k].arg);
	printf("\n");
}

void proc_cmd() {
	int k;
	for (k = 0; k < num_cmd; ++k) { // 입력한 명령어 정보를 cmd_tbl[]에서 찾음
		//전역변수 char *cmd; 사용자가 입력한 명령어 문자열 주소. 예) cmd ->ls
		if (EQUAL(cmd, cmd_tbl[k].cmd)) {
			if ((check_arg(cmd_tbl[k].argc) < 0) || (check_opt(cmd_tbl[k].opt) < 0))
			// 인자 개수 또는 옵션이 잘못된 경우임 : 사용법이 출력
				print_usage("사용법 : " , cmd_tbl[k].cmd, cmd_tbl[k].opt, cmd_tbl[k].arg);
				
			else
				cmd_tbl[k].func();
			return;
		}
	}
	printf("%s : 지원되지 않은 명령어 입니다.\n", cmd);
}
	

int main(int argc, char *argv[]) {
	int cmd_count = 1;
	char cmd_line[SZ_STR_BUF];
	
	setbuf(stdout, NULL); //표준 출력 버퍼 제거: printf()즉시 화면 출력
	setbuf(stderr, NULL); //표준 에러 출력 버퍼 제거
	
	help();
	getcwd(cur_work_dir, SZ_STR_BUF);
	
	for( ; ; ) {
		printf("<%s> %d : ", cur_work_dir, cmd_count);
		fgets(cmd_line, SZ_STR_BUF, stdin); // 키보드에서 한 행 전체를 입력 받아 cmd_line에 저장
		if ( get_argv_optv(cmd_line) !=NULL) {// 입력받은 문자를 개별 문자열로 분리
			proc_cmd();
			cmd_count++;
		}
	}
}





