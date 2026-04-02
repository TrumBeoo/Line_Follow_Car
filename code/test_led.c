/*
#include <18F2685.h>
#fuses HS, NOWDT, NOPROTECT, NOLVP
#use delay(clock=20000000)

#define LED1 PIN_B2
#define LED2 PIN_B3



void main(void) {
    setup_adc(ADC_OFF);
    
    output_high(LED1);
    output_high(LED2);
    
    while(TRUE) {
        
    }
}*/

#include <18F2685.h>
#fuses HS, NOWDT, NOPROTECT, NOLVP, PUT
#use delay(crystal=20000000)

// --- Ð?nh nghia Pin Map [86, 114, History] ---
#define LED1        PIN_B2   // LED Status - B?t khi ch?y, t?t khi END
#define LED2        PIN_B3   // LED Action - Nháy khi dang l?y bi
#define BTN         PIN_B7   // Nút nh?n ph?c h?i (Active-LOW)
#define SL          PIN_A2   // IR Trái (Active-LOW)
#define SM          PIN_A3   // IR Gi?a
#define SR          PIN_A4   // IR Ph?i
#define Trig        PIN_A0   // Siêu âm Trigger
#define Echo        PIN_A1   // Siêu âm Echo
#define STBY        PIN_C4   // TB6612 Standby (Ph?i HIGH d? ch?y)
#define AIN1        PIN_C0   // DIR Motor Trái 1
#define AIN2        PIN_C3   // DIR Motor Trái 2 (N?i I/O d? lùi)
#define BIN1        PIN_C5   // DIR Motor Ph?i 1 (Ðã d?i t? B2)
#define BIN2        PIN_C6   // DIR Motor Ph?i 2 (Ðã d?i t? B3)
#define PWMA_PIN    PIN_C2   // Software PWM Motor Trái (Thay cho CCP1)
#define PWMB_PIN    PIN_C1   // Software PWM Motor Ph?i (Thay cho CCP2)
#define GRAB_PIN    PIN_C4   // Co c?u l?y bi (Pure mechanical timer)
#define RELEASE_PIN PIN_C5   // Co c?u th? bi

// --- H?ng s? di?u ch?nh [1, 2] ---
#define CHATTER_N   3        // L?c nhi?u 3 m?u liên ti?p
#define GRAB_MS     2000     // Ch? g?p/th? bi 2s
#define BACK_MS     600      // Th?i gian lùi r?i tr?m
#define STOP_CM     3        // Kho?ng cách d?ng t?i tr?m
#define END_CM      5        // Kho?ng cách nh?n di?n dích

// --- Ki?u d? li?u và Bi?n toàn c?c [3, 4] ---
typedef enum { 
    STATE_IDLE, STATE_FOLLOW_LINE, STATE_STATION, 
    STATE_STATION_STOP, STATE_STATION_BACK, STATE_NAVIGATION, 
    STATE_END 
} SystemState_t;

volatile uint32_t ms_tick = 0;
volatile uint8_t duty_L = 0;   // Duty cycle Software PWM Trái (0-250)
volatile uint8_t duty_R = 0;   // Duty cycle Software PWM Ph?i (0-250)
volatile int1 led2_en = 0;     // C? cho phép nháy LED2
SystemState_t current_state = STATE_IDLE;
uint8_t checkpoint_id = 1;     // CP1=Start, CP2=Sau Tr?m
int1 ball_taken = 0;           // C? xác nh?n dã có bi
uint8_t last_pat = 0x02;       // Pattern g?n nh?t (m?c d?nh di th?ng)

// --- B?ng tra c?u Motor (Lookup Table) [5, 6] ---
typedef struct { uint8_t L; uint8_t R; } MotorCmd_t;
const MotorCmd_t motor_table[7] = {
    {0,   0  }, // 000: M?t line
    {200, 0  }, // 001: SR den -> r? g?t ph?i
    {160, 160}, // 010: SM den -> TH?NG
    {160, 90 }, // 011: SM+SR den -> r? ph?i nh?
    {0,   200}, // 100: SL den -> r? g?t trái
    {0,   0  }, // 101: Junction T
    {90,  160}, // 110: SL+SM den -> r? trái nh?
    {0,   0  }  // 111: Ngã tu
};

// --- Trình ph?c v? ng?t ---
#int_timer0 // Nh?p tim h? th?ng 1ms [8]
void timer0_isr() {
    ms_tick++;
    set_timer0(64286); // Reset cho 1ms @ 20MHz (5MHz/prescale 4)
    if (led2_en && (ms_tick % 200 == 0)) output_toggle(LED2);
    else if (!led2_en) output_low(LED2);
}

#int_timer2 // Software PWM d?ng b? @ 20kHz [76, History]
void timer2_isr() {
    static uint8_t cnt = 0;
    if (++cnt >= 250) cnt = 0; // PR2 tuong duong 250
    
    // Motor Trái (RC2)
    if (cnt < duty_L) output_high(PWMA_PIN);
    else output_low(PWMA_PIN);
    
    // Motor Ph?i (RC3)
    if (cnt < duty_R) output_high(PWMB_PIN);
    else output_low(PWMB_PIN);
}

// --- Hàm ch?c nang ---
void motor_set(uint8_t l, uint8_t r, uint8_t dir) {
    output_high(STBY);
    if (dir == 1) {      // Ti?n
        output_high(AIN1); output_low(AIN2); output_high(BIN1); output_low(BIN2);
    } else if (dir == 2) { // Lùi [History]
        output_low(AIN1); output_high(AIN2); output_low(BIN1); output_high(BIN2);
    } else {             // D?ng
        output_low(AIN1); output_low(AIN2); output_low(BIN1); output_low(BIN2);
        l = 0; r = 0;
    }
    duty_L = l; duty_R = r;
}

uint8_t get_dist() { // HC-SR04 formula [9, 10]
    output_high(Trig); delay_us(10); output_low(Trig);
    uint32_t to = 0;
    while(!input(Echo) && ++to < 2000);
    set_timer1(0);
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_1); // 5MHz
    while(input(Echo));
    setup_timer_1(T1_DISABLED);
    return (uint8_t)(get_timer1() / 290); // 1cm = 290 ticks
}

uint8_t read_pattern() { // L?c nhi?u Chatter N=3 [11, 12]
    static uint8_t p_prev = 0xFF, p_cnt = 0, p_conf = 0x02;
    uint8_t raw = (!input(SL) << 2) | (!input(SM) << 1) | !input(SR);
    if (raw == p_prev) {
        if (++p_cnt >= CHATTER_N) p_conf = raw;
    } else {
        p_prev = raw; p_cnt = 1;
    }
    return p_conf;
}

void transition(SystemState_t next) {
    ms_tick = 0;
    current_state = next;
}

void main() {
    setup_adc_ports(NO_ANALOGS); // ADCON1 = 0x0F [5, 13]
    output_low(LED1); output_low(LED2);
    output_low(GRAB_PIN); output_low(RELEASE_PIN);
    
    setup_timer_0(T0_INTERNAL | T0_DIV_4);
    setup_timer_2(T2_DIV_BY_1, 249, 1); // 20kHz base
    setup_ccp1(CCP_OFF); // Dùng Software PWM hoàn toàn [History]
    
    enable_interrupts(INT_TIMER0);
    enable_interrupts(INT_TIMER2);
    enable_interrupts(GLOBAL);
    
    transition(STATE_IDLE);

    while(TRUE) {
        uint8_t pat = read_pattern();
        uint8_t dist = (current_state == STATE_STATION || current_state == STATE_FOLLOW_LINE) ? get_dist() : 255;

        switch(current_state) {
            case STATE_IDLE:
                motor_set(0, 0, 0);
                if (!input(BTN)) {
                    delay_ms(50);
                    uint32_t t_start = ms_tick;
                    while(!input(BTN));
                    if (ms_tick - t_start > 2000) { // Nh?n gi? 2s: Reset Hard [History]
                        checkpoint_id = 1; ball_taken = 0;
                    }
                    output_high(LED1); transition(STATE_FOLLOW_LINE);
                }
                break;

            case STATE_FOLLOW_LINE:
                if (ball_taken == 0 && (pat == 0x05 || pat == 0x07)) transition(STATE_STATION);
                else if (dist <= END_CM) transition(STATE_END);
                else {
                    if (pat == 0) pat = last_pat; // M?t line: gi? hu?ng cu?i [14]
                    else last_pat = pat;
                    motor_set(motor_table[pat].L, motor_table[pat].R, 1);
                }
                break;

            case STATE_STATION:
                motor_set(80, 80, 1); // Bám ch?m vào tr?m [2]
                if (dist <= STOP_CM) transition(STATE_STATION_STOP);
                break;

            case STATE_STATION_STOP:
                motor_set(0, 0, 0);
                output_high(GRAB_PIN); led2_en = 1;
                if (ms_tick > GRAB_MS) { // Ch? 2s co c?u co khí [15]
                    ball_taken = 1; led2_en = 0;
                    output_low(GRAB_PIN); transition(STATE_STATION_BACK);
                }
                break;

            case STATE_STATION_BACK:
                motor_set(120, 120, 2); // Lùi r?i tr?m [16]
                if (ms_tick > BACK_MS) transition(STATE_NAVIGATION);
                break;

            case STATE_NAVIGATION:
                motor_set(200, 0, 1); // Quay ph?i tìm line chính [17, 18]
                if (input(SM) && ms_tick > 200) {
                    checkpoint_id = 2; transition(STATE_FOLLOW_LINE);
                }
                break;

            case STATE_END:
                if (ms_tick < 400) motor_set(100, 100, 1); // Ti?n thêm d? ti?p xúc [19]
                else if (ms_tick < 2400) {
                    motor_set(0, 0, 0); output_high(RELEASE_PIN);
                } else {
                    output_low(RELEASE_PIN); output_low(LED1);
                    transition(STATE_IDLE);
                }
                break;
        }
    }
}

