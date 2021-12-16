#include <sqlite3.h>
#include "Todolist.h"

int main(void)
{
	sqlite3* db;
	int rc;
	sqlite3_stmt* res;
	int step;
	char* err_msg = 0;

	struct sockaddr_in ser, cli;
    	int sd, nsd, len, clen;
	// 받는 메시지
	char Rmsg[TotalMax];
	// 보낼 메시지
	char Smsg[TotalMax];

	// 시간데이터를 받아올 구조제와 데이터연동
	// 즉 날짜를 확인하기 위해 사용한다.
	time_t t;
	struct tm* tm;
	time(&t);
	tm = localtime(&t);
	
	// 메모리 초기화 및 소켓 구조체에 값을 저장한다.
        memset((char *)&ser, 0, sizeof(struct sockaddr_in));
        ser.sin_family = AF_INET;
        ser.sin_port = htons(PORTNUM);
        ser.sin_addr.s_addr = inet_addr("127.0.0.1");


	// socket 생성
    	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{
		// 문제가 발생하면 에러를 출력후 종료
        	perror("socket");
        	exit(1);
    	}

	// 소켓기술자와 소켈 주소 구조체를 연결한다.
    	if (bind(sd, (struct sockaddr *)&ser, sizeof(ser))) 
	{
		// 오류가 발생하면 에러를 출력후 종료
        	perror("bind");
        	exit(1);
    	}

	printf("클라이언트 접속 대기\n");
	// 클라이언트의 접속을 대기한다.
    	if (listen(sd, 5) < 0) 
	{
		// 에러가 발생하면 에러를 출력후 종료
        	perror("listen");
        	exit(1);
    	}
	printf("클라이언트 접속 수용\n");

	// 데이터 베이스 생성
        rc = sqlite3_open("Todolist.db", &db);
        // 해당 데이터 베이스를 연결 할 수 없다면
        if(rc !=  SQLITE_OK)
        {
                // standard error에 해당 메시지를 출력하고 프로그램을 종료한다.
                fprintf(stderr, "해당 데이터베이스를 열 수 없습니다. %s\n",sqlite3_errmsg(db));
                sqlite3_close(db);
                exit(1);
        }
	// 데이터 베이스 생성 쿼리문 데이터베이스가 있다면 제거하고 만든다.
	// 이유는 서버는 항상 동작하는 것을 기본 전제조건을 한다.
	// 컴파일과 동작 오류를 방지하기 위해 있다면 제거하고 생성
        char* query = "DROP TABLE IF EXISTS Lists;"
                "CREATE TABLE Lists (ID INTEGER PRIMARY KEY, EndDay INTEGER, Name TEXT, Description TEXT);";

	// Sqlite query문 실행
        rc = sqlite3_exec(db, query, 0, 0, &err_msg);

        // 명령어 실행이 실패했다면 standard err에 오류를 출력
        if( rc != SQLITE_OK)
        {
                fprintf(stderr, "테이블을 만드는데 실패했습니다.\n");
                fprintf(stderr, "SQL error: %s\n",err_msg);
                sqlite3_free(err_msg);
        }
	
	
	while(1)
	{
		// 클라이언트의 접속을 수용한다.
        	if ((nsd = accept(sd, (struct sockaddr *)&cli, &clen)) == -1)
        	{
                	// 에러가 발생하면 오류를 출력후 종료
                	perror("accept");
                	exit(1);
        	}
		printf("클라이언트 접속\n");
		while(1)
		{
			char command[4];
			memset(command,'\0',sizeof(command));
			// 클라이언트가 보낸 메시지를 읽기
			if (recv(nsd, command, sizeof(command), 0) == -1)
			{
				// 오류가 발생했다면 다시 메시지 읽기.
				perror("recve error\n");
				exit(1);
			}
			command[strlen(command)+1] = '\0';
			// 받은 명령문을 출력
			printf("%s\n", command);

			// 만약 클라이언트에서 종료를 보냈다면 서버 종료
			if(strcmp(command,"qq")== 0)
			{
				printf("quit client\n");
				break;
			}
			// 데이터 insert를 호출했다면
			else if(strcmp(command,"cd") == 0)
			{
				// 현재 지금 어떠한 명령을 실행중인지 출력
				printf("ok cd\n");

				int day;
				char Name[MAX];
				char Desct[MAXLEN];

				// SQLITE3 쿼리문 생성
				char* query = "INSERT INTO Lists(EndDay ,Name, Description) VALUES(?,?,?)";
			
				// client에서 보낸 day정보를 받기	
				if(recv(nsd, Rmsg, sizeof(Rmsg), 0) == -1)
				{
					// 오류가 발생하면 출력후 종효
					perror("recv error1\n");
					exit(1);
				}
				// day를 integer로 변경
				day = atoi(Rmsg);

				// client에서 보낸 name정보 받기 
				if(recv(nsd, Rmsg, sizeof(Rmsg), 0) == -1)
				{     
					// 오류가 발생하면 에러를 출력하고 종료
					perror("recv error2\n");
					exit(1);
				}
				// 정보 복사
				strcpy(Name,Rmsg);

				// client로부터 세부설명 정보 받기
				if(recv(nsd, Rmsg, sizeof(Rmsg), 0) == -1)
				{
					perror("recv error3\n");
					exit(1);
				}
				// 정보 복사
				strcpy(Desct, Rmsg);

				// SQLITE query문 준비
				rc = sqlite3_prepare_v2(db,query, -1, &res, 0);
				// 제대로 생성됬다면
				if( rc == SQLITE_OK)
				{
					// 쿼리문의 ?변수에 값 연동하기
					sqlite3_bind_int(res,1, day);
					sqlite3_bind_text(res,2,Name, -1, SQLITE_TRANSIENT);
					sqlite3_bind_text(res,3,Desct,-1, SQLITE_TRANSIENT);
					// 쿼리문 동작
					step = sqlite3_step(res);
					strcpy(Smsg,"저장이 완료되었습니다.\n");
                        	        // 쿼리문이 제대로 되었기에 이에 대한 정보를 client에게 전송.
					if(send(nsd, Smsg, sizeof(Smsg),0) == -1)
                                	{
						// 오류발생시 출력후 종료
                                	        perror("send error\n");
                                	        exit(1);
                                	}

				}
				// 만약 문제가 발생했다면
				else
				{
					// 저장에 실패함을 client에게 전달
					strcpy(Smsg, "저장에 실패했습니다\n");
					if(send(nsd, Smsg, sizeof(Smsg),0) == -1)
					{
						perror("send error\n");
						exit(1);
					}
				}
				// 쿼리 비우기.
				sqlite3_finalize(res);
				// 사용한 공간은 에러방지를 위해 초기화
				memset(Rmsg,'\0',sizeof(Rmsg));
                               	memset(Smsg,'\0',sizeof(Smsg));
				printf("cd done\n");
			}
			// 데이터 delete를 호출했다면
			else if(strcmp(command,"dd") == 0)
			{
				int id;
				// 현재 command를 출력한다.
				printf("ok dd\n");
				// client로부터 삭제할 정보의 데이터를 받는다.
				if(recv(nsd, Rmsg, sizeof(Rmsg), 0) == -1)
				{
					// 실패했다면 오류를 출력후 종료
					perror("recve error\n");
					exit(1);
				}
				// 받은 데이터를 변환하기
				id = atoi(Rmsg);
				// SQLITE 쿼리문 만들기
				char* query = "DELETE FROM Lists WHERE ID = ?";
				// 쿼리문 준비하기
				rc = sqlite3_prepare_v2(db,query, -1, &res, 0);
	
				// 정상적으로 만들어졌다면
				if( rc == SQLITE_OK)
				{
					// ?에 값을 연동한다.
					sqlite3_bind_int(res,1,id);
				}
				// 쿼리문을 실행한다.
				step = sqlite3_step(res);
				// 실행이되면 client에게 결과를 전달한다.
				strcpy(Smsg,"삭제가 완료되었습니다.\n");
				// 결과를 전달
				if( send(nsd,Smsg, sizeof(Smsg),0) == -1)
				{
					// 오류가 발생하면 출력후 종료.
					perror("send to fail");
					exit(1);
				}
				// 쿼리 비우기.
				sqlite3_finalize(res);
				// err 메시지 공간 비우기
				sqlite3_free(err_msg);
				// 사용한 공간은 에러방지를 위해 초기화
				memset(Rmsg,'\0',sizeof(Rmsg));
                                memset(Smsg,'\0',sizeof(Smsg));
				printf("dd done\n");

			}	
			// 데이터 모두 출력을 받았을 경우
			else if (strcmp(command,"Pd") == 0)
			{
				// 현재 행하는 동작을 출력
				printf("ok Pd\n");
				int count;
				// SQLITE 쿼리문 생성
				char* query1 = "SELECT COUNT(Id) FROM Lists";
				// 쿼리문 준비
				rc = sqlite3_prepare_v2(db,query1,-1,&res,0);
				// 문제가 없다면 진행 있다면 에러를 출력후 종료
				if(rc != SQLITE_OK)
				{
					perror("query error1\n");
					exit(1);
				}
				// 쿼리문 실행
				step = sqlite3_step(res);
	
				// 현재 몇개의 row가있는지 전송
				if(step == SQLITE_ROW)
				{
					sprintf(Smsg,"%d", sqlite3_column_int(res,0));
					// 쿼리문을 몇번 반복해서 돌릴 지에 대해 저장
					count = atoi(Smsg);                        
				
					// 데이터를 client로 전달
					if(send(nsd,Smsg, sizeof(Smsg), 0) == -1)
					{
						perror("send error");
					}
				}
				// 쿼리문 비우기
				sqlite3_finalize(res);

				// 모든 데이터를 전송하기하기위한 쿼리문 생성
				char* query2 = "SELECT * FROM Lists";
				// 쿼리문 준비하기
				rc = sqlite3_prepare_v2(db,query2,-1,&res,0);
				// 오류가 없다면 진행 있다면 에러를 출력후 종료
				if(rc != SQLITE_OK)
				{
					perror("query error2\n");
					exit(1);
				}
				// SQL에서 전달해줄 ROW가 남아있는 동안
				while(sqlite3_step(res) == SQLITE_ROW)
				{
					// 해당 문을 저장해서 클라이언트로 전달한다.
					sprintf(Smsg,"ID: %d, 마감일: %d, 이름: %s, 세부내용: %s\n",
					sqlite3_column_int(res,0), sqlite3_column_int(res,1), sqlite3_column_text(res,2),
					sqlite3_column_text(res,3));
					// 데이터 전달.
					if(send(nsd, Smsg, sizeof(Smsg),0) == -1)
                               	        	printf("send error\n");
				}

				// 쿼리문 비우기.
				sqlite3_finalize(res);
				// err메시지 공간 비우기
				sqlite3_free(err_msg);
				// 사용한 공간은 에러방지를 위해 초기화
				memset(Rmsg,'\0',sizeof(Rmsg));
                                memset(Smsg,'\0',sizeof(Smsg));
				printf("Pd done\n");
			}
			// Id으로 원하는 데이터 출력을 전달받은 경우
			else if (strcmp(command,"pd") == 0)
			{
				int id;
				// 현재 진행중인 동작을 출력
				printf("ok pd\n");
				// client로부터 데이터를 전달 받는다.
				if(recv(nsd,Rmsg,sizeof(Rmsg),0) == -1)
				{
					printf("rece error\n");
					exit(1);
				}
				id = atoi(Rmsg);
				// 쿼리문을 생성한다.
				char* query = "SELECT * FROM Lists WHERE ID = ?";
				// 쿼리문 준비하기
				rc = sqlite3_prepare_v2(db,query, -1, &res, 0);
				// 쿼리문에 문제가 없다면
				if( rc == SQLITE_OK)
				{
				// ?에 대한 값을 연동한다.
					sqlite3_bind_int(res,1,id);
				}
				// 쿼리문을 실행한다.
				step = sqlite3_step(res);
		
				// 쿼리문에서 받아올 값이 있다면
				if(step ==  SQLITE_ROW)
				{
					// 해당 형식에 맞게 변환하여 client로 전달한다.
					sprintf(Smsg,"ID: %d, 마감일: %d, 이름: %s, 세부내용: %s\n",sqlite3_column_int(res,0), sqlite3_column_int(res,1),sqlite3_column_text(res,2), sqlite3_column_text(res,3));
					// client로 전달
					if(send(nsd, Smsg, sizeof(Smsg),0) == -1)
					{
						perror("send error\n");
						exit(1);
					}
				}
				// 그렇지 않다면 == 없다면
				else
				{
					// 없다는 결과를 전송한다.
					strcpy(Smsg,"아무런 데이터가 없습니다.");
					// 결과를 전송한다.
					if(send(nsd, Smsg, sizeof(Smsg),0) == -1)
					{	
						printf("send erro\n");
						exit(1);
					}
				}
				// 쿼리문 정리하기
				sqlite3_finalize(res);
				// err 메시지 공간 비우기
				sqlite3_free(err_msg);
				// 사용한 공간은 에러방지를 위해 초기화
				memset(Rmsg,'\0',sizeof(Rmsg));
                                memset(Smsg,'\0',sizeof(Smsg));
				printf("pd done\n");
			}
			// 오늘 할일 출력을 전달받았다면
			else if (strcmp(command, "td") == 0)
			{
				int days;
				int count;
                       	 printf("ok td\n");
				// 시간구조체에서 날에 대한 정보 받아오기.
        	                days = tm->tm_mday;
			
				// 1번째 쿼리문 작성하기(몇개의 개수가 있는지 전달을 위한)
				char* query1 = "SELECT COUNT(ID) FROM Lists WHERE EndDay = ?";
                        	// 쿼리문을 준비하기
				rc = sqlite3_prepare_v2(db,query1, -1, &res, 0);
                        	// 문제가 없다면
				if( rc == SQLITE_OK)
                        	{
					// ?에  값을 연동한다
                                	sqlite3_bind_int(res,1,days);
                        	}
				// 쿼리문을 실행한다.
                        	step = sqlite3_step(res);

				// 쿼리문에서 받아올 값이 있다면
                        	if(step ==  SQLITE_ROW)
                        	{
					// 즉 row가 있다면 이를 client에게 전달
					sprintf(Smsg,"%d", sqlite3_column_int(res,0));
					count = atoi(Smsg);
					// client로 전달
					if(send(nsd, Smsg, sizeof(Smsg),0) == -1)
					{
						perror("send error\n");
						exit(1);
					}
                	        }
				// 쿼리 비우기
                        	sqlite3_finalize(res);

				// 2번째 쿼리 작성하기(전체 데이터 가져오기위한)
				char* query2 = "SELECT * FROM Lists WHERE EndDay = ?";
				// 쿼리문 준비하기
				rc = sqlite3_prepare_v2(db,query2,-1,&res,0);
				// 문제가 없다면
				if( rc == SQLITE_OK)
				{
					// ?에 값 연동하기.
					sqlite3_bind_int(res,1,days);
				}
				// SQL에서 전달할 데이터가 있는 동안
				while(sqlite3_step(res) == SQLITE_ROW)
				{
					// 형식에 맞게 작성해서 client에게 전달
					sprintf(Smsg, "ID: %d, 마감일: %d, 이름: %s, 세부내용: %s", sqlite3_column_int(res,0), sqlite3_column_int(res,1), sqlite3_column_text(res,2),sqlite3_column_text(res,3));
					// client로 데이터 전달.
					if(send(nsd, Smsg, sizeof(Smsg), 0) == -1)
					{
						perror("send error\n");
					}
				}
				// 쿼리 비우기.
				sqlite3_finalize(res);
				// 에러 메시지를 위한 공간 비우기
				sqlite3_free(err_msg);
				// 오류방지를 위해 모든 공간 초기화
				memset(Rmsg,'\0',sizeof(Rmsg));
                                memset(Smsg,'\0',sizeof(Smsg));
				printf("td done\n");

			}
			else if( strcmp(command, "ud") == 0)
			{
				int id;
				// 서버에 어떠한 것을 진행하는 지 출력
				printf("Ok ud\n");
				// id를 전달받는다.
				if (recv(nsd, Rmsg, sizeof(Rmsg),0) == -1)
				{
					perror("recv error\n");
					exit(1);
				}
				id = atoi(Rmsg);
				// 쿼리문을 생성한다. 해당 id가 데이터베이스에 있는지
				char* query = "SELECT * FROM Lists WHERE ID = ?";
				// 쿼리문 준비하기
				rc = sqlite3_prepare_v2(db,query, -1, &res, 0);
				// 쿼리문에 문제가 없다면
				if( rc == SQLITE_OK)
				{
					// ?에 대한 값을 연동한다.
					sqlite3_bind_int(res,1,id);
				}
				// 쿼리문을 실행한다.
				step = sqlite3_step(res);

				// 쿼리문에서 받아올 값이 있다면
				if(step ==  SQLITE_ROW)
				{
					// 해당 형식에 맞게 변환하여 client로 전달한다.
					strcpy(Smsg,"OK");
					// client로 전달
					if(send(nsd, Smsg, sizeof(Smsg),0) == -1)
					{
						perror("send error\n");
						exit(1);
					}
				}
				// 그렇지 않다면 == 없다면
				else
				{
					// 없다는 결과를 전송한다.
					strcpy(Smsg,"Fail");
					// 결과를 전송한다.
					if(send(nsd, Smsg, sizeof(Smsg),0) == -1)
					{
						printf("send erro\n");
						exit(1);
					}
				}
				// 쿼리문을 정리한다.
				sqlite3_finalize(res);
				printf("Check done\n");
				// 전송결과에 대한 클라이언트에 대답을 받는다.
				if(recv(nsd,Rmsg,sizeof(Rmsg),0) == -1)
				{
					perror("recv error\n");
					exit(1);
				}
	
				// 만약 변경작업을 한다면
				if(strcmp(Rmsg, "FIX") == 0)
				{
					// 변경값에 대해 값을 받아온다.
					int day;
					char Name[MAX];
					char Desct[MAXLEN];
					char* query = "UPDATE Lists set EndDay = ? WHERE ID = ?";
					if(recv(nsd, Rmsg, sizeof(Rmsg), 0) == -1)
					{
						// 오류가 발생하면 출력후 종효
						perror("recv error1\n");
						exit(1);
					}
					// day를 integer로 변경
					day = atoi(Rmsg);
					rc = sqlite3_prepare_v2(db,query, -1, &res, 0);

					// 정상적으로 만들어졌다면
					if( rc == SQLITE_OK)
					{
						// ?에 값을 연동한다.
						sqlite3_bind_int(res,1,day);
						sqlite3_bind_int(res,2,id);
					}
					// 쿼리문을 실행한다.
					step = sqlite3_step(res);
					strcpy(Smsg,"Update Done");
					if(send(nsd,Smsg,sizeof(Smsg),0) == -1)
					{
						perror("send error");
						exit(1);
					}
					// 에러 방지를 위해 사용한 공간 초기화
					memset(Rmsg,'\0',sizeof(Rmsg));
					memset(Smsg,'\0',sizeof(Smsg));
					// 쿼리문 정리
					sqlite3_finalize(res);
					// 에러메시지 정리
					sqlite3_free(err_msg);
					printf("ud Done\n");
				}
				else
				{
					printf("Do Noting\n");
				}
			}
		}
	}
	// SQL을 닫고 모든 소켓을 닫고 종료한다.
	sqlite3_close(db);
	
	close(nsd);
    	close(sd);

	return 0;
}
