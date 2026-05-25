#include <mqueue.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG_SIZE 101 // max str len 100 + 1 for 8-bit user id, 0 for init
#define TOTAL_CLIENTS_PLUS_ONE 256
#define MAX_MSG 10
#define MQ_NAME "/simplechatserver_q"

int assign_client(mqd_t client_q, mqd_t clients_queues[TOTAL_CLIENTS_PLUS_ONE], char active_clients[TOTAL_CLIENTS_PLUS_ONE]) {
    for (int i = 1; i < TOTAL_CLIENTS_PLUS_ONE; i++) {
        if (active_clients[i] == 0) { // found free id
            clients_queues[i] = client_q;
            active_clients[i] = 1;
            return i;
        }
    }
    return -1;
}

void process_msg(char msg_buf[MSG_SIZE], mqd_t clients_queues[TOTAL_CLIENTS_PLUS_ONE], char active_clients[TOTAL_CLIENTS_PLUS_ONE]) {
    if (msg_buf[0] == 0) { //init
        pid_t client_pid;
        memcpy((char*)&client_pid, msg_buf+1, sizeof(pid_t)); // le assumption
        char client_queue_name[21+10]; // "/simplechatclient_%d" length + ceil(log10(sizeof(pid_t)))
        sprintf(client_queue_name, "/simplechatclient_%d", client_pid);
        mqd_t client_queue = mq_open(client_queue_name, O_WRONLY);
        if (client_queue == -1) {
            perror("Nie udalo sie otworzyc kolejki klienta: ");
        } else {
            int client_id = assign_client(client_queue, clients_queues, active_clients);
            if (client_id == -1) {
                fprintf(stderr, "Osiagnieto max klientow");
                mq_close(client_queue);
                return;
            }
            char msg_buf[MSG_SIZE+1] = {0};
            msg_buf[0] = (char)client_id;
            if (mq_send(client_queue, msg_buf ,MSG_SIZE, 0) == -1) {
                perror("Blad w wyslaniu id do klienta: ");
                mq_close(client_queue);
                active_clients[client_id] = 0;
                return;
            }
            printf("Klient o pid: %d, id: %d podlaczyl sie.", client_pid, client_id);
        }
    } else {
        for (int i = 1; i < TOTAL_CLIENTS_PLUS_ONE; i++) {
            if (active_clients[i]) {
                if (mq_send(clients_queues[i], msg_buf, MSG_SIZE, 0) == -1) {
                    perror("Nie udalo sie wyslac wiadomosci do jednego z klientow");
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    mq_unlink(MQ_NAME);
    struct mq_attr attr = {
        .mq_maxmsg = MAX_MSG,
        .mq_msgsize = MSG_SIZE
    };

    mqd_t queue = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0600, &attr);
    if (queue == -1) {
        perror("Nie udalo sie otworzyc kolejki serwera");
        exit(1);
    }

    char msg_buf[MSG_SIZE+1] = {0};
    mqd_t clients_queues[TOTAL_CLIENTS_PLUS_ONE]; //255 clients supported, first element is empty
    char active_clients[TOTAL_CLIENTS_PLUS_ONE] = {0}; // 1 if id is active, 0 otherwise
    while (1) {
        if (mq_receive(queue, msg_buf, MSG_SIZE+1, NULL) == -1) {
            perror("Blad przy czytaniu kolejki : ");
        } else {
            process_msg(msg_buf, clients_queues, active_clients);
        }
    }
}

