#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "monitor.h"

#define DEST_ADDR "10.10.10.1" // osaka
#define DEST_PORT 22222
#define RECV_ADDR "10.10.10.2" // sendai
#define RECV_PORT 22222
#define INTERVAL 0.1 // modified. The unit is second. before 0.01 2024/05/09 before 0.001:2024/06/22
#define NUMMONITORING 1200 // modified. before 12000:2024/05/09 before 12000:2024/06/22

int main(int argc, char **argv) {
    struct sockaddr_in send_addr, recv_addr;
    int send_sd, recv_sd;
    if (argc == 1) {
        printf("usage: ./manager result_filename");
        return 0;
    }

    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    inet_aton(DEST_ADDR, &send_addr.sin_addr);
    send_addr.sin_port = htons(DEST_PORT);

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    inet_aton(RECV_ADDR, &recv_addr.sin_addr);
    recv_addr.sin_port = htons(RECV_PORT);

    if ((send_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket send");
        exit(1);
    }

    if ((recv_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket recv");
        exit(1);
    }

    if (bind(recv_sd, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (connect(send_sd, (struct sockaddr *)&send_addr, sizeof(send_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    if (argc != 2) puts("Enter result file name");
    FILE *fp = fopen(argv[1], "w");
    long init_data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    long metrics[10];
    long old_metrics[10];

    // store first data into old_metrics[]
    if (send(send_sd, init_data, sizeof(init_data), 0) < 0) {
        perror("send");    
        exit(1);
    }
    if ((recv(recv_sd, old_metrics, sizeof(old_metrics), 0) < 0)) {
        perror("recv");
        exit(1);
    }

    // declare variables relavant to time
    struct timespec send_time, recv_time, interval = {.tv_sec = 0, .tv_nsec = INTERVAL * 1000000000};
    struct tm *time;

    // start monitoring for NUMMONITORING times
    for (int i = 0; i < NUMMONITORING; i++) {
        nanosleep(&interval, NULL);
        if (send(send_sd, init_data, sizeof(init_data), 0) < 0) {
            perror("send");    
            exit(1);
        }

        timespec_get(&send_time, TIME_UTC);

        if (recv(recv_sd, metrics, sizeof(metrics), 0) < 0) {
            perror("recv");
            exit(1);
        }

        timespec_get(&recv_time, TIME_UTC);

        time = localtime(&recv_time.tv_sec);
        /*the unit of elapsed time is micro second*/
        long elapsed_time = (recv_time.tv_sec - send_time.tv_sec) * 1000000L + (recv_time.tv_nsec - send_time.tv_nsec) / 1000L;
        fprintf(fp, "%d/%02d/%02d-%02d:%02d:%02d.%ld,", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec, recv_time.tv_nsec / 1000);
        fprintf(fp, "%ld.%09ld,%ld,", send_time.tv_sec, send_time.tv_nsec, elapsed_time);
        for (int j = 0; j < 9; j++) {
            fprintf(fp, "%lu,", metrics[j] - old_metrics[j]);
        }
        fprintf(fp, "%lu\n", metrics[9] - old_metrics[9]);
        memcpy(old_metrics, metrics, sizeof(long) * 10);
        if (i % 100 == 0){
            printf("message[%d] is sent\n", i);
        }
    }

    close(send_sd);
    close(recv_sd);
    fclose(fp);

    return 0;
}
