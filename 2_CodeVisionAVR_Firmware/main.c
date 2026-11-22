/*
 * 40 MHz Dual-Range Auto-Switching Frequency Meter + UART Telemetry
 * Target: ATmega32A @ 11.0592 MHz Crystal
 * Prescaler IC: 74HC191 (Divide-by-16 on Q3 -> PB1/T1)
 * UART Output: PD1 (TXD, Pin 15) @ 9600 Baud, 8N1 -> CP2102 -> Android OTG App
 * Compiler: CodeVisionAVR
 */

#include <delay.h>

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned long int u32;

#define Set_Bit(Var,BitNo) Var=Var|(1<<BitNo)
#define Clr_Bit(Var,BitNo) Var=Var&(~(1<<BitNo))
#define Get_Bit(Var,BitNo) ((Var>>BitNo)&1)
#define Tog_Bit(Var,BitNo) Var=Var^(1<<BitNo)

/* Memory Mapped Registers for ATmega32 */
#define SREG_register   *((volatile u8*)(0x5F))

/* Timer 0 (8-bit Direct Counter on PB0/T0) */
#define TCCR0_register  *((volatile u8*)(0x53))
#define TCNT0_register  *((volatile u8*)(0x52))

/* Timer 1 (16-bit Prescaled Counter on PB1/T1) */
#define TCCR1A_register *((volatile u8*)(0x4F))
#define TCCR1B_register *((volatile u8*)(0x4E))
#define TCNT1L_register *((volatile u8*)(0x4C))
#define TCNT1H_register *((volatile u8*)(0x4D))

/* Timer 2 (8-bit Precision 1.000s Gate Generator) */
#define TCCR2_register  *((volatile u8*)(0x45))
#define TCNT2_register  *((volatile u8*)(0x44))

#define TIMSK_register  *((volatile u8*)(0x59))
#define TIFR_register   *((volatile u8*)(0x58))

/* USART Registers */
#define UDR_register    *((volatile u8*)(0x2C))
#define UCSRA_register  *((volatile u8*)(0x2B))
#define UCSRB_register  *((volatile u8*)(0x2A))
#define UCSRC_register  *((volatile u8*)(0x40))
#define UBRRH_register  *((volatile u8*)(0x40))
#define UBRRL_register  *((volatile u8*)(0x29))

/* GPIO Registers */
#define PORTA_register  *((volatile u8*)(0x3B))
#define DDRA_register   *((volatile u8*)(0x3A))
#define PORTB_register  *((volatile u8*)(0x38))
#define DDRB_register   *((volatile u8*)(0x37))
#define PORTD_register  *((volatile u8*)(0x32))
#define DDRD_register   *((volatile u8*)(0x31))
#define PIND_register   *((volatile u8*)(0x30))

/* Overflow Counters */
volatile u32 Ovf0 = 0; // Direct input overflows
volatile u32 Ovf1 = 0; // Divided (/16) input overflows
volatile u32 Ovf2 = 0; // 1.000s Timebase overflows

/* CodeVisionAVR ATmega32 Interrupt Vector Numbers */
interrupt [6]  void timer2_ovf_isr(void) { Ovf2++; } // TIMER2 OVF
interrupt [10] void timer1_ovf_isr(void) { Ovf1++; } // TIMER1 OVF
interrupt [12] void timer0_ovf_isr(void) { Ovf0++; } // TIMER0 OVF

/* ---------------- UART Driver (9600 8N1 @ 11.0592 MHz) ---------------- */
void UART_Init(void) {
    UBRRH_register = 0x00;
    UBRRL_register = 71;   // 11059200 / (16 * 9600) - 1 = 71 (0% error)
    UCSRA_register = 0x00; // Normal speed (U2X = 0)
    UCSRB_register = 0x08; // Enable Transmitter ONLY (TXEN = 1, RXEN = 0 so PD0 stays GPIO)
    UCSRC_register = 0x86; // URSEL = 1, 8 Data Bits, 1 Stop Bit, No Parity
}

void UART_TxChar(char ch) {
    while (Get_Bit(UCSRA_register, 5) == 0); // Wait for UDRE (Bit 5) to be empty
    UDR_register = ch;
}

void UART_Send_Freq(u32 val) {
    char buf[12];
    u8 i = 0;

    if (val == 0) {
        UART_TxChar('0');
    } else {
        while (val > 0) {
            buf[i++] = (val % 10) + '0';
            val /= 10;
        }
        while (i > 0) {
            UART_TxChar(buf[--i]);
        }
    }
    UART_TxChar('\r'); // CR
    UART_TxChar('\n'); // LF (#10 expected by Delphi HandleIncoming)
}

/* ---------------- 4-bit LCD Driver ---------------- */
void LCD_Pulse(void) {
    Set_Bit(PORTB_register, 4); // E = High
    delay_ms(2);
    Clr_Bit(PORTB_register, 4); // E = Low
    delay_ms(2);
}

void LCD_Write_Command(u8 Command) {
    Clr_Bit(PORTB_register, 2); // RS = 0
    Clr_Bit(PORTB_register, 3); // RW = 0

    PORTA_register = (PORTA_register & 0x0F) | (Command & 0xF0);
    LCD_Pulse();

    PORTA_register = (PORTA_register & 0x0F) | ((Command << 4) & 0xF0);
    LCD_Pulse();
}

void LCD_Write_Data(u8 Data) {
    Set_Bit(PORTB_register, 2); // RS = 1
    Clr_Bit(PORTB_register, 3); // RW = 0

    PORTA_register = (PORTA_register & 0x0F) | (Data & 0xF0);
    LCD_Pulse();

    PORTA_register = (PORTA_register & 0x0F) | ((Data << 4) & 0xF0);
    LCD_Pulse();
}

void LCD_Init(void) {
    DDRA_register |= 0xF0;                            // PA4-PA7 Output
    DDRB_register |= (1 << 2) | (1 << 3) | (1 << 4);  // PB2, PB3, PB4 Output
    delay_ms(40);

    LCD_Write_Command(0x02);
    LCD_Write_Command(0x28);
    delay_ms(2);
    LCD_Write_Command(0x0C);
    delay_ms(2);
    LCD_Write_Command(0x01);
    delay_ms(3);
    LCD_Write_Command(0x06);
    delay_ms(2);
}

void LCD_Go_To(u8 y, u8 x) {
    LCD_Write_Command(128 + (y * 0x40) + x);
}

void LCD_Write_String(char *Str, u8 Length) {
    u8 i = 0;
    for(i = 0; i < Length; i++) {
        LCD_Write_Data(Str[i]);
    }
}

void display(u32 freq_val) {
    LCD_Go_To(1, 0);
    LCD_Write_Data(((freq_val % 100000000) / 10000000) + '0');
    LCD_Write_Data(((freq_val % 10000000) / 1000000) + '0');
    LCD_Write_Data(((freq_val % 1000000) / 100000) + '0');
    LCD_Write_Data(((freq_val % 100000) / 10000) + '0');
    LCD_Write_Data(((freq_val % 10000) / 1000) + '0');
    LCD_Write_Data(((freq_val % 1000) / 100) + '0');
    LCD_Write_Data(((freq_val % 100) / 10) + '0');
    LCD_Write_Data(((freq_val % 10) / 1) + '0');
    LCD_Write_String(" Hz       ", 9);
}

void MHz(u32 freq_val) {
    LCD_Go_To(0, 0);
    LCD_Write_String("Freq= ", 6);
    LCD_Write_Data(((freq_val % 100000000) / 10000000) + '0');
    LCD_Write_Data(((freq_val % 10000000) / 1000000) + '0');
    LCD_Write_Data('.');
    LCD_Write_Data(((freq_val % 1000000) / 100000) + '0');
    LCD_Write_Data(((freq_val % 100000) / 10000) + '0');
    LCD_Write_Data(((freq_val % 10000) / 1000) + '0');
    LCD_Write_String(" MHz  ", 6);
}

void KHz(u32 freq_val) {
    LCD_Go_To(0, 0);
    LCD_Write_String("Freq= ", 6);
    LCD_Write_Data(((freq_val % 1000000) / 100000) + '0');
    LCD_Write_Data(((freq_val % 100000) / 10000) + '0');
    LCD_Write_Data(((freq_val % 10000) / 1000) + '0');
    LCD_Write_Data('.');
    LCD_Write_Data(((freq_val % 1000) / 100) + '0');
    LCD_Write_Data(((freq_val % 100) / 10) + '0');
    LCD_Write_Data(((freq_val % 10) / 1) + '0');
    LCD_Write_String(" KHz  ", 6);
}

void Hz(u32 freq_val) {
    LCD_Go_To(0, 0);
    LCD_Write_String("Freq= ", 6);
    LCD_Write_Data(((freq_val % 1000) / 100) + '0');
    LCD_Write_Data(((freq_val % 100) / 10) + '0');
    LCD_Write_Data(((freq_val % 10) / 1) + '0');
    LCD_Write_String(" Hz       ", 9);
}

void main(void) {
    u8 low_b = 0;
    u8 high_b = 0;
    u16 tcnt1_val = 0;
    u8 tcnt0_val = 0;

    u32 freq_direct = 0;  // Fo (Direct count on T0)
    u32 freq_divided = 0; // Fd * 16 (Prescaled count on T1)
    u32 final_freq = 0;

    LCD_Init();
    LCD_Write_Command(0x01);
    UART_Init(); // Initialize 9600 8N1 UART on PD1 (TXD)

    /* Configure GPIO */
    Clr_Bit(DDRD_register, 0);   // PD0 Input (Mode button)
    Set_Bit(PORTD_register, 0);  // Pull-up on PD0
    Set_Bit(DDRD_register, 7);   // PD7 Output (Heartbeat LED)
    Clr_Bit(PORTD_register, 7);

    Clr_Bit(DDRB_register, 0);   // PB0 / T0 Input (Direct signal)
    Clr_Bit(DDRB_register, 1);   // PB1 / T1 Input (74HC191 Q3 / 16 signal)

    Set_Bit(SREG_register, 7);   // Global Interrupt Enable

    LCD_Write_String("40MHz Counter", 13);
    delay_ms(500);
    LCD_Write_Command(0x01);

    while(1) {
        /* 1. Stop all timers before resetting */
        TCCR0_register  = 0x00;
        TCCR1B_register = 0x00;
        TCCR2_register  = 0x00;

        /* 2. Reset counters and preload Timer2 for 1.000s @ 11.0592 MHz */
        Ovf0 = 0;
        Ovf1 = 0;
        Ovf2 = 0;

        TCNT0_register  = 0;
        TCNT1H_register = 0;
        TCNT1L_register = 0;
        TCNT2_register  = 208; // 43 overflows * 256 - 208 = 10,800 ticks = 1.000s

        /* 3. Clear pending interrupt flags (TOV0, TOV1, TOV2) */
        TIFR_register = (1 << 0) | (1 << 2) | (1 << 6);

        /* 4. Enable overflow interrupts for Timer0, Timer1, and Timer2 */
        TIMSK_register |= (1 << 0) | (1 << 2) | (1 << 6);

        /* 5. Start all three timers simultaneously */
        TCCR2_register  = 0x07; // Timer2 Prescaler = 1024
        TCCR0_register  = 0x07; // Timer0 External clock on T0, rising edge
        TCCR1B_register = 0x07; // Timer1 External clock on T1, rising edge

        /* 6. Wait for exact 1.000s gate window */
        while(Ovf2 < 43) {
            #asm("nop")
        }

        /* 7. Halt all counters immediately */
        TCCR0_register  = 0x00;
        TCCR1B_register = 0x00;
        TCCR2_register  = 0x00;
        TIMSK_register &= ~((1 << 0) | (1 << 2) | (1 << 6));

        /* 8. Read hardware counter registers */
        tcnt0_val = TCNT0_register;
        low_b     = TCNT1L_register;
        high_b    = TCNT1H_register;
        tcnt1_val = ((u16)high_b << 8) | low_b;

        /* 9. Calculate both direct and prescaled frequencies */
        freq_direct  = (Ovf0 * 256UL) + tcnt0_val;
        freq_divided = ((Ovf1 * 65536UL) + tcnt1_val) * 16UL;

        /* 10. Auto-range selection */
        if (freq_divided > 3500000UL || (freq_divided > freq_direct + 1000UL)) {
            final_freq = freq_divided;
        } else {
            final_freq = freq_direct;
        }

        /* 11. Display formatted reading on Line 1 */
        if(final_freq >= 1000000UL) {
            MHz(final_freq);
        }
        else if(final_freq >= 1000UL) {
            KHz(final_freq);
        }
        else {
            Hz(final_freq);
        }

        /* 12. Check PD0 button for raw Hz on Line 2 */
        if(Get_Bit(PIND_register, 0) == 0) {
            display(final_freq);
        } else {
            LCD_Go_To(1, 0);
            LCD_Write_String("                ", 16);
        }

        /* 13. Transmit raw Hz integer over UART to Android App */
        UART_Send_Freq(final_freq);

        /* 14. Toggle Heartbeat LED on PD7 */
        Tog_Bit(PORTD_register, 7);
    }
}