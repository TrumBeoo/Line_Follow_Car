
#include <18F2685.h>
#fuses HS, NOWDT, NOPROTECT, NOLVP
#use delay(clock=20000000)

// Motor A
#define MOTOR_PWMA PIN_C2
#define MOTOR_AIN1 PIN_C0
#define MOTOR_AIN2 PIN_C3

// Motor B
#define MOTOR_PWMB PIN_C1
#define MOTOR_BIN1 PIN_C5
#define MOTOR_BIN2 PIN_C6

// Chung
#define MOTOR_STBY PIN_C4

volatile int8 pwm_duty_a = 0;
volatile int8 pwm_duty_b = 0;
volatile int8 pwm_counter = 0;

#INT_TIMER2
void timer2_isr(void) {
    output_bit(MOTOR_PWMA, pwm_counter < pwm_duty_a);
    output_bit(MOTOR_PWMB, pwm_counter < pwm_duty_b);
    pwm_counter++;
    if (pwm_counter >= 100) {
        pwm_counter = 0;
    }
}

void motor_init(void) {
    setup_timer_2(T2_DIV_BY_4, 49, 1);
    pwm_duty_a = 0;
    pwm_duty_b = 0;
    output_low(MOTOR_STBY);
    enable_interrupts(INT_TIMER2);
}

void motor_stop(void) {
    output_low(MOTOR_STBY);
    pwm_duty_a = 0;
    pwm_duty_b = 0;
}

void motor_forward(int8 speed) {
    output_high(MOTOR_STBY);
    output_high(MOTOR_AIN1);
    output_low(MOTOR_AIN2);
    output_high(MOTOR_BIN1);
    output_low(MOTOR_BIN2);
    pwm_duty_a = speed;
    pwm_duty_b = speed;
}

void motor_backward(int8 speed) {
    output_high(MOTOR_STBY);
    output_low(MOTOR_AIN1);
    output_high(MOTOR_AIN2);
    output_low(MOTOR_BIN1);
    output_high(MOTOR_BIN2);
    pwm_duty_a = speed;
    pwm_duty_b = speed;
}

void motor_turn_left(int8 speed) {
    output_high(MOTOR_STBY);
    output_low(MOTOR_AIN1);
    output_high(MOTOR_AIN2);
    output_high(MOTOR_BIN1);
    output_low(MOTOR_BIN2);
    pwm_duty_a = speed;
    pwm_duty_b = speed;
}

void motor_turn_right(int8 speed) {
    output_high(MOTOR_STBY);
    output_high(MOTOR_AIN1);
    output_low(MOTOR_AIN2);
    output_low(MOTOR_BIN1);
    output_high(MOTOR_BIN2);
    pwm_duty_a = speed;
    pwm_duty_b = speed;
}

void main(void) {
    setup_adc(ADC_OFF);
    setup_adc_ports(NO_ANALOGS);
    
    motor_init();
    enable_interrupts(GLOBAL);
    
    while(TRUE) {
        motor_forward(80);
        delay_ms(2000);
        
        motor_backward(80);
        delay_ms(2000);
        
        motor_turn_left(60);
        delay_ms(2000);
        
        motor_turn_right(60);
        delay_ms(2000);
        
        motor_stop();
        delay_ms(2000);
    }
}

