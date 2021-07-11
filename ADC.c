#include <xc.h>
#include "pragmas.h"
#include "ADC.h"
#include "LCD.h"
#include <string.h>
#include <stdio.h>


int program_state = 1 ; // Program state, 1 => Password Set, 2 => Password Check, 3 => Success, 4 => Fail

int first_number = -1, second_number = -1, third_number = -1; // In password set state, I change these variables to save inputted numbers.

int entered_password_number = 0 ; // I increment this variable while taking input in password set and check states.

int first_number_guess = -1, second_number_guess = -1, third_number_guess = -1 ; // In password check state, I change these variables to save inputted numbers.

int remaining_seconds = 90; // Remaining seconds in password check state.

int remaining_attempts = 3; // Remaining attempts in password check state. 

int display2_value=-1,display3_value=-1,display4_value=-1; // These values will be displayed in display2,3,4 respectively.

int is_change_in_password_check = 0; // I use this flag to check changes in password check state. 
                                     // If RB0 button is pressed or ADC value is changed I set this to another value and start countdown.

int read_thermometer = 0; // Read thermometer flag. If user enters 2 wrong password, I set this to 1 and start reading thermometer.

int is_thermometer_readed = 0; // I use this to indicate I am currently reading thermometer from second channel.

int counter_for_1sec = 0; // I am using this to count 1 second from timer-0 interrupt. It generates 500ms interrupt.

int flag_for_port_blinking = 0; // I am using this flag to blink PORTB when remanining attempts decrease to 2.

int display_number = 2; // I am using this number to set proper display in timer-1 interrupt. If it is 2, I load display2_value to display2.


// Seven segment encodings for displaying values.
#define number0_for_display 0b00111111  
#define number1_for_display 0b00000110
#define number2_for_display 0b01011011 
#define number3_for_display 0b01001111
#define number4_for_display 0b01100110
#define number5_for_display 0b01101101
#define number6_for_display 0b01111101
#define number7_for_display 0b00000111
#define number8_for_display 0b01111111
#define number9_for_display 0b01101111
#define attempts3_left 0b01001001
#define attempts2_left 0b01001000
#define attempts1_left 0b00001000
#define empty_for_display 0b00000000



void set_numbers_to_the_LCD(int which_number,int number);
void failed();
void start_timer0_interrupt_for_500ms();
void successed();
void start_timer1_interrupt();
void stop_timer1_interrupt();
void determine_display_values();


// I use 5 interrupts which are external,  A/D, timer-0, timer-1 and RB port change. I commented before each of them.
void __interrupt(high_priority) FNC()
{
    
    // External interrupt handler. If program state is 2 which is password check, 
    // I check guesses with actual values do the necessary actions. 
    // If guess is true I increment entered_password_number and if it reaches 3, I call successed function and program enters success state.
    // If guess is false, I do necessary actions according to remaining attempt. If attempt is 2, I set blink flag to 1 to blink PORTB.
    // If attempt is 1, I turn one heater and set 1 to the read_thermometer flag.
    // If attempt is 0, I call failed function and program enters failed state.
    // If program state is 1 which is password set,
    // I just increment entered_password_number and if it reaches 3, I change program state to 2.
    if (INTCONbits.INT0IF) {
        INTCONbits.INT0IF = 0;
        if (program_state == 2) {
            if (first_number_guess > 99 || second_number_guess > 99 || third_number_guess > 99) { }
            else {
                if (is_change_in_password_check == 0) {
                    is_change_in_password_check = 5;
                    start_timer0_interrupt_for_500ms();
                }
                if (entered_password_number == 2) {
                    if (third_number_guess == third_number)
                        successed(); 
                    else {
                        remaining_attempts--;
                        if (remaining_attempts == 0)
                            failed();
                        if (remaining_attempts == 1) {
                            PORTCbits.RC5 = 1;
                            read_thermometer = 1;
                        }
                        if (remaining_attempts == 2)
                            flag_for_port_blinking = 1; 
                    }
                }
                if (entered_password_number == 1) {
                    if (second_number_guess == second_number)
                        entered_password_number++;
                    else {
                        remaining_attempts--;
                        if (remaining_attempts == 0)
                            failed();
                        if (remaining_attempts == 1) {
                            PORTCbits.RC5 = 1;
                            read_thermometer = 1;
                        }
                        if (remaining_attempts == 2)
                            flag_for_port_blinking = 1;                       
                    }
                }
                if (entered_password_number == 0) {
                    if (first_number_guess == first_number)
                        entered_password_number++;
                    else {
                        remaining_attempts--;
                        if (remaining_attempts == 0)
                            failed();
                        if (remaining_attempts == 1) {
                            PORTCbits.RC5 = 1;
                            read_thermometer = 1;
                        }
                        if (remaining_attempts == 2)
                            flag_for_port_blinking = 1;                    
                    }
                }
            }
        }

        
        
        if (program_state == 1) {
            if (first_number > 99 || second_number > 99 || third_number > 99) {
            } else {
                entered_password_number++;
                if (entered_password_number == 3) {
                    program_state = 2;
                    entered_password_number = 0;
                    LCDGoto(1, 1);
                    LCDStr("Input Password: \0");
                    LCDGoto(4, 2);
                    LCDStr("__\0");
                    LCDGoto(7, 2);
                    LCDStr("__       \0");
                    start_timer1_interrupt();
                }
            }
        }

        return;
    }


    // A/D interrupt handler. In password set state, I change number values to save inputted numbers, and change the LCD.
    // In password check state, I first check read_thermometer and  is_thermometer_readed flags, and if they are 1 I check celcius value.
    // If celcius is greater than 40, I call failed and program enters failed state. 
    // I use is_thermometer_readed flag to seperate potentiometer and thermometer values.
    // If I read potentiometer, I set guesses to this value according to entered_password_number and show them on the LCD.   
    if (PIR1bits.ADIF) {
        PIR1bits.ADIF = 0;
        int conversion_value = (int) ((ADRESH << 8) + ADRESL);
        float celcius = (conversion_value * 5.0f / 1023.0f * 100.0f);
        conversion_value = conversion_value / 10;
        if (program_state == 1) {
            if (entered_password_number == 0) {
                if (conversion_value != first_number) {
                    first_number = conversion_value;
                    set_numbers_to_the_LCD(1, first_number);
                }
            }
            if (entered_password_number == 1) {
                if (conversion_value > 99) {
                    if (second_number != 101) {
                        second_number = 101;
                        set_numbers_to_the_LCD(2, second_number);
                    }
                } else {
                    if (second_number != 100 - conversion_value) {
                        second_number = 100 - conversion_value;
                        set_numbers_to_the_LCD(2, second_number);
                    }

                }
            }
            if (entered_password_number == 2) {
                if (third_number != conversion_value) {
                    third_number = conversion_value;
                    set_numbers_to_the_LCD(3, third_number);
                }

            }
        }


        
        if (program_state == 2) {
            if (read_thermometer && is_thermometer_readed) {
                is_thermometer_readed = 0;
                if (celcius > 40) 
                    failed();
            }
            else {
                is_thermometer_readed = 1;
                if (entered_password_number == 0) {
                    if (conversion_value != first_number_guess) {
                        if (first_number_guess != -1 && is_change_in_password_check == 0) {
                            is_change_in_password_check = 5;
                            start_timer0_interrupt_for_500ms();
                        }
                        first_number_guess = conversion_value;
                        set_numbers_to_the_LCD(1, first_number_guess);
                        LCDGoto(4, 2);
                        LCDStr("__\0");
                        LCDGoto(7, 2);
                        LCDStr("__\0");
                    }
                }
                if (entered_password_number == 1) {
                    if (conversion_value > 99) {
                        if (second_number_guess != 101) {
                            second_number_guess = 101;
                            set_numbers_to_the_LCD(2, second_number_guess);
                            set_numbers_to_the_LCD(1, first_number_guess);
                            LCDGoto(7, 2);
                            LCDStr("__\0");
                        }
                    } else {
                        if (second_number_guess != 100 - conversion_value) {
                            second_number_guess = 100 - conversion_value;
                            set_numbers_to_the_LCD(2, second_number_guess);
                            set_numbers_to_the_LCD(1, first_number_guess);
                            LCDGoto(7, 2);
                            LCDStr("__\0");
                        }
                    }
                }
                if (entered_password_number == 2) {
                    if (conversion_value != third_number_guess) {
                        third_number_guess = conversion_value;
                        set_numbers_to_the_LCD(3, third_number_guess);
                        set_numbers_to_the_LCD(2, second_number_guess);
                        set_numbers_to_the_LCD(1, first_number_guess);
                    }
                }
            }
        }

        return;
    }


    // Timer-0 interrupt handler. It interrupts every 500ms. So, I am using this interrupt 
    // to decrease remaining seconds in password check state and blink PORTB if condition holds.
    if (INTCONbits.T0IF) {
        INTCONbits.T0IF = 0;
        if (program_state == 2) {
            TMR0H = 0x60;
            TMR0L = 0x60;
            counter_for_1sec++;
            if (flag_for_port_blinking) {
                if (PORTBbits.RB6)
                    PORTB = 0b00000000;
                else
                    PORTB = 0b11111111;
            }
            if (counter_for_1sec >= 2) {
                counter_for_1sec = 0;
                remaining_seconds--;
                if (remaining_seconds == 0)
                    failed();
            }
        }
        return;
    }


    // Timer-1 interrupt handler. I am using this interrupt to display values. I first call determine_display_values() to update display values.
    // Then, I load values to the proper display and increment display_number. So, in the next interrupt, I load value to the next display. 
    if (PIR1bits.TMR1IF) {
        PIR1bits.TMR1IF = 0;
        TMR1H = 0xD7;
        TMR1L = 0xD7;
        if (program_state == 2) {
            determine_display_values();
            if (display_number == 2) {
                PORTD = display2_value;
                PORTAbits.RA3 = 1;
                PORTAbits.RA4 = 0;
                PORTAbits.RA5 = 0;
            }
            if (display_number == 3) {
                PORTD = display3_value;
                PORTAbits.RA3 = 0;
                PORTAbits.RA4 = 1;
                PORTAbits.RA5 = 0;
            }
            if (display_number == 4) {
                PORTD = display4_value;
                PORTAbits.RA3 = 0;
                PORTAbits.RA4 = 0;
                PORTAbits.RA5 = 1;
            }
            display_number++;
            if (display_number >= 5) 
                display_number = 2;
        }
        return;
    }

    
    // RB port change interrupt handler. I use this interrupt to change state success to password check again.
    // I am basically reseting values which I used in password check state. 
    if (INTCONbits.RBIF) {
        INTCONbits.RBIF = 0;
        if (program_state == 3 && PORTBbits.RB4 == 0) {
            INTCONbits.RBIE = 0;
            program_state = 2;
            remaining_seconds = 90;
            remaining_attempts = 3;
            first_number_guess = -1;
            second_number_guess = -1;
            third_number_guess = -1;
            entered_password_number = 0;
            is_change_in_password_check = 0;
            read_thermometer = 0;
            is_thermometer_readed = 0;
            flag_for_port_blinking = 0;
            counter_for_1sec = 0;
            TRISB = 0b00000001;
            PORTB = 0b00000000;
            LCDGoto(1, 1);
            LCDStr("Input Password: \0");
            LCDGoto(1, 2);
            LCDStr("__-__-__        \0");
            start_timer1_interrupt();
        }
        return;
    }

    
}



/*
 * Here, I take which_number and number parameters. I am using which number parameter to go proper place.
 * For example, if which number is 3 I go to 7th position of second line and load number parameter to there.
 * 
 */
void set_numbers_to_the_LCD(int which_number,int number) {
    if (number == -1) {
        if (which_number == 1) {
            LCDGoto(1, 2);
            LCDStr("__\0");
            return;
        } 
        if (which_number == 2) {
            LCDGoto(4, 2);
            LCDStr("__\0");
            return;
        }         
        if (which_number == 3) {
            LCDGoto(7, 2);
            LCDStr("__\0");
            return;
        } 
    }     
    if (number > 99) {
        if (which_number == 1) {
            LCDGoto(1, 2);
            LCDStr("XX\0");
            return;
        } 
        if (which_number == 2) {
            LCDGoto(4, 2);
            LCDStr("XX\0");
            return;
        }         
        if (which_number == 3) {
            LCDGoto(7, 2);
            LCDStr("XX\0");
            return;
        } 
    }
    char number_as_char[10] = {0};
    char zero_as_char[10] = {0};
    sprintf(number_as_char, "%d", number);
    sprintf(zero_as_char, "%d", 0);
    if (number < 10) {
        if (which_number == 1) {
            LCDGoto(1, 2);
            LCDStr(zero_as_char);
            LCDGoto(2, 2);
            LCDStr(number_as_char); 
            return;
        } 
        if (which_number == 2) {
            LCDGoto(4, 2);
            LCDStr(zero_as_char);
            LCDGoto(5, 2);
            LCDStr(number_as_char); 
            return;
        }         
        if (which_number == 3) {
            LCDGoto(7, 2);
            LCDStr(zero_as_char);
            LCDGoto(8, 2);
            LCDStr(number_as_char); 
            return;
        }     
    }
    else {
        if (which_number == 1) {
            LCDGoto(1, 2);
            LCDStr(number_as_char); 
            return;
        } 
        if (which_number == 2) {
            LCDGoto(4, 2);
            LCDStr(number_as_char); 
            return;
        }         
        if (which_number == 3) {
            LCDGoto(7, 2);
            LCDStr(number_as_char); 
            return;
        }        
    }
    return;
}



/*
 * I am using this function to clear displays when we enter success or fail state.
 */
void clear_display() {
    PORTD = empty_for_display;
    PORTAbits.RA3 = 1;
    PORTAbits.RA4 = 0;
    PORTAbits.RA5 = 0;
    
    PORTAbits.RA3 = 0;
    PORTAbits.RA4 = 1;
    PORTAbits.RA5 = 0;
    
    PORTAbits.RA3 = 0;
    PORTAbits.RA4 = 0;
    PORTAbits.RA5 = 1;
    PORTAbits.RA5 = 0;
    return;
}



/*
 * I determine display values and update values.
 * I look remaining_attempt value to update display2. If it is 2, I load attempt2 encoding to the display2.
 * Then, I look remaning seconds value and load proper encodings to the display3 and display4.
 */
void determine_display_values() {
    int temp_for_display3,temp_for_display4;
    if (remaining_attempts == 3) {
        display2_value = attempts3_left;
    }
    if (remaining_attempts == 2) {
        display2_value = attempts2_left;
    }
    if (remaining_attempts == 1) {
        display2_value = attempts1_left;
    }
    display3_value = remaining_seconds / 10;
    display4_value = remaining_seconds % 10;
    
    if (display3_value == 9) {
        temp_for_display3 = number9_for_display;
    }
    if (display3_value == 8) {
        temp_for_display3 = number8_for_display;
    }
    if (display3_value == 7) {
        temp_for_display3 = number7_for_display;
    }
    if (display3_value == 6) {
        temp_for_display3 = number6_for_display;
    }
    if (display3_value == 5) {
        temp_for_display3 = number5_for_display;
    }
    if (display3_value == 4) {
        temp_for_display3 = number4_for_display;
    }
    if (display3_value == 3) {
        temp_for_display3 = number3_for_display;
    }
    if (display3_value == 2) {
        temp_for_display3 = number2_for_display;
    }
    if (display3_value == 1) {
        temp_for_display3 = number1_for_display;
    }
    if (display3_value == 0) {
        temp_for_display3 = number0_for_display;
    }
    
    if (display4_value == 9) {
        temp_for_display4 = number9_for_display;
    }
    if (display4_value == 8) {
        temp_for_display4 = number8_for_display;
    }
    if (display4_value == 7) {
        temp_for_display4 = number7_for_display;
    }
    if (display4_value == 6) {
        temp_for_display4 = number6_for_display;
    }
    if (display4_value == 5) {
        temp_for_display4 = number5_for_display;
    }
    if (display4_value == 4) {
        temp_for_display4 = number4_for_display;
    }
    if (display4_value == 3) {
        temp_for_display4 = number3_for_display;
    }
    if (display4_value == 2) {
        temp_for_display4 = number2_for_display;
    }
    if (display4_value == 1) {
        temp_for_display4 = number1_for_display;
    }
    if (display4_value == 0) {
        temp_for_display4 = number0_for_display;
    }
    
    display3_value = temp_for_display3;
    display4_value = temp_for_display4;  
    return;
}


/*
 * I start Timer-0 interrupt via this function. 
 * I configured 1:128 prescale and 16 bit mode. It starts from 0x6060.So, it does interrupt for every 500ms.
 * I use this timer to update remaining seconds and blink PORTB if condition holds.
 * I enable timer-0 interrupt and start timer-0. 
 */
void start_timer0_interrupt_for_500ms() {
    counter_for_1sec = 0;
    T0CON = 0b00000110;
    TMR0H = 0x60;
    TMR0L = 0x60;
    INTCONbits.TMR0IF = 0;
    INTCONbits.TMR0IE = 1;
    T0CONbits.TMR0ON = 1;
    return;
}


/*
 * It stops timer-0 via clearing enable, flag and on bits.
 */
void stop_timer0_interrupt() {
    counter_for_1sec = 0;
    INTCONbits.TMR0IE = 0;
    INTCONbits.TMR0IF = 0;
    T0CONbits.TMR0ON = 0;  
    T0CON = 0b00000110;
    TMR0H = 0x60;
    TMR0L = 0x60;  
    return;
}


/*
 * I start timer-1 interrupt via this function.
 * I configured 1:1 prescale. It does interrupt every 1ms. 
 * I use this timer for display. When it interrupts, I will load value to the next display.
 * So, I catch smooth view between displays via timer-1.
 */
void start_timer1_interrupt() {
    T1CON = 0b01001000;
    TMR1H = 0xD7;
    TMR1L = 0xD7;
    PIR1bits.TMR1IF = 0;
    PIE1bits.TMR1IE = 1;
    T1CONbits.TMR1ON = 1;
    return;
}


/*
 * I use this to stop timer-1 when we enter success state.
 */
void stop_timer1_interrupt() {
    PIE1bits.TMR1IE = 0;
    PIR1bits.TMR1IF = 0;
    T1CONbits.TMR1ON = 0;
    T1CON = 0b01001000;
    TMR1H = 0xD7;
    TMR1L = 0xD7;    
    return;
}


/*
 * I call this function when user fail with any reason.
 * I first disable interrupts. Change program state to 4 (which is fail state) .
 * Then I write message to LCD, clear displays, turn off heater and turn on PORTB. 
   Failed function never returns.
 */
void failed() {
    INTCONbits.GIE = 0;
    INTCONbits.PEIE = 0;

    program_state = 4;
    LCDGoto(1, 1);
    LCDStr("You Failed!     ");
    LCDGoto(1, 2);
    LCDStr("             ");

    clear_display();

    PORTCbits.RC5 = 0;

    TRISB  = 0; 
    PORTB = 0xFF;
    
    while (1) { }
    return;
}


/*
 * I call this function when user enter success state. 
 * I disable timer interrupts, change program state to 3 (which is success state) .
 * Then I write message to LCD, clear displays, turn off heater and PORTB.
 * Finally, I enable port change interrupt to catch RB4 press.
 * 
 */
void successed() {
    stop_timer0_interrupt();
    stop_timer1_interrupt();
    flag_for_port_blinking = 0;
    program_state = 3;
    LCDGoto(1, 1);
    LCDStr("Unlocked; Press ");
    LCDGoto(1, 2);
    LCDStr("RB4 to lock!    ");

    clear_display();

    PORTCbits.RC5 = 0;
    
    TRISB = 0b00010001;
    PORTB = 0b00010000;
    
    INTCONbits.RBIF = 0;
    INTCONbits.RBIE = 1;
    
    return;
}



/*
 * I have four state which are 1 => Password Set, 2 => Password Check, 3 => Success, 4 => Fail.
 * Overall, I read analog values via A/D interrupt. This interrupt updates values, and RB0 interrupt do necessary actions with these values.
 * In password set state, I simply read values from zeroth channel continously and A/D interrupt updates first_number,second_number,third_number variables and also updates LCD.
 * With external interrupt, I simply increment entered_password_number variable and if it reaches 3 I change program state to 2 which is password check state.
 * In password check state, logic is same. A/D interrupt updates first_number_guess,second_number_guess,third_number_guess variables and also updates LCD accordingly.
 * If we should read thermometer, I also read thermometer in this state with some flags to provide synchronization between potentiometer and thermometer values.
 * When external interrupt happens, it compares guess values with actual values and do necessary actions according to remanining attempt value.
 * In this state, I use 500ms timer-0 interrupt for countdown and PORTB blinking and 1ms timer-1 interrupt for switching between displays.
 * determine_display_values function look remaning seconds and attempts and updates display values accordingly.
 * set_numbers_to_the_LCD function set password numbers to the proper place of the second line according to its arguments.
 * In success state, I do necessary actions and activate port change interrupt to be able to return back.
 * In fail state, I do necessary actions and never return.
 * I commented for each global variables, interrupts and helper functions. For more detailed explanation, you can read them.  
 */
void main(void) {
    // Initialization Part
    TRISAbits.RA3 = 0;
    TRISAbits.RA4 = 0;
    TRISAbits.RA5 = 0;
    TRISD = 0;

    TRISCbits.RC5 = 0; // Heater configuration.

    initADC();
    InitLCD();

    TRISB = 0b00000001;
    PORTB = 0b00000000;

    LCDGoto(1, 1);
    LCDStr("SuperSecureSafe!\0");
    __delay_ms(3000);
    LCDGoto(1, 1);
    LCDStr("Set Password:   \0");
    LCDGoto(1, 2);
    LCDStr("__\0");
    LCDGoto(3, 2);
    LCDStr("-\0");
    LCDGoto(4, 2);
    LCDStr("__\0");
    LCDGoto(6, 2);
    LCDStr("-\0");
    LCDGoto(7, 2);
    LCDStr("__        \0");

    TRISBbits.RB0 = 1; // 
    INTCONbits.INT0IF = 0; // INT-0 external interrupt configuration.
    INTCONbits.INT0IE = 1; //


    PIR1bits.ADIF = 0; // A/D interrupt configuration.
    PIE1bits.ADIE = 1; // 



    RCONbits.IPEN = 0; // Disable priorities.
    INTCONbits.GIE = 1; // Enable global interrupt pin.
    INTCONbits.PEIE = 1; // Enable peripheral interrupts.



    while (1) {
        if (program_state == 1)
            readADCChannel(0);
        if (program_state == 2) {
            if (read_thermometer) {
                if (is_thermometer_readed) {
                    readADCChannel(2);
                    is_thermometer_readed = 1;
                }
                else {
                    readADCChannel(0);
                    is_thermometer_readed = 0;              
                }
            }
            else 
               readADCChannel(0);
        }
    }

    
    return;
}