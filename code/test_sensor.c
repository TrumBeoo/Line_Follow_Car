#include <18F2685.h>
#fuses HS, NOWDT, NOPROTECT, NOLVP
#use delay(clock=20000000)

#define SENSOR_LEFT PIN_A2
#define SENSOR_MID PIN_A3
#define SENSOR_RIGHT PIN_A4
#define LED1 PIN_B2
#define LED2 PIN_B3
#define BUTTON_POWER PIN_B7

int1 system_on = FALSE;
int1 led_state = FALSE;
int16 blink_counter = 0;
int1 button_pressed = FALSE;

#INT_TIMER0
void timer0_isr(void) {
    set_timer0(61536);
    
    if (system_on) {
        int1 left = input(SENSOR_LEFT);
        int1 mid = input(SENSOR_MID);
        int1 right = input(SENSOR_RIGHT);
        
        int1 any_black = (left || mid || right);
        
        if (any_black) {
            blink_counter++;
            if (blink_counter >= 250) {
                blink_counter = 0;
                led_state = !led_state;
                output_bit(LED1, led_state);
                output_bit(LED2, !led_state);
            }
        } else {
            blink_counter = 0;
            output_low(LED1);
            output_low(LED2);
        }
    }
}

#INT_RB
void portb_isr(void) {
    int8 portb = input_b();
    button_pressed = TRUE;
}

void main(void) {
    setup_adc(ADC_OFF);
    setup_adc_ports(NO_ANALOGS);
    
    set_tris_a(0b00011111);
    set_tris_b(0b10000011);
    port_b_pullups(0b10000011);
    
    output_low(LED1);
    output_low(LED2);
    
    setup_timer_0(RTCC_INTERNAL | RTCC_DIV_8);
    set_timer0(61536);
    
    enable_interrupts(INT_TIMER0);
    enable_interrupts(INT_RB);
    enable_interrupts(GLOBAL);
    
    while(TRUE) {
        if (button_pressed) {
            delay_ms(50);
            if (!input(BUTTON_POWER)) {
                system_on = !system_on;
                if (!system_on) {
                    output_low(LED1);
                    output_low(LED2);
                }
            }
            button_pressed = FALSE;
        }
        delay_ms(10);
    }
}

