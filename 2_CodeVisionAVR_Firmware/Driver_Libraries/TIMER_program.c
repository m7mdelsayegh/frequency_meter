/*
 * TIMER_program.c - Timer0 / Timer1 driver (CodeVisionAVR compatible)
 *
 *  Created on: Oct 6, 2023
 *      Author: 20128
 *
 *  NOTE: This file was restored from the backup "TIMER_program.c~".
 *        The original "TIMER_program.c" had been overwritten by an old
 *        main() program and its comments were destroyed by an encoding
 *        error, so it could not be compiled as a driver.
 */
#define TIM0_OVF 12   /* CodeVisionAVR interrupt vector number: TIMER0 OVF (ATmega32) */
#define TIM1_OVF 10   /* CodeVisionAVR interrupt vector number: TIMER1 OVF (ATmega32) */

#include "STD_TYPES.h"
#include "BIT_MATH.h"
#include "TIMER_register.h"
#include "TIMER_config.h"
#include "TIMER_interface.h"

void (* TIMER0_OVF_CALLBACK_PTR) (void) = NULL;

void TIMER0_Init(void) {
    Clr_Bit(TCCR0_register, WGM00);
    Clr_Bit(TCCR0_register, WGM01);

    Clr_Bit(TCCR0_register, COM00);
    Clr_Bit(TCCR0_register, COM01);

    /* Prescaler clk/1024 */
    Set_Bit(TCCR0_register, CS00);
    Clr_Bit(TCCR0_register, CS01);
    Set_Bit(TCCR0_register, CS02);
}

void TIMER0_OVF_Interrupt_Enable(void) {
    Set_Bit(TIMSK_register, TOIE0);
}

void TIMER0_OVF_Interrupt_Disable(void) {
    Clr_Bit(TIMSK_register, TOIE0);
}

void TIMER0_Set_TCNT0_Register(u8 Value) {
    TCNT0_register = Value;
}

void TIMER0_OVF_Set_Callback_Address(void (*PTR) (void)) {
    TIMER0_OVF_CALLBACK_PTR = PTR;
}

/* Timer0 overflow ISR - CodeVisionAVR syntax */
interrupt [TIM0_OVF] void timer0_ovf_isr(void) {
    if(TIMER0_OVF_CALLBACK_PTR != NULL) {
        TIMER0_OVF_CALLBACK_PTR();
    }
}

// TIMER1
void (* TIMER1_CTC_A_CALLBACK_PTR) (void) = NULL;
void (* TIMER1_CTC_B_CALLBACK_PTR) (void) = NULL;
void (* TIMER1_OVF_CALLBACK_PTR) (void) = NULL;
void (* TIMER1_ICI_CALLBACK_PTR) (void) = NULL;

void TIMER1_Init(void) {
    Clr_Bit(TCCR1B_register, WGM12);
    Clr_Bit(TCCR1B_register, WGM13);
    Clr_Bit(TCCR1A_register, WGM10);
    Clr_Bit(TCCR1A_register, WGM11);

    Clr_Bit(TCCR1A_register, COM1A0);
    Clr_Bit(TCCR1A_register, COM1A1);
    Clr_Bit(TCCR1A_register, COM1B0);
    Clr_Bit(TCCR1A_register, COM1B1);

    /* External clock on T1 pin, rising edge */
    Set_Bit(TCCR1B_register, CS10);
    Set_Bit(TCCR1B_register, CS11);
    Set_Bit(TCCR1B_register, CS12);
}

void TIMER1_OVF_Interrupt_Enable(void) {
    Set_Bit(TIMSK_register, TOIE1);
}

void TIMER1_OVF_Interrupt_Disable(void) {
    Clr_Bit(TIMSK_register, TOIE1);
}

void TIMER1_OVF_Set_Callback_Address(void (*PTR) (void)) {
    TIMER1_OVF_CALLBACK_PTR = PTR;
}

void TIMER1_Set_TCNT1_Register(u16 Value) {
    /* 16-bit write: the HIGH byte must be written first, then the LOW byte (ATmega32 datasheet) */
    *((volatile u8*)(0x4D)) = (u8)(Value >> 8);   // TCNT1H
    *((volatile u8*)(0x4C)) = (u8)(Value & 0xFF); // TCNT1L
}

u16 TIMER1_Get_TCNT1_Register(void) {
    /* 16-bit read: the LOW byte must be read first, then the HIGH byte */
    u8 low  = *((volatile u8*)(0x4C)); // TCNT1L
    u8 high = *((volatile u8*)(0x4D)); // TCNT1H
    return ((u16)high << 8) | low;
}

interrupt [TIM1_OVF] void timer1_ovf_isr(void) {
    if(TIMER1_OVF_CALLBACK_PTR != NULL) {
        TIMER1_OVF_CALLBACK_PTR();
    }
}
