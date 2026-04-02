#include <18F2685.h>
#fuses HS, NOWDT, NOPROTECT, NOLVP
#use delay(clock=20000000)

#define ULTRA_TRIG PIN_C0
#define ULTRA_ECHO PIN_B5
#define LED1 PIN_B2
#define BUTTON_POWER PIN_B7

int1 system_on = FALSE;
int1 led_state = FALSE;
int16 blink_counter = 0;
int16 distance_cm = 0;

int16 ultrasonic_read(void) {
    int32 pulse_width;
    
    output_low(ULTRA_TRIG);
    delay_us(2);
    output_high(ULTRA_TRIG);
    delay_us(10);
    output_low(ULTRA_TRIG);
    
    set_timer1(0);
    while(!input(ULTRA_ECHO) && get_timer1() < 50000);
    
    if (get_timer1() >= 50000) return 999;
    
    set_timer1(0);
    while(input(ULTRA_ECHO) && get_timer1() < 50000);
    
    pulse_width = get_timer1();
    
    if (pulse_width >= 50000) return 999;
    
    // distance = pulse_width / 58 (for cm)
    return (int16)(pulse_width / 58);
}

#INT_TIMER0
void timer0_isr(void) {
    set_timer0(61536); // 1ms
    
    if (system_on) {
        // Nháy LED n?u kho?ng cách <= 5cm
        if (distance_cm <= 5 && distance_cm > 0) {
            blink_counter++;
            if (blink_counter >= 250) { // 250ms
                blink_counter = 0;
                led_state = !led_state;
                output_bit(LED1, led_state);
            }
        } else {
            // T?t LED n?u > 5cm
            blink_counter = 0;
            led_state = FALSE;
            output_low(LED1);
        }
    }
}

#INT_RB
void portb_isr(void) {
    int8 portb = input_b();
    delay_ms(50);
    
    if (!input(BUTTON_POWER)) {
        system_on = !system_on;
        if (!system_on) {
            output_low(LED1);
        }
    }
}

void main(void) {
    setup_adc(ADC_OFF);
    setup_adc_ports(NO_ANALOGS);
    
    set_tris_b(0b10100011);
    set_tris_c(0b00000000);
    port_b_pullups(0b10000011);
    
    output_low(LED1);
    output_low(ULTRA_TRIG);
    
    setup_timer_0(RTCC_INTERNAL | RTCC_DIV_8);
    set_timer0(61536);
    
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_1);
    
    enable_interrupts(INT_TIMER0);
    enable_interrupts(INT_RB);
    enable_interrupts(GLOBAL);
    
    while(TRUE) {
        if (system_on) {
            distance_cm = ultrasonic_read();
            delay_ms(100);
        } else {
            delay_ms(100);
        }
    }
}

