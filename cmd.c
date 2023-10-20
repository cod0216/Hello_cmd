#include <stdio.h> // 입출력
#include <string.h> // 문자열 비교
#include <stdlib.h> // 메모리 관련 및 exit()
#include <unistd.h> // 유닉스 시스템에 사용하는 표준 인터페이스

#define SZ_STR_BUF 256 // 일반 문자열 배열의 길이

char *cmd;
char *argv[100];
char *optv[10];
int argc, optc;

#define EQUAL(_s1, _s2) (strcmp(_s1, _s2) == 0) // 문자열이 같으면 ture
#define NOT_EQUAL(_s1, _s2) (strcmp(_s1, _s2) != 0) //문자열이 다르면 ture

static int check_arg(int count) { reutrn(0); }
static int check_opt(char *opt) { reutrn(0); }

static char *get_argv_optv(char *cmd_line) {
	return (cmd = strtok(cmd_line, " \t\n")); 
}

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

#define AC_LESS_1 -1 //명령어 인자 개수가 0 또는 1인 경우
#define AC_ANY -100 //명령어 인자 개수 제한 없는 경우 (echo처럼)

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
	{ "ls",	ls,	AC_LESS,	"-l",	"[디렉토리이름]" },
	{ "exit", quit, 0, "", "" },
	{ "rm",	rm,	1,	"", "파일이름" },
};

int num_cmd = sizeof(cmd_tbl) / sizeof(cmd_tbl[0]); // 전체크기 vs 한칸의 크기

int mait(int argc, char *argv[]) {
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





