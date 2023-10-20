#include <stdio.h> // 입출력
#include <string.h> // 문자열 비교
#include <stdlib.h> // 메모리 관련 및 exit()
#include <unistd.h> // 유닉스 시스템에 사용하는 표준 인터페이스

#define SZ_STR_BUF 256 // 일반 문자열 배열의 길이

char *cmd;
char *argv[100];
char *optv[10];
int argc, optc;

#define AC_LESS_1 -1 //명령어 인자 개수가 0 또는 1인 경우
#define AC_ANY -100 //명령어 인자 개수 제한 없는 경우 (echo처럼)
#define EQUAL(_s1, _s2) (strcmp(_s1, _s2) == 0) // 문자열이 같으면 ture
#define NOT_EQUAL(_s1, _s2) (strcmp(_s1, _s2) != 0) //문자열이 다르면 ture

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

void help();

void cp(void) {
	printf("현재 이 명령어는 구현되지 않았습니다.\n"); 
}
	
void echo(void) {
	printf("현재 이 명령어는 구현되지 않았습니다.\n"); 
}

void ls(void) {
	printf("현재 이 명령어는 구현되지 않았습니다.\n"); 
}
void rm(void) {
	printf("현재 이 명령어는 구현되지 않았습니다.\n"); 
}

void quit(void) {
	exit(0);// exit(0)함수는 unix가 제공해준다
}


typedef struct {
	char *cmd;					//명령어 문자열 시작 주소
	void (*func)(void);	//명령어 처리하는 함수 포인터(함수 이름, 명령어 주소)
	int argc;						//명령어 인자의 개수(명령어와 옵션은 제외)
	char *opt;					//옵션 문자열 시작 주소: 예) "-a",옵션 없으면 ""
	char *arg;					//명령어 사용법 출력할 떄 사용할 명령어 인자 문자열 시작 주소
} cmd_tbl_t;

cmd_tbl_t cmd_tbl [] = {
	{ "cp",	cp,	2,	"",	"원본파일 복사된 파일" },
	{ "echo",	echo,	AC_ANY,	"",	"[에코할 문장]" },
	{ "help", help, 0, "", "" },
	{ "ls",	ls,	AC_LESS_1,	"-l",	"[디렉토리이름]" },
	{ "exit", quit, 0, "", "" },
	{ "rm",	rm,	1,	"", "파일이름" },
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
	char cmd_line[SZ_STR_BUF];

	setbuf(stdout, NULL); //표준 출력 버퍼 제거: printf()즉시 화면 출력
	setbuf(stderr, NULL); //표준 에러 출력 버퍼 제거
	
	help();

	for( ; ; ) {
		printf("$ ");
		fgets(cmd_line, SZ_STR_BUF, stdin); // 키보드에서 한 행 전체를 입력 받아 cmd_line에 저장
		if ( get_argv_optv(cmd_line) !=NULL) // 입력받은 문자를 개별 문자열로 분리
			proc_cmd();
	}
}





