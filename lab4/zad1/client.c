#include <mqueue.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MSG_SIZE 101 // max str len 100 + 1 for 8-bit user id, 0 for init
#define TOTAL_CLIENTS_PLUS_ONE 256
#define MAX_MSG 10
#define CLIENT_MQ_NAME "/simplechatclient_%d"
#define SERVER_MQ_NAME "/simplechatserver_q"

int main(int argc, char* argv[]) {

    mq_unlink(CLIENT_MQ_NAME);
    struct mq_attr attr = {
        .mq_maxmsg = MAX_MSG,
        .mq_msgsize = MSG_SIZE
    };

    pid_t pid = getpid();
    char queue_name[21+10] = {0};
    sprintf(queue_name, CLIENT_MQ_NAME, pid);
    mqd_t client_q = mq_open(queue_name, O_CREAT | O_RDONLY, 0600, &attr); 
    if (client_q == -1) {
        perror("Nie udalo sie otworzyc kolejki klienta ");
        exit(1);
    }
    mqd_t server_q = mq_open(SERVER_MQ_NAME, O_WRONLY);
    if (server_q == -1) {
        perror("Nie udalo sie otworzyc kolejki serwera ");
        mq_close(client_q);
        exit(1);
    }
    char msg_buf[MSG_SIZE+1] = {0};
    memcpy(msg_buf+1, (char*)&pid, sizeof(pid_t)); //le assumption
    if (mq_send(server_q, msg_buf, MSG_SIZE, 0) == -1) {
        perror("Nie udalo sie wyslac init wiadomosci ");
        mq_close(client_q);
        mq_close(server_q);
        exit(1);
    }
    if (mq_receive(client_q, msg_buf, MSG_SIZE+1, NULL) == -1) {
        perror("Nie udalo sie odczytac wiadomosci init serwera ");
        mq_close(client_q);
        mq_close(server_q);
        exit(1);
    }
    int client_id = msg_buf[0];
    
    if (fork() == 0) {
        while (1) {
            if (mq_receive(client_q, msg_buf, MSG_SIZE+1, NULL) == -1) {
                perror("Nie udalo sie otrzymac wiadomosci");
            }
            printf("ID: %d >>> %s", (int)msg_buf[0],msg_buf+1);
        }
    } else {
        while (1) {
            msg_buf[0] = client_id; //this line should be removeable
            if (fgets(msg_buf+1, MSG_SIZE, stdin) == NULL) {
                fprintf(stderr, "Wpisany tekst jest nieprawidlowy");
                continue;
            }
            if (mq_send(server_q, msg_buf, MSG_SIZE, 0) == -1) {
                perror("Nie udalo sie wyslac wiadomosci do serwera");
            }
        }
    }
}