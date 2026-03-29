// ============================================================================
// FILE: main.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: Main program with initialization, interrupts, and FSM dispatcher
// ============================================================================

#include "main.h"

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// System timing
volatile uint32_t ms_tick = 0;
volatile uint32_t led_tick = 0;

// State machine
volatile SystemState_t current_state = STATE_IDLE;
volatile SystemState_t next_state = STATE_IDLE;

// Control flags
volatile bool run_flag = false;
volatile bool stop_flag = false;
volatile bool ball_ok = false;

// Sensor data
volatile uint8_t sensor_pattern = 0;
volatile uint8_t last_valid_pattern = PATTERN_CENTER;

// Checkpoint management
volatile uint8_t checkpoint_state = 0;

// Error handling
volatile uint8_t error_count = 0;
volatile ErrorType_t error_type = ERR_NONE;

// ============================================================================
// INTERRUPT SERVICE ROUTINES
// ============================================================================

/**
 * Timer0 ISR - System tick @ 2ms
 * Handles: ms_tick increment, chatter filter update, LED blinking
 */
#INT_TIMER0
void timer0_isr(void) {
    // Reload Timer0 for 2ms period
    // For 20MHz, Timer0 16-bit with 1:256 prescaler
    // Instruction cycle = 20MHz/4 = 5MHz
    // Ticks needed = 2ms * 5MHz / 256 = 39.0625 ≈ 39
    // TMR0 = 65536 - 39 = 65497 = 0xFFD9
    set_timer0(65497);
    
    // Increment system tick
    ms_tick += 2;
    
    // Update LED blink counter
    led_tick += 2;
    
    // Non-blocking LED blink for action LED (when needed)
    // This can be used for visual feedback during operations
}

/**
 * IOCB ISR - Interrupt-on-Change for Port B
 * Handles: RUN button (RB7), STOP button / Ball sensor (RB3)
 */
#INT_RB
void iocb_isr(void) {
    static uint32_t last_run_time = 0;
    static uint32_t last_stop_time = 0;
    
    // Read port to clear mismatch condition
    uint8_t portb_state = input_b();
    
    // Check RUN button (RB7) with debounce
    if(!input(BTN_RUN)) {
        if((ms_tick - last_run_time) > DEBOUNCE_MS) {
            run_flag = true;
            last_run_time = ms_tick;
        }
    }
    
    // Check STOP/Ball sensor (RB3) with debounce
    if(!input(BTN_STOP)) {
        if((ms_tick - last_stop_time) > DEBOUNCE_MS) {
            // Context-dependent: STOP button or ball detection
            if(current_state == STATE_STATION_STOP) {
                ball_ok = true;
            } else {
                stop_flag = true;
            }
            last_stop_time = ms_tick;
        }
    }
    
    // Clear interrupt flag
    clear_interrupt(INT_RB);
}

// ============================================================================
// INITIALIZATION FUNCTIONS
// ============================================================================

/**
 * Initialize GPIO pins
 * MUST be called first to set ADCON1 for digital I/O
 */
void gpio_init(void) {
    // Configure all Port A as digital I/O (disable ADC)
    setup_adc_ports(NO_ANALOGS);
    setup_adc(ADC_OFF);
    
    // Alternative method for digital mode
    #byte ADCON1 = 0xFC1
    ADCON1 = 0x0F;  // All digital
    
    // Set TRIS registers (1=input, 0=output)
    // Port A: RA0-RA2 inputs (sensors), rest can be I/O
    set_tris_a(0b00000111);
    
    // Port B: RB3,RB5,RB7 inputs (stop/ball, echo, run), RB4 output (trig)
    //         RB0,RB2,RB6 outputs (motor direction, STBY)
    set_tris_b(0b10101000);
    
    // Port C: RC0,RC1,RC4,RC5 outputs (LEDs, servo, drop)
    //         RC2,RC3 outputs (PWM - will be configured by CCP)
    set_tris_c(0b00000000);
    
    // Initialize outputs to safe state
    output_low(AIN1_PIN);
    output_low(BIN1_PIN);
    output_low(STBY_PIN);      // Motor driver disabled initially
    output_low(TRIG_PIN);
    output_low(LED_STATUS);
    output_low(LED_ACTION);
    output_low(SERVO_GRAB_PIN);
    output_low(DROP_PIN);
}

/**
 * Initialize Timer0 for system tick
 * 16-bit mode, 2ms period @ 20MHz
 */
void timer_init(void) {
    // Timer0: 16-bit, internal clock, 1:256 prescaler
    // Period = 4 * prescaler * (65536 - TMR0) / Fosc
    // For 2ms: TMR0 = 65536 - (20MHz * 0.002 / (4 * 256)) = 65497
    setup_timer_0(RTCC_INTERNAL | RTCC_DIV_256);
    set_timer0(65497);
    enable_interrupts(INT_TIMER0);
    
    // Timer1: 16-bit, internal clock, 1:1 prescaler for ultrasonic
    // Used for precise microsecond timing (0.2us resolution)
    // Will be started/stopped in ultrasonic.c as needed
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_1);
    set_timer1(0);
    // Note: Timer1 interrupt not enabled - used for pulse width measurement only
    
    // Timer2: Used for PWM (configured in pwm_init)
}

/**
 * Initialize PWM for motor control
 * PWM frequency: 20kHz (suitable for TB6612)
 */
void pwm_init(void) {
    // PWM frequency = Fosc / (4 * prescaler * (PR2 + 1))
    // For 20kHz: PR2 = Fosc / (4 * prescaler * freq) - 1
    // With prescaler=1: PR2 = 20MHz / (4 * 1 * 20kHz) - 1 = 249
    
    setup_timer_2(T2_DIV_BY_1, 249, 1);
    
    // Setup CCP1 for PWM (RC2 - Motor A)
    setup_ccp1(CCP_PWM);
    set_pwm1_duty(0);  // Start with 0% duty
    
    // Setup CCP2 for PWM (RC1 by default, but we want RC3 - Motor B)
    // Note: On PIC18F2685, CCP2 can be remapped
    // Check datasheet for CCP2MX bit in CONFIG3H
    // If CCP2MX=1, CCP2 is on RC1; if CCP2MX=0, CCP2 is on RB3
    // We need to use RC3, so we'll use CCP1 for one motor and
    // software PWM or another CCP module for the second motor
    
    // For simplicity, assuming CCP2 is available on RC1 and
    // we'll remap or use RC3 via bit-bang if needed
    setup_ccp2(CCP_PWM);
    set_pwm2_duty(0);  // Start with 0% duty
    
    // Note: If CCP2 mapping is an issue, consider:
    // 1. Using hardware PWM on available pins
    // 2. Implementing software PWM for one motor
    // 3. Using external PWM generator IC
}

/**
 * Initialize interrupts
 * MUST be called last after all peripherals are configured
 */
void interrupt_init(void) {
    // Enable interrupt-on-change for Port B (RB3, RB7)
    #byte INTCON3 = 0xFF0
    #bit IOCB_ENABLE = INTCON3.0
    
    enable_interrupts(INT_RB);
    IOCB_ENABLE = 1;
    
    // Enable global and peripheral interrupts
    enable_interrupts(GLOBAL);
    enable_interrupts(PERIPH);
}

/**
 * Complete system initialization
 * Call all init functions in proper order
 */
void system_init(void) {
    // Order is critical: GPIO -> Timers -> PWM -> Interrupts
    gpio_init();
    timer_init();
    pwm_init();
    
    // Initialize modules
    sensor_calibrate();  // Optional calibration
    motor_disable();     // Ensure motor is off initially
    
    // Enable interrupts last
    interrupt_init();
    
    // Initial state
    current_state = STATE_IDLE;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Microsecond delay (using built-in delay_us)
 */
void delay_us(uint16_t us) {
    delay_us(us);
}

/**
 * Millisecond delay (using ms_tick for non-blocking delays in interrupts)
 */
void delay_ms(uint16_t ms) {
    uint32_t start = ms_tick;
    while((ms_tick - start) < ms);
}

// ============================================================================
// MAIN PROGRAM
// ============================================================================

void main(void) {
    // Initialize system
    system_init();
    
    // Flash LEDs to indicate system ready
    for(uint8_t i = 0; i < 3; i++) {
        led_status_on();
        led_action_on();
        delay_ms(100);
        led_status_off();
        led_action_off();
        delay_ms(100);
    }
    
    // Main FSM dispatcher loop
    while(1) {
        // Execute update function based on current state
        switch(current_state) {
            case STATE_IDLE:
                idle_update();
                break;
                
            case STATE_FOLLOW_LINE:
                follow_line_update();
                break;
                
            case STATE_STATION:
                station_update();
                break;
                
            case STATE_STATION_STOP:
                station_stop_update();
                break;
                
            case STATE_STATION_BACK:
                station_back_update();
                break;
                
            case STATE_NAVIGATION:
                navigation_update();
                break;
                
            case STATE_END_APPROACH:
                end_approach_update();
                break;
                
            case STATE_END:
                end_update();
                break;
                
            case STATE_ERROR:
                error_update();
                break;
                
            case STATE_CHECKPOINT_1:
                checkpoint1_update();
                break;
                
            case STATE_CHECKPOINT_2:
                checkpoint2_update();
                break;
                
            case STATE_CHECKPOINT_3:
                checkpoint3_update();
                break;
                
            default:
                // Invalid state, go to IDLE
                transition(STATE_IDLE);
                break;
        }
        
        // Small delay to prevent tight loop (optional)
        // The FSM should be responsive, so keep this minimal
        delay_ms(10);
    }
}