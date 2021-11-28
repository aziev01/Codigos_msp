//#include <msp430.h>

#include "msp430f6747a.h"

void inicializacion_osciladores();
void encender_apagar_rele(int);
void enviar_falla(int, int);

unsigned int i; //Counter




// REGISTRO DE COMUNICACION
int msg_rcv = 0;

//VARIABLES MAIN LOOP
int reg_corr[24];
int reg_rele[24];
int reg_falla[24];
int reg_falla_ant[27];
int contador[24]; // FILTRO PARA DETECTAR FALLA
int g1 = 0;
int h1 = 0;
int h2 = 0;
int j3 = 0;
int j5 = 0;

//VARIABLES SUPERVISION ALIMENTACION DE FUENTES
int fuente_aux = 0;
int fuente_main = 0;

int fuente_aux_ant = 0;
int fuente_main_ant = 0;

// VARIABLES ENCENDER-APAGAR INTERFAZ
int apagar_stat = 0;

int main(void)
{
    WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

    //Configuracion Oscilador interno
    inicializacion_osciladores();

    //PUERTOS DETECTORES DE FALLA
    P5DIR &= 0x00; // Puerto 5 como entrada (SensorC 1-8)
    P4DIR &= 0x00; // Puerto 4 como entrada (SensorC 9-16)
    P6DIR &= 0x00; // Puerto 6 como entrada (SensorC 17-24)

    //PUERTO DETECCION DE FUENTE
    P1DIR &= ~BIT0; //AUX
    P1DIR &= ~BIT0; //MAIN
    //P1DIR &= ~BIT7; SOLO PARA PCB #4 Stage Control
    //P3DIR &= ~BIT5; SOLO PARA PCB #4 Stage Control

    //RESISTENCIAS PULL UP PARA ENTRADAS
    P5REN |= 0xff;
    P5OUT |= 0xff;
    P4REN |= 0xff;
    P4OUT |= 0xff;
    P6REN |= 0xff;
    P6OUT |= 0xff;
    //P1REN |= BIT7; SOLO PARA PCB #4 Stage Control
    //P1OUT |= BIT7; SOLO PARA PCB #4 Stage Control
    //P3REN |= BIT5; SOLO PARA PCB #4 Stage Control
    //P3OUT |= BIT5; SOLO PARA PCB #4 Stage Control

    //PUERTOS SALIDA RELES
    P8DIR |= 0xff; // Puerto 8 como salida (Rele 1-8)
    P9DIR |= 0xff; // Puerto 9 como salida (Rele 9-16)
    P10DIR |= 0xff; // Puerto 10 como salida (Rele 17-24)

    //APAGAR RELES
    P8OUT &= 0x00;
    P9OUT &= 0x00;
    P10OUT &= 0x00;

    // INICIALIZAR UART
    P3SEL0 |= BIT0 + BIT1;                       // Seleccionar funcion UART pines P3.(0,1)
    UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA0CTL1 |= UCSSEL_2;                     // SMCLK
    UCA0BR0 = 6;                              // 12MHz 115200 (see User's Guide)
    UCA0BR1 = 0;                              //   12MHz 115200
    UCA0MCTLW |= UCBRF_8 + UCOS16;            // Modulation UCBRSx=1, UCBRFx=0
    //UCA0MCTLW |= UCBRF_8 + UCOS16;
    UCA0CTL1 &= ~UCSWRST;                   // **Initialize USCI state machine**
    UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
    //

    // INICIALIZAR TIMER
    TA1CCTL0 |= CCIE;                          // CCR0 interrupt enabled
    TA1CTL = TASSEL_1 + MC_1 + ID_3 + TACLR;       // SMCLK, contmode, clear TAR
    TA1EX0 = TAIDEX_7;
    TA1CCR0 = 51200;

    volatile unsigned int i;        // volatile to prevent optimization
    __bis_SR_register(GIE); // Habilita interrupciones

    _delay_cycles(30000);
    while (1)
    {

        // DETECTAR CORRIENTE
        // Se realiza comparacion entre el puerto de entrada y la salida del comparador
        // para detectar el consumo de corriente del rele
        reg_corr[0] = (P5IN & BIT0); //Rele Corriente 1
        reg_corr[1] = (P5IN & BIT1); //Rele Corriente 2
        reg_corr[2] = (P5IN & BIT2); //reg_corr[2] = (P3IN & BIT5); SOLO PARA PCB #4 Stage Control. Rele Corriente 3
        reg_corr[3] = (P5IN & BIT3); //reg_corr[3] = (P1IN & BIT7); SOLO PARA PCB #4 Stage Control. Rele Corriente 4
        reg_corr[4] = (P5IN & BIT4); //Rele Corriente 5
        reg_corr[5] = (P5IN & BIT5); //Rele Corriente 6
        reg_corr[6] = (P5IN & BIT6); //Rele Corriente 7
        reg_corr[7] = (P5IN & BIT7); //Rele Corriente 8
        reg_corr[8] = (P6IN & BIT0); //Rele Corriente 9
        reg_corr[9] = (P4IN & BIT0); //Rele Corriente 10
        reg_corr[10] = (P4IN & BIT1); //Rele Corriente 11
        reg_corr[11] = (P4IN & BIT2); //Rele Corriente 12
        reg_corr[12] = (P4IN & BIT3); //Rele Corriente 13
        reg_corr[13] = (P4IN & BIT4); //Rele Corriente 14
        reg_corr[14] = (P4IN & BIT5); //Rele Corriente 15
        reg_corr[15] = (P4IN & BIT6); //Rele Corriente 16
        reg_corr[16] = (P4IN & BIT7); //Rele Corriente 17
        reg_corr[17] = (P6IN & BIT1); //Rele Corriente 18
        reg_corr[18] = (P6IN & BIT2); //Rele Corriente 19
        reg_corr[19] = (P6IN & BIT3); //Rele Corriente 20
        reg_corr[20] = (P6IN & BIT4); //Rele Corriente 21
        reg_corr[21] = (P6IN & BIT5); //Rele Corriente 22
        reg_corr[22] = (P6IN & BIT6); //Rele Corriente 23
        reg_corr[23] = (P6IN & BIT7); //Rele Corriente 24


        //DETECTAR FALLA

        for (h1 = 0; h1 < 24; h1++)
        {
            if ((reg_rele[h1] > 0) && (reg_corr[h1] == 0))
            {
                reg_falla[h1] = 1;
                contador[h1] = 0;
            }

            if ((reg_rele[h1] > 0) && (reg_corr[h1] > 0))
            {
                contador[h1] += 1;
                if (contador[h1] == 150)
                {
                    reg_falla[h1] = 0;
                }
            }

            if (reg_rele[h1] == 0)
            {
                reg_falla[h1] = 0; //Apaga la falla cada vez que el btn se apaga
            }

        }

        // ENVIAR MSG DE FALLA
        for (h2 = 0; h2 < 24; h2++)
        {
            if ((reg_falla[h2] == 1) && (reg_falla_ant[h2] == 0))
            {
                enviar_falla(h2, 1);
                enviar_falla(h2, 1);
                _delay_cycles(30000);
            }
            else if ((reg_falla[h2] == 0) && (reg_falla_ant[h2] == 1))
            {
                enviar_falla(h2, 0);
                enviar_falla(h2, 0);
                _delay_cycles(30000);
            }
            reg_falla_ant[h2] = reg_falla[h2]; //Actualizar el vector de fallas;
        }

    }
}

//INTERRUPCION TIMER 1 A0
#pragma vector=TIMER1_A0_VECTOR
__interrupt
void TIMER1_A0_ISR(void)
{
    fuente_main = (P1IN & BIT0);
    fuente_aux = (P1IN & BIT1);

    if (fuente_main > 0 && fuente_main_ant == 0)
    {
        UCA0TXBUF = 0x70;
        UCA0IE |= UCTXIE;

    }
    else if (fuente_main == 0 && fuente_main_ant > 0)
    {
        UCA0TXBUF = 0x60;
        UCA0IE |= UCTXIE;
    }

    if (fuente_aux > 0 && fuente_aux_ant == 0)
    {
        UCA0TXBUF = 0x71;
        UCA0IE |= UCTXIE;

    }
    else if (fuente_aux == 0 && fuente_aux_ant > 0)
    {
        UCA0TXBUF = 0x61;
        UCA0IE |= UCTXIE;
    }

    fuente_main_ant = fuente_main;
    fuente_aux_ant = fuente_aux;

}

#pragma vector=USCI_A0_VECTOR
__interrupt
void USCI0TX_ISR(void)
{
    switch (__even_in_range(UCA0IV, 4))
    {
    case 0:
        break;                             // Vector 0 - no interrupt
    case 2:
        //while (!(UCA0IFG & UCTXIFG));
        while (!(UCTXIFG))
            ;                             // USCI_A0 TX buffer ready?
        msg_rcv = UCA0RXBUF;              // Se almacena la palabra recibida
        encender_apagar_rele(msg_rcv);
        break;
    case 4:
        // UCA0TXBUF = 0x41; // TX next character
        UCA0IE &= ~UCTXIE; // Disable USCI_A0 TX interrupt
        break;
    default:
        break;
    }
}

void inicializacion_osciladores()
{
    UCSCTL3 |= SELREF_2;                     // Set DCO FLL reference = REFO
    UCSCTL4 |= SELA_2;                        // Set ACLK = REFO

    __bis_SR_register(SCG0);                 // Disable the FLL control loop
    UCSCTL0 = 0x0000;                      // Set lowest possible DCOx, MODx
    UCSCTL1 = DCORSEL_7;                 // Select DCO range 24MHz operation
    UCSCTL2 = FLLD_1 + 374;                  // Set DCO Multiplier for 12MHz
    // (N + 1) * FLLRef = Fdco
    // (374 + 1) * 32768 = 12MHz
    // Set FLL Div = fDCOCLK/2
    __bic_SR_register(SCG0);                  // Enable the FLL control loop

    // Worst-case settling time for the DCO when the DCO range bits have been
    // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
    // UG for optimization.
    // 32 x 32 x 12 MHz / 32,768 Hz = 375000 = MCLK cycles for DCO to settle
    __delay_cycles(375000);

    // Loop until XT1,XT2 & DCO fault flag is cleared
    do
    {
        UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
        // Clear XT2,XT1,DCO fault flags
        SFRIFG1 &= ~OFIFG;                      // Clear fault flags
    }
    while (SFRIFG1 & OFIFG);                   // Test oscillator fault flag

    UCSCTL4 |= SELA_3 + SELS_3 + SELM_3;                  // Set ACLK = REFO
    UCSCTL5 |= DIVA_0 + DIVS_0 + DIVM_0;

}

void encender_apagar_rele(int c)
{
    switch (c)
    {

    //APAGAR RELE
    case 0x00:
        P8OUT &= ~BIT0;
        reg_rele[0] = 0;
        break;

    case 0x01:
        P8OUT &= ~BIT1;
        reg_rele[1] = 0;
        break;

    case 0x02:
        P8OUT &= ~BIT2;
        reg_rele[2] = 0;
        break;

    case 0x03:
        P8OUT &= ~BIT3;
        reg_rele[3] = 0;
        break;

    case 0x04:
        P8OUT &= ~BIT4;
        reg_rele[4] = 0;
        break;

    case 0x05:
        P8OUT &= ~BIT5;
        reg_rele[5] = 0;
        break;

    case 0x06:
        P8OUT &= ~BIT6;
        reg_rele[6] = 0;
        break;

    case 0x07:
        P8OUT &= ~BIT7;
        reg_rele[7] = 0;
        break;

    case 0x08:
        P9OUT &= ~BIT0;
        reg_rele[8] = 0;
        break;

    case 0x09:
        P9OUT &= ~BIT3;
        reg_rele[9] = 0;
        break;

    case 0x0A:
        P9OUT &= ~BIT1;
        reg_rele[10] = 0;
        break;

    case 0x0B:
        P9OUT &= ~BIT2;
        reg_rele[11] = 0;
        break;

    case 0x0C:
        P9OUT &= ~BIT5;
        reg_rele[12] = 0;
        break;

    case 0x0D:
        P9OUT &= ~BIT6;
        reg_rele[13] = 0;
        break;

    case 0x0E:
        P9OUT &= ~BIT4;
        reg_rele[14] = 0;
        break;

    case 0x0F:
        P9OUT &= ~BIT7;
        reg_rele[15] = 0;
        break;

    case 0x10:
        P10OUT &= ~BIT0;
        reg_rele[16] = 0;
        break;

    case 0x11:
        P10OUT &= ~BIT1;
        reg_rele[17] = 0;
        break;

    case 0x12:
        P10OUT &= ~BIT2;
        reg_rele[18] = 0;
        break;

    case 0x13:
        P10OUT &= ~BIT3;
        reg_rele[19] = 0;
        break;

    case 0x14:
        P10OUT &= ~BIT4;
        reg_rele[20] = 0;
        break;

    case 0x15:
        P10OUT &= ~BIT5;
        reg_rele[21] = 0;
        break;

    case 0x16:
        P10OUT &= ~BIT6;
        reg_rele[22] = 0;
        break;

    case 0x17:
        P10OUT &= ~BIT7;
        reg_rele[23] = 0;
        break;

        // ENCENDER RELE
    case 0x20:
        P8OUT |= BIT0;
        reg_rele[0] = 1;
        break;

    case 0x21:
        P8OUT |= BIT1;
        reg_rele[1] = 1;
        break;

    case 0x22:
        P8OUT |= BIT2;
        reg_rele[2] = 1;
        break;

    case 0x23:
        P8OUT |= BIT3;
        reg_rele[3] = 1;
        break;

    case 0x24:
        P8OUT |= BIT4;
        reg_rele[4] = 1;
        break;

    case 0x25:
        P8OUT |= BIT5;
        reg_rele[5] = 1;
        break;

    case 0x26:
        P8OUT |= BIT6;
        reg_rele[6] = 1;
        break;

    case 0x27:
        P8OUT |= BIT7;
        reg_rele[7] = 1;
        break;

    case 0x28:
        P9OUT |= BIT0;
        reg_rele[8] = 1;
        break;

        //////
    case 0x29:
        P9OUT |= BIT3;
        reg_rele[9] = 1;
        break;

    case 0x2A:
        P9OUT |= BIT1;
        reg_rele[10] = 1;
        break;

    case 0x2B:
        P9OUT |= BIT2;
        reg_rele[11] = 1;
        break;

    case 0x2C:
        P9OUT |= BIT5;
        reg_rele[12] = 1;
        break;

    case 0x2D:
        P9OUT |= BIT6;
        reg_rele[13] = 1;
        break;

    case 0x2E:
        P9OUT |= BIT4;
        reg_rele[14] = 1;
        break;

        ////

    case 0x2F:
        P9OUT |= BIT7;
        reg_rele[15] = 1;
        break;

    case 0x30:
        P10OUT |= BIT0;
        reg_rele[16] = 1;
        break;

    case 0x31:
        P10OUT |= BIT1;
        reg_rele[17] = 1;
        break;

    case 0x32:
        P10OUT |= BIT2;
        reg_rele[18] = 1;
        break;

    case 0x33:
        P10OUT |= BIT3;
        reg_rele[19] = 1;
        break;

    case 0x34:
        P10OUT |= BIT4;
        reg_rele[20] = 1;
        break;

    case 0x35:
        P10OUT |= BIT5;
        reg_rele[21] = 1;
        break;

    case 0x36:
        P10OUT |= BIT6;
        reg_rele[22] = 1;
        break;

    case 0x37:
        P10OUT |= BIT7;
        reg_rele[23] = 1;
        break;

    case 0xF0: //BOTON APAGAR
        P8OUT &= 0x00;
        P9OUT &= 0x00;
        P10OUT &= 0x00;
        for (g1 = 0; g1 < 24; g1++)
        {
            reg_falla[g1] = 0;
            reg_falla_ant[g1] = 0;
            contador[g1] = 0;
        }
        break;

    default:
        break;
    }

}

void enviar_falla(int d, int e)
{
    if (e == 1)
    {
        switch (d)
        {
        //ENCENDER PARPADEO ALARMA
        case 0:
            UCA0TXBUF = 0xC0; //0x20
            UCA0IE |= UCTXIE;
            break;

        case 1:
            UCA0TXBUF = 0xC1;
            UCA0IE |= UCTXIE;
            break;

        case 2:
            UCA0TXBUF = 0xC2;
            UCA0IE |= UCTXIE;
            break;

        case 3:
            UCA0TXBUF = 0xC3;
            UCA0IE |= UCTXIE;
            break;

        case 4:
            UCA0TXBUF = 0xC4;
            UCA0IE |= UCTXIE;
            break;

        case 5:
            UCA0TXBUF = 0xC5;
            UCA0IE |= UCTXIE;
            break;

        case 6:
            UCA0TXBUF = 0xC6;
            UCA0IE |= UCTXIE;
            break;

        case 7:
            UCA0TXBUF = 0xC7;
            UCA0IE |= UCTXIE;
            break;

        case 8:
            UCA0TXBUF = 0xC8;
            UCA0IE |= UCTXIE;
            break;

        case 9:
            UCA0TXBUF = 0xC9;
            UCA0IE |= UCTXIE;
            break;

        case 10:
            UCA0TXBUF = 0xCA;
            UCA0IE |= UCTXIE;
            break;

        case 11:
            UCA0TXBUF = 0xCB;
            UCA0IE |= UCTXIE;
            break;

        case 12:
            UCA0TXBUF = 0xCC;
            UCA0IE |= UCTXIE;
            break;

        case 13:
            UCA0TXBUF = 0xCD;
            UCA0IE |= UCTXIE;
            break;

        case 14:
            UCA0TXBUF = 0xCE;
            UCA0IE |= UCTXIE;
            break;

        case 15:
            UCA0TXBUF = 0xCF;
            UCA0IE |= UCTXIE;
            break;

        case 16:
            UCA0TXBUF = 0xD0;
            UCA0IE |= UCTXIE;
            break;

        case 17:
            UCA0TXBUF = 0xD1;
            UCA0IE |= UCTXIE;
            break;

        case 18:
            UCA0TXBUF = 0xD2;
            UCA0IE |= UCTXIE;
            break;

        case 19:
            UCA0TXBUF = 0xD3;
            UCA0IE |= UCTXIE;
            break;

        case 20:
            UCA0TXBUF = 0xD4;
            UCA0IE |= UCTXIE;
            break;

        case 21:
            UCA0TXBUF = 0xD5;
            UCA0IE |= UCTXIE;
            break;

        case 22:
            UCA0TXBUF = 0xD6;
            UCA0IE |= UCTXIE;
            break;

        case 23:
            UCA0TXBUF = 0xD7;
            UCA0IE |= UCTXIE;
            break;

        default:
            break;
        }
    }
    else
    {
        switch (d)
        {
        //APAGAR PARPADEO ALARMA
        case 0:
            UCA0TXBUF = 0xA0; //0x00
            UCA0IE |= UCTXIE;
            break;

        case 1:
            UCA0TXBUF = 0xA1;
            //reg_falla[1]=0;
            UCA0IE |= UCTXIE;
            break;

        case 2:
            UCA0TXBUF = 0xA2;
            //reg_falla[2]=0;
            UCA0IE |= UCTXIE;
            break;

        case 3:
            UCA0TXBUF = 0xA3;
            //reg_falla[3]=0;
            UCA0IE |= UCTXIE;
            break;

        case 4:
            UCA0TXBUF = 0xA4;
            UCA0IE |= UCTXIE;
            break;

        case 5:
            UCA0TXBUF = 0xA5;
            UCA0IE |= UCTXIE;
            break;

        case 6:
            UCA0TXBUF = 0xA6;
            UCA0IE |= UCTXIE;
            break;

        case 7:
            UCA0TXBUF = 0xA7;
            UCA0IE |= UCTXIE;
            break;

        case 8:
            UCA0TXBUF = 0xA8;
            UCA0IE |= UCTXIE;
            break;

        case 9:
            UCA0TXBUF = 0xA9;
            UCA0IE |= UCTXIE;
            break;

        case 10:
            UCA0TXBUF = 0xAA;
            UCA0IE |= UCTXIE;
            break;

        case 11:
            UCA0TXBUF = 0xAB;
            UCA0IE |= UCTXIE;
            break;

        case 12:
            UCA0TXBUF = 0xAC;
            UCA0IE |= UCTXIE;
            break;

        case 13:
            UCA0TXBUF = 0xAD;
            UCA0IE |= UCTXIE;
            break;

        case 14:
            UCA0TXBUF = 0xAE;
            UCA0IE |= UCTXIE;
            break;

        case 15:
            UCA0TXBUF = 0xAF;
            UCA0IE |= UCTXIE;
            break;

        case 16:
            UCA0TXBUF = 0xB0;
            UCA0IE |= UCTXIE;
            break;

        case 17:
            UCA0TXBUF = 0xB1;
            UCA0IE |= UCTXIE;
            break;

        case 18:
            UCA0TXBUF = 0xB2;
            UCA0IE |= UCTXIE;
            break;

        case 19:
            UCA0TXBUF = 0xB3;
            UCA0IE |= UCTXIE;
            break;

        case 20:
            UCA0TXBUF = 0xB4;
            UCA0IE |= UCTXIE;
            break;

        case 21:
            UCA0TXBUF = 0xB5;
            UCA0IE |= UCTXIE;
            break;

        case 22:
            UCA0TXBUF = 0xB6;
            UCA0IE |= UCTXIE;
            break;

        case 23:
            UCA0TXBUF = 0xB7;
            UCA0IE |= UCTXIE;
            break;

        default:
            break;
        }
    }
}

