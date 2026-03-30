////////////////////////////////////////////////////////////////////////////////
// FILE: main.c
// DESC: Main program - initialization and FSM dispatch loop
// PROJECT: Line Follower Robot with Ball Pickup/Delivery
// MCU: PIC18F2685 @ 20MHz
////////////////////////////////////////////////////////////////////////////////

#include "main.h"

// CCS C Compiler requires including .c files
#include "sensors.c"
#include "motor.c"
#include "ultrasonic.c"
#include "fsm.c"

// ============================================================================
// GLOBAL TIMING VARIABLES (volatile - modified in ISR)
// ============================================================================
volatile int32 ms_tick = 0;        // System millisecond counter
volatile int16 led_tick = 0;       // LED blink counter

// ============================================================================
// TIMER0 INTERRUPT SERVICE ROUTINE
// Triggers every 2ms to provide system heartbeat
// ============================================================================
#INT_TIMER0
void timer0_isr(void) {
    // Reload Timer0 for 2ms interval
    // Timer0 in 16-bit mode at 20MHz with 1:4 prescaler
    // Calculation: 65536 - (2ms * 20MHz / 4 / 4) = 65536 - 2500 = 63036
    set_timer0(63036);
    
    ms_tick += 2;  // Increment millisecond counter
    
    // Read sensors and apply chatter filter
    int8 raw_pattern = sensors_read_raw();
    sensors_chatter_filter(raw_pattern);
    
    // Handle LED blinking for action LED
    led_tick++;
    if (led_tick >= 125) {  // 125 * 2ms = 250ms
        led_tick = 0;
    }
}

// ============================================================================
// PORT B CHANGE INTERRUPT
// Handles RUN button press and ball sensor
// ============================================================================
#INT_RB
void portb_change_isr(void) {
    int8 portb = input_b();  // Read to clear mismatch
    
    // Check RUN button (RB7)
    if (!input(BUTTON_RUN)) {  // Active low
        delay_ms(50);  // Debounce
        if (!input(BUTTON_RUN)) {
            run_flag = TRUE;
        }
    }
    
    // Check ball sensor (RB3) - optional
    if (input(BALL_SENSOR)) {
        ball_grabbed = TRUE;
    }
}

// ============================================================================
// GPIO INITIALIZATION
// CRITICAL: Must be called FIRST before any other init
// ============================================================================
void gpio_init(void) {
    // Disable all analog inputs - set all as digital
    setup_adc(ADC_OFF);
    setup_adc_ports(NO_ANALOGS);
    
    // Alternative: ADCON1 = 0x0F for all digital
    #byte ADCON1 = 0xFC1
    ADCON1 = 0x0F;
    
    // Configure TRIS registers
    // Inputs: RA0-RA2 (sensors), RB3 (ball sensor), RB5 (echo), RB7 (button)
    // Outputs: All others
    
    set_tris_a(0b00000111);  // RA0-RA2 as inputs
    set_tris_b(0b10101000);  // RB3, RB5, RB7 as inputs
    set_tris_c(0b00000000);  // All outputs
    
    // Initialize all outputs low
    output_low(MOTOR_PWMA);
    output_low(MOTOR_PWMB);
    output_low(MOTOR_AIN1);
    output_low(MOTOR_BIN1);
    output_low(MOTOR_STBY);
    output_low(LED_STATUS);
    output_low(LED_ACTION);
    output_low(SERVO_GRAB);
    output_low(RELAY_RELEASE);
    output_low(ULTRA_TRIG);
    
    // Enable weak pull-ups on Port B for buttons
    port_b_pullups(0b10000000);  // Pull-up on RB7 (RUN button)
}

// ============================================================================
// TIMERS INITIALIZATION
// ============================================================================
void timers_init(void) {
    // Timer0: System tick (2ms interval) for chattering, LED, ms_tick
    // 16-bit mode, internal clock, 1:4 prescaler
    // Calculation: 2ms = (65536 - TMR0) * 4 / (20MHz/4)
    // TMR0 = 65536 - 2500 = 63036
    setup_timer_0(RTCC_INTERNAL | RTCC_DIV_4 | RTCC_8_BIT);
    set_timer0(63036);
    
    // Timer1: Free-running for ultrasonic sensor
    // Will be reset when needed, but stays enabled
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_1);
    
    // Timer2: Used by PWM @ 20kHz (configured in motor_init)
    // PR2 = 249, prescaler = 1 → 20MHz/(4*250*1) = 20kHz
}

// ============================================================================
// PWM INITIALIZATION
// ============================================================================
void pwm_init(void) {
    // PWM initialization is handled in motor_init()
    motor_init();
}

// ============================================================================
// INTERRUPTS INITIALIZATION
// ============================================================================
void interrupts_init(void) {
    // Enable Timer0 interrupt
    enable_interrupts(INT_TIMER0);
    
    // Enable Timer2 interrupt (software PWM for right motor)
    enable_interrupts(INT_TIMER2);
    
    // Enable Port B change interrupt for button
    enable_interrupts(INT_RB);
    
    // Enable global interrupts
    enable_interrupts(GLOBAL);
}

// ============================================================================
// SYSTEM INITIALIZATION (calls all init functions in correct order)
// ============================================================================
void system_init(void) {
    // CRITICAL: GPIO must be initialized FIRST
    gpio_init();
    
    // Initialize subsystems
    sensors_init();
    ultrasonic_init();
    timers_init();
    pwm_init();
    
    // Initialize interrupts LAST
    interrupts_init();
    
    // Brief delay to stabilize
    delay_ms(100);
    
    // Flash LEDs to indicate system ready
    sensors_led_status(TRUE);
    sensors_led_action(TRUE);
    delay_ms(500);
    sensors_led_status(FALSE);
    sensors_led_action(FALSE);
}

// ============================================================================
// PERIPHERAL CONTROL - GRAB BALL
// ============================================================================
void peripherals_grab_ball(void) {
    // Activate servo/grabber mechanism
    output_high(SERVO_GRAB);
    sensors_led_action(TRUE);
}

// ============================================================================
// PERIPHERAL CONTROL - RELEASE BALL
// ============================================================================
void peripherals_release_ball(void) {
    // Activate release mechanism (relay or servo)
    output_high(RELAY_RELEASE);
    delay_ms(RELEASE_MS);
    output_low(RELAY_RELEASE);
    output_low(SERVO_GRAB);
    sensors_led_action(FALSE);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================
int32 get_ms_tick(void) {
    return ms_tick;
}

int1 check_timeout(int32 start_time, int32 timeout_ms) {
    return ((ms_tick - start_time) >= timeout_ms);
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================
void main(void) {
    // Initialize all systems
    system_init();
    
    // Start in IDLE state
    current_state = STATE_IDLE;
    state_idle_entry();
    
    // Main FSM dispatch loop
    while (TRUE) {
        // Dispatch to current state's update function
        switch (current_state) {
            case STATE_IDLE:
                state_idle_update();
                break;
                
            case STATE_FOLLOW_LINE:
                state_follow_line_update();
                break;
                
            case STATE_STATION:
                state_station_update();
                break;
                
            case STATE_STATION_STOP:
                state_station_stop_update();
                break;
                
            case STATE_STATION_BACK:
                state_station_back_update();
                break;
                
            case STATE_NAVIGATION:
                state_navigation_update();
                break;
                
            case STATE_END:
                state_end_update();
                break;
                
            case STATE_ERROR:
                state_error_update();
                break;
                
            case STATE_CHECKPOINT:
                state_checkpoint_update();
                break;
                
            default:
                // Invalid state - reset to IDLE
                fsm_transition(STATE_IDLE);
                break;
        }
        
        // Small delay to prevent tight looping (optional)
        delay_ms(10);
    }
}
