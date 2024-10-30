#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "monitor.h"

#define BUF_SIZE 256
#define DEST_ADDR "192.168.23.172"
#define DEST_PORT 19999
#define RECV_ADDR "192.168.23.98"
#define RECV_PORT 22224
#define RES_LEN 60000

char res[RES_LEN];

const char *request =
    "GET /api/v1/allmetrics?format=shell&filter=system.* HTTP/1.1\r\n"
    "Host: " DEST_ADDR ":19999\r\n"
    "\r\n";

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("usage: ./manager result_filename\n");
        return 0;
    }
    struct sockaddr_in send_addr;
    int sd;
    char recv_buf[BUF_SIZE];

    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    inet_aton(DEST_ADDR, &send_addr.sin_addr);
    send_addr.sin_port = htons(DEST_PORT);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    if (connect(sd, (struct sockaddr *)&send_addr, sizeof(send_addr)) < 0) {
        perror("connect");
        exit(1);
    }
    struct timespec send_time, recv_time, interval = {.tv_sec = 0, .tv_nsec = 5000000};
    struct tm *time;
    FILE *fp = fopen(argv[1], "w");
    if (send(sd, request, strlen(request), 0) < 0) {
        perror("send");
        exit(1);
    }

    if (recv(sd, res, RES_LEN, 0) < 0) {
        perror("recv");
        exit(1);
    }
    printf("%s", res);

    // close(sd);
    for (int i = 0; i < 12000; ++i) {
        nanosleep(&interval, NULL);
        printf("i: %d\n", i);
        if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket");
            exit(1);
        }

        if (connect(sd, (struct sockaddr *)&send_addr, sizeof(send_addr)) < 0) {
            perror("connect");
            exit(1);
        }
        if (send(sd, request, strlen(request), 0) < 0) {
            perror("send");
            exit(1);
        }
        timespec_get(&send_time, TIME_UTC);

        if (recv(sd, res, RES_LEN, 0) < 0) {
            perror("recv");
            exit(1);
        }
        timespec_get(&recv_time, TIME_UTC);
        time = localtime(&recv_time.tv_sec);
        // printf("%ld.%ld,%09ld: ", send_time.tv_sec, send_time.tv_nsec, (recv_time.tv_sec - send_time.tv_sec) * 1000000000 + recv_time.tv_nsec - send_time.tv_nsec);
        // for (int i = 0; i < NSTATS; ++i) {
        //     printf("%ld ", metric[i]);
        // }
        // printf("\n");
        fprintf(fp, "%d/%02d/%02d-%02d:%02d:%02d.%ld,%ld.%09ld,%ld\n", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec, recv_time.tv_nsec / 1000, send_time.tv_sec, send_time.tv_nsec,
                (recv_time.tv_sec - send_time.tv_sec) * 1000000000L + recv_time.tv_nsec - send_time.tv_nsec);
        // print_metrics(fp, res);
        printf("%s", res);
        close(sd);
    }

    close(sd);
    fclose(fp);

    return 0;
}
