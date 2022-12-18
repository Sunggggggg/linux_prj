#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h> // need -lrt option. means to use the real-time library

//#define DEBUG_R
#define DEBUG
#define DEBUG_BTN
#define DEBUG_TIME

#define DEATH_MATCH     1  // do until someone is defeated
#define SERVIVAL        0  // do just three rounds
#define USER        0  // score index
#define RASPI       1
#define LED_OFF         2
#define LED_WIN         1
#define LED_LOSE        0
#define SEC2nSEC    1000000000
#define SEC2uSEC    1000000

////////////////////// clock timer //////////////////////

typedef struct timespec Pitime;
Pitime gettime_now;
Pitime NOW() {
    clock_gettime(CLOCK_REALTIME, &gettime_now);
    return gettime_now;
}

int timePassed_us(Pitime* ref) {
    int time_passed_nsec;
    int time_passed_sec;
    int underflow;
    
    // 1s = 1,000,000us, 1us = 1,000ns
    clock_gettime(CLOCK_REALTIME, &gettime_now);
    time_passed_nsec  = gettime_now.tv_nsec - ref->tv_nsec;
    underflow = time_passed_nsec < 0 ? 1 : 0;
    if (underflow) time_passed_nsec += 1 * SEC2nSEC;
    time_passed_sec   = gettime_now.tv_sec  - ref->tv_sec - underflow;
#ifdef DEBUG_TIME
    if (time_passed_sec < 0) printf("timePassed_us : !\n");
#endif
    return time_passed_sec * SEC2uSEC + time_passed_nsec / 1000;
}

int myRand() {
    Pitime rand_seed;
    rand_seed = NOW();
#ifdef DEBUG_R
    printf("main : rand val : %d\n", (rand_seed.tv_nsec & 1));
#endif
    return rand_seed.tv_nsec & 1;
}


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

int openAllDev();
void closeAllDev();


////////////////////// GPIO //////////////////////

int toggle_button_state = 0;

// update toggle button signal w/ signal debouncing
void buttonUpdate() {
    const  int      debounce_time_us = 40;
           char     buff;
    static char     last_button_state = '0';
    static char     curr_button_state = '0';
    static Pitime   last_pushed = {0};

    read(dev_gpio, &buff, 1); // read pin 6

    if (buff != last_button_state) // if the button signal detected(pressed or noise),
        last_pushed = NOW();         
    if (timePassed_us(&last_pushed) > debounce_time_us) // count the time a little
        if (buff != curr_button_state) { // if the button signal is still changed
            curr_button_state = buff;
            if (curr_button_state == '1') {
#ifdef DEBUG_TIME
                printf("buttonUpdate : pressed\n");
#endif
                toggle_button_state = !toggle_button_state;
            }
        }
    last_button_state = buff; // last_button_state will follow the signal(pressed or noise).
}

void writeLED(const int winflag) {
    char buff;
    if      (winflag == LED_WIN)
        buff = '1'; // blue LED
    else if (winflag == LED_OFF)
        buff = '2'; // turn all LED off
    else // (winflag == LED_LOSE)
        buff = '0'; // red LED
    write(dev_gpio, &buff, 1);
}


////////////////////// Buzzer //////////////////////

void playBuzzer(char song) {
    static char prev_song = 'i';
    if (prev_song != song) {
        write(dev_bzzr, &song, 1);
        prev_song = song;
    }
}


////////////////////// 7-Segment //////////////////////

#define D1 0x01
#define D2 0x02
#define D3 0x04
#define D4 0x08

const char seg_num[10] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xd8, 0x80, 0x90};
const char seg_dot = 0x7f;
int FND(int* score) {
    unsigned short data[3];
    static int n = 0;

    data[0] = (seg_num[score[0]] << 4) | D1;
    data[2] = (seg_dot           << 4) | D3;
    data[1] = (seg_num[score[1]] << 4) | D4;

    write(dev_fnd, &data[n], 2);
    n++;
    n = (n + 1) % 2;
}


////////////////////// Servo Motor //////////////////////



////////////////////// main //////////////////////

int main(void) {
    Pitime time_ref;

    // open character devices
    int opn_err = openAllDev();
    if (opn_err & ERR_OPN_GPIO) goto CDevOpenFatal;
    

    // wait for the start button pressed (behave as toggle)
    do {buttonUpdate();} 
    while (!toggle_button_state);

    // game started. wait 2sec...
    int game_mode = SERVIVAL;
    time_ref = NOW();
    while (timePassed_us(&time_ref) < (2 * SEC2uSEC)) buttonUpdate();
    if (toggle_button_state == 0) {
        toggle_button_state = 1;
        game_mode = DEATH_MATCH;
    }
#ifdef DEBUG
    printf("main : game starts. curr game mode : %s\n", game_mode == SERVIVAL ? "SERVIVAL" : "DEATH_MATCH");
#endif

    // initialize variables for loop
    int passed_time_from_ref;
    int score[2] = { 3, 3 };
    int stage_count = (game_mode == SERVIVAL) ? 3 : 999999999;
    int stage_result = 1;
    int rpi_dir, usr_dir, usr_dir0, usr_dir1;
    time_ref = NOW();
    //TODO: init motor to 0.
#ifdef DEBUG
    int current = 0;
#endif

    while (toggle_button_state) {
        // Face Detecting...

        FND(score);
        buttonUpdate();
        passed_time_from_ref = timePassed_us(&time_ref);
            /////////////////////////////////////////
        if (passed_time_from_ref < (0.7 * SEC2uSEC)){ 


#ifdef DEBUG
            if (current != 1) {printf("Stage 1\n"); current = 1;}
#endif
            playBuzzer('a'); //cham cham cham! (only once)
            rpi_dir = myRand(); //is current system clock count odd? or even?
            //TODO: motor set to 0 (only once)
            //FIXME: get user face direction0.


            //////////////    ~0.7s    //////////////
        } else if (passed_time_from_ref <  (1.4 * SEC2uSEC)) { 


#ifdef DEBUG
            if (current != 2) {printf("Stage 2 : rpi_dir = %d, usr_dir0 = \n", rpi_dir); current = 2;}
#endif
            writeLED(LED_OFF);
            //TODO: motor set to dir_rpi (only once)
            //FIXME: get user face direction1.


            //////////////    ~1.4s    //////////////
        } else if (passed_time_from_ref < (4.1 * SEC2uSEC)) {
            

#ifdef DEBUG
            if (current != 3) {printf("Stage 3 : usr_dir1 = \n"); current = 3;}
#endif                
            if (stage_result == 1) playBuzzer('b');  // win (user side)
            else playBuzzer('c'); //stage_result == 0   lose
            //FIXME: compute user's decision and update the result.

            //////////////    ~4.1s    //////////////
        } else {
            ////////////// after 4.1s  //////////////

#ifdef DEBUG
            if (current != 4) {printf("Stage 4\n"); current = 4;}
#endif  
            if (stage_result == 1) {  // win (user side)
                score[RASPI]--;
                writeLED(LED_WIN);
            }
            else {                    // lose
                score[USER]--;
                writeLED(LED_LOSE);
            }

            if (--stage_count && (score[RASPI] && score[USER])) {
                // if the game isn't over
                time_ref = NOW();
            } else { // if the game is over.
                if (score[USER] >= score[RASPI])
                    playBuzzer('d'); // user won  this game
                else
                    playBuzzer('e'); // user lost this game

                break; // finish game.
            }
        }
    }
    time_ref = NOW();
    while (timePassed_us(&time_ref) < (2 * SEC2uSEC));

CDevOpenFatal:
    closeAllDev();

    return 0;
}








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