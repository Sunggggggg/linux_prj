#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/jiffies.h>

#define DEBUG

typedef unsigned long Pitime
#define NOW     jiffies

#define DRAW    2
#define WIN     1
#define LOSE    0

#define D1 0x01
#define D4 0x08


char seg_num[10] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xd8, 0x80, 0x90};
char seg_dnum[10] = {0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x58, 0x00, 0x10};

int dev_svmt;
int dev_bzzr;
int dev_gpio;
int dev_fnd;

int OpenCharDev(const char* dir) {
    int tmp = open(dir, O_RDWR);
    if (tmp < 0) {
        printf("main : Opening %s is not Possible!\n", dir);
        goto FATAL;
    }
    return tmp;
}


int toggle_button_state = 0;

void buttonUpdate() {
    char            buff;
    static char     last_button_state = '0';
    static char     curr_button_state = '0';
    static Pitime   last_pushed = 0;

    read(dev_gpio, &buff, 1); // read pin 6

    if (buff != last_button_state) // if the button signal detected(pressed or noise),
        last_pushed = NOW;         
    else if ((NOW - last_pushed) > msecs_to_jiffies(20)) // count the time a little
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

    //dev_svmt = OpenCharDev("/dev/my_motor_driver");
    dev_bzzr = OpenCharDev("/dev/my_buzzer_driver");
    //dev_gpio = OpenCharDev("/dev/my_gpio_driver");
    //dev_fnd  = OpenCharDev("/dev/my_fnd_driver");
    printf("main : Opening char devs success...!!!\n");


    // wait for the start button pressed (behave as toggle)
    do {buttonUpdate()} 
    while (!toggle_button_state);

    int score[2] = { 0, 0 };
    Motor(0);

    // game started. wait a sec...
    Pitime time_ref = NOW;
    while ((time_ref + msecs_to_jiffies(2000)) > NOW);
    
    time_ref = NOW;
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
                time_ref = NOW; //���ؽð� �ٽ� ������Ʈ
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

    close(dev);

    return 0;
FATAL :
    return -1;
}