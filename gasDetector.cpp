//
// Created by 马浩程 on 2018/12/6.
//

#include <iostream>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/termios.h>
#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>
#include <time.h>

//const char* logo = " _    _ _ _                             _         _____             __  __      _            \n| |  | | | |                           (_)       / ____|           |  \\/  |    | |           \n| |  | | | |_ _ __ __ _ ___  ___  _ __  _  ___  | |  __  __ _ ___  | \\  / | ___| |_ ___ _ __ \n| |  | | | __| '__/ _` / __|/ _ \\| '_ \\| |/ __| | | |_ |/ _` / __| | |\\/| |/ _ \\ __/ _ \\ '__|\n| |__| | | |_| | | (_| \\__ \\ (_) | | | | | (__  | |__| | (_| \\__ \\ | |  | |  __/ ||  __/ |   \n \\____/|_|\\__|_|  \\__,_|___/\\___/|_| |_|_|\\___|  \\_____|\\__,_|___/ |_|  |_|\\___|\\__\\___|_|   \n ";


const float scale = 24.67;// 1.48 -- 60mL
#pragma pack (1)
struct {
    uint8_t addr;
    uint8_t func;
    uint8_t len;

    uint8_t addrH;
    uint8_t addrL;
    uint8_t ppm1;
    uint8_t ppm2;
    uint8_t ppm3;
    uint8_t ppm4;

    uint8_t statusH;
    uint8_t statusL;
    uint8_t lightH;
    uint8_t lightL;

    uint8_t CRCL;
    uint8_t CRCH;

}results = {0};

using namespace std;


int main (int argc,char* argv[]) {
    const char *sp_path = (argc > 1) ? argv[1] : "/dev/tty.wchusbserial1420";

    int fd = open(sp_path, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        printf("open port failed");
        return fd;
    }

    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if ( tcgetattr ( fd, &tty ) != 0 )
    {
        cout << "Error " << errno << " from tcgetattr: " << strerror(errno) << endl;
    }
    cfsetospeed (&tty, B9600);
    cfsetispeed (&tty, B9600);
    tty.c_cflag     &=  ~PARENB;        // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;
    tty.c_cflag     &=  ~CRTSCTS;       // no flow control
    tty.c_lflag     =   0;          // no signaling chars, no echo, no canonical processing
    tty.c_oflag     =   0;                  // no remapping, no delays
    tty.c_cc[VMIN]      =   0;                  // read doesn't block
    tty.c_cc[VTIME]     =   5;                  // 0.5 seconds read timeout

    tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines
    tty.c_iflag     &=  ~(IXON | IXOFF | IXANY);// turn off s/w flow ctrl
    tty.c_lflag     &=  ~(ICANON | ECHO | ECHOE | ISIG); // make raw
    tty.c_oflag     &=  ~OPOST;              // make raw

    /* Flush Port, then applies attributes */
    tcflush( fd, TCIFLUSH );
    if ( tcsetattr ( fd, TCSANOW, &tty ) != 0)
    {
        cout << "Error " << errno << " from tcsetattr" << endl;
    }



    for (int i = 0;; i++) {

        int readCount = 0;
        while (1) {
            unsigned char str[]={0x01,0x03,0x00,0x00,0x00,0x05,0x85,0xC9};
            write(fd, str,sizeof(str));
            usleep(150000);

            long r = read(fd, ((unsigned char *) &results) , sizeof(results) );
            if (r != 15){
                read(fd, ((unsigned char *) &results) , sizeof(results) );
                printf("read count error.\n");
                continue;
            }

            if( (results.addr!=1) || (results.len!=10)  ){
                read(fd, ((unsigned char *) &results) , sizeof(results) );
                printf("read content error.\n");
                continue;
            }



            printf("浓度:%6d ppm*m, 光强:%5d, 状态:%d, 数据：",
                    (((uint32_t)results.ppm1)<<24)+(((uint32_t)results.ppm2)<<16)+(((uint32_t)results.ppm3)<<8)+results.ppm4,
                    ((uint32_t)results.lightH<<8)+results.lightL,((uint32_t)results.statusH<<8)+results.statusL );
            for(int j=0;j<sizeof(results);j++){
                printf("%02X ",((unsigned char *)&results)[j]);
            }

            time_t timer;
            char timeStr[50];
            struct tm* tm_info;
            time(&timer);
            tm_info = localtime(&timer);
            strftime(timeStr, 26, "%H:%M:%S", tm_info);
            puts(timeStr);

        }

        fflush(stdout);
    }
}



//            system("clear");
//            printf("\033[0m[Heat Meter]\n");
//            printf("\033[1;35;40mID:0x%x 累计热量:%7.4f kWh   累计暖水量:%7.4f m^3   进水温度:%3.1f°C   回水温度:%3.1f°C\n",
//                       addr1, data1.accu_Heat, data1.accu_Flow,  data1.T_Display, data1.T_return_Display);

//
//            printf("\033[0m[Water Meter]\n");
//            printf("\033[1;32;40mID:0x%x 累计流量:%7.4f m^3   瞬时流量:%7.4f m^3/h\n",addr3,data3.accu_Flow,data3.inst_Flow);

//
////    转义序列以ESC(\033)开头.  \033[显示方式;前景色;背景色m
////    显示方式:0（默认值）、1（高亮）、22（非粗体）、4（下划线）、24（非下划线）、5（闪烁）、25（非闪烁）、7（反显）、27（非反显）
////    前景色:30（黑色）、31（红色）、32（绿色）、 33（黄色）、34（蓝色）、35（洋红）、36（青色）、37（白色）
////    背景色:40（黑色）、41（红色）、42（绿色）、 43（黄色）、44（蓝色）、45（洋红）、46（青色）、47（白色）
//
//}



