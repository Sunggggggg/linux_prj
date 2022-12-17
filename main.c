#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h> // need -lrt option. means to use the real-time library

#define DEBUG

typedef struct timespec Pitime;
Pitime gettime_now;
Pitime NOW() {
    clock_gettime(CLOCK_REALTIME, &gettime_now);
    return gettime_now;
}
int isTimePassed_us(Pitime ref, int time_us) {
    // 1s = 1,000,000us, 1us = 1,000ns
    int time_sec  = time_us / 1000000;
    int time_nsec = (time_us % 1000000) * 1000;
    int passed_sec = ref.tv_sec + time_sec;
    int passed_nsec = ref.tv_nsec + time_nsec;
    clock_gettime(CLOCK_REALTIME, &gettime_now);
    return (gettime_now.tv_sec < passed_sec) ? 0 : (gettime_now.tv_nsec < passed_nsec) ? 0 : 1;
}


#define DRAW    2
#define WIN     1
#define LOSE    0

#define D1 0x01
#define D4 0x08


char seg_num[10] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xd8, 0x80, 0x90};
char seg_dnum[10] = {0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x58, 0x00, 0x10};

const char* cdev_dirs[4] = {
    "/dev/my_motor_driver",
    "/dev/my_buzzer_driver",
    "/dev/my_gpio_driver",
    "/dev/my_fnd_driver"
};
int dev_svmt;
int dev_bzzr;
int dev_gpio;
int dev_fnd;
#define ERR_OPN_SVMT    (1 << 0)
#define ERR_OPN_BZZR    (1 << 1)
#define ERR_OPN_GPIO    (1 << 2)
#define ERR_OPN_FND     (1 << 3)

int openAllDev() {
    int* cdevs[4] = {
        &dev_svmt,
        &dev_bzzr,
        &dev_gpio,
        &dev_fnd
    };
    int err = 0;

    // note : access order - [i] is faster than *
    for (int i = 0; i < 4; i++) {
        *cdevs[i] = open(cdev_dirs[i], O_RDWR);
        if (*cdevs[i] < 0) {
            printf("main : Opening %s is not Possible!\n", cdev_dirs[i]);
            err += (1 << i);
        }
    }

    return err;
}

void closeAllDev() {
    int* cdevs[4] = {
        &dev_svmt,
        &dev_bzzr,
        &dev_gpio,
        &dev_fnd
    };
    for (int i = 0; i < 4; i++) 
        if (*cdevs[i] > 0)
            close(*cdevs[i]);
}




int toggle_button_state = 0;

void buttonUpdate() {
    char            buff;
    static char     last_button_state = '0';
    static char     curr_button_state = '0';
    static Pitime   last_pushed = 0;

    read(dev_gpio, &buff, 1); // read pin 6

    if (buff != last_button_state) // if the button signal detected(pressed or noise),
        last_pushed = NOW();         
    else if ((NOW() - last_pushed) > 20000L) // count the time a little
        if (buff != curr_button_state) { // if the button signal is still changed
            curr_button_state = buff;
            if (curr_button_state == '1')
                toggle_button_state = !toggle_button_state;
        }
    last_button_state = buff; // last_button_state will follow the signal(pressed or noise).
}


void playBuzzer(char song) {
    write(dev_bzzr, &song, 1);
}


int FND(int dev, int* score) {
    unsigned short data[2];
    int n = 0;

    data[0] = (seg_num[score[0]] << 4) | D1;    // pin2
    data[1] = (seg_num[score[1]] << 4) | D4;    // pin5

    write(dev, &data[n], 2);
    n++;
    n = (n + 1) % 2;
}




int main(void) {

    int opn_err = openAllDev();
    if (opn_err & ERR_OPN_GPIO) goto CDevOpenFatal;
    

    // wait for the start button pressed (behave as toggle)
    do {buttonUpdate();} 
    while (!toggle_button_state);

    int score[2] = { 0, 0 };
    Motor(0);

    // game started. wait a sec...
    Pitime time_ref = NOW();
    while (isTimePassed_us(time_ref, 2000000));
    
    time_ref = NOW();
    while (toggle_button_state){
        // Face Detecting...

        // FND ON
        FND(dev_fnd, score);
                
        /*��ư���� ������Ʈ*/
        toggle_button_state = buttonUpdate();

        if(/*���غ��� 0.7�� ��������...*/){
            /* ������ ���� �︮��*/
            chamchamcham(); //��� �ѹ��� ������
                        
            Motor(0); //��� �ѹ��� �� �ʿ� ���µ� ����� �ʿ�� ������ ����

            user_dir0 = /*�̿��� �󱼰��� �о����*/;


            //while�� �ȿ����� �ǵ��� printf�� ����
            //printf("Compter : %d \t User : %d \t Result : %d", com_dir, user_dir, result);         
            //else printf("Can't Detect face...\n");

        } else if (/*�� ����, ���غ��� 1.4�� ��������...*/) {

            com_dir = /*��ǻ�� �¿� ���� ����*/;
            Motor(com_dir);

            user_dir1 = /*�̿��� �󱼰��� �о����*/;

        } else if (/*�� ����, ���غ��� 3.1�� ��������...*/) {
            
            user_dir = user_dir0 - user_dir1;
            result_stage = Compare(com_dir, user_dir);
                
            // ���������� �ѹ���
            stageBeep(result_stage); //�������� ��� ���(��/��)

        } else { //�� ���������� ������... ������������? �ƴ� ��?
                
            //�� else���� ���������� ������ �ѹ��� �����

            if (--stage_count) {//������ �ȳ�������
                time_ref = NOW(); //���ؽð� �ٽ� ������Ʈ
                /*�������� ����� ���ھ �ݿ��ϱ�*/ 

            } else {   //������ ��������

                switch (result) {
                case WIN:
                    /* �Ķ��� LED*/ // usleep(delay_time);?
                    /* �¸� ����*/
                    score[0]++;
                    break;
                case LOSE:
                    /* ������ LED*/
                    /* �й� ����*/
                    score[1]++;
                    break;
                case DRAW:
                    /* */
                    printf("Draw Game\n");
                default:
                    break;
                }

            }
        }
        
        /* �ʱ�ȭ ���·� ����� -�缮: �ٵ� �̷����ϸ� ���Ͱ� �׻� 0���� ��*/ 
        //Motor(0);
        //LED(0,0);
    }

CDevOpenFatal:
    closeAllDev();

    return 0;
}