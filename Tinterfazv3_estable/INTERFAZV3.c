//#include <msp430.h>

#include "msp430f6747a.h"

void inicializacion_osciladores();
int debounce_button(int);
void enviar_encender_rele(int);
void enviar_apagar_rele(int);
void recepcion_senal_alarma(int);
void test_leds();
void encender_apagar_interfaz(int);

int rb = 0;
int reg_botones[24];


int buzzer=0;

//VARIABLES PARPADEO FALLA
int flag_a[24] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                   1, 1, 1, 1 };
int reg_falla[24] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                      0, 0, 0, 0, 0 };
int reg_falla_ant[24]= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0 };
int j6;
unsigned int contador = 0;
int parpadeo = 1;
int flag_test=0;

// VARIABLES DIMMER
unsigned int i; //Counter
int intensidad = 0;
int dimmer = 1;
int a = 512;

//VARIABLES DEBOUNCER
int debounce = 0;
int bugged = 0;
int debounced_button = 0;
int n_button = 0;
int bit_puerto;
int bouncer_thresh = 100;
int bouncer_count = 150;

// VARIABLE UART
int msg_rcv = 0;

//VARIABLES MAIN LOOP
int reg_num_p8;
int reg_num_p9;
int reg_num_p10;

int reg_boton_p8[8];
int reg_boton_p9[8];
int reg_boton_p10[8];

int reg_boton_p8_ant[8];
int reg_boton_p9_ant[8];
int reg_boton_p10_ant[8];

int h2 = 0;
int h3 = 0;
int h4 = 0;
int j1 = 0;
int j2 = 0;
int j4 = 0;
int j3 = 0;
int j5 = 0;

int main_supply=0;
int main_supply_ant=1;

// VARIABLES ENCENDER-APAGAR INTERFAZ
int apagar_stat = 0;

int main(void)
{

    WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

    //Configuracion Oscilador interno
    inicializacion_osciladores();

    //P6DIR |= BIT0;
    //P6OUT &= ~BIT0;


    //ILUMINACION BOTONES FUNCIONES
    P3DIR |= BIT3;
    P3OUT |= BIT3;

    //LED_MAIN_SUPPLY
    P3DIR |= BIT4;
    P3OUT &= ~BIT4;

    //LED_AUX_SUPPLY
    P3DIR |= BIT5;
    P3OUT &= ~BIT5;

    // PUERTOS ENTRADA BOTONES

    P1DIR &= ~BIT6;

    P5DIR = 0x00; // Puerto 5 como entrada (SensorC 1-8)
    P4DIR = 0x00; // Puerto 4 como entrada (SensorC 9-16)
    P6DIR = 0x00; // Puerto 6 como entrada (SensorC 17-24)

    //PUERTOS SALIDA BOTONES

    P8DIR |= 0xff; // Puerto 8 como salida (Rele 1-8)
    P9DIR |= 0xff; // Puerto 9 como salida (Rele 9-16)
    P10DIR |= 0xff; // Puerto 10 como salida (Rele 17-24)

    P8OUT &= 0x00;
    P9OUT &= 0x00;
    P10OUT &= 0x00;

    //INTERRUPCION IO BTN FUNCIONES
    P2DIR &= 0x00; // Set P2.1 SEL as Input

    P2IES |= (BIT7 + BIT6 + BIT5 + BIT4); // Falling Edge 1 -> 0
    P2IFG &= 0x00; // Clear interrupt flag for P2.0
    P2IE |= (BIT7 + BIT6 + BIT5 + BIT4); // Enable interrupt for P2.0

    // INICIALIZAR UART
    P3SEL0 |= BIT0 + BIT1;                       // P3.3,4 = USCI_A0 TXD/RXD
    UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA0CTL1 |= UCSSEL_2;                     // SMCLK
    UCA0BR0 = 6;                              // 12MHz 115200 (see User's Guide)
    UCA0BR1 = 0;                              //   12MHz 115200
    UCA0MCTLW |= UCBRF_8 + UCOS16;            // Modulation UCBRSx=1, UCBRFx=0
    //UCA0MCTLW |= UCBRF_8 + UCOS16;
    UCA0CTL1 &= ~UCSWRST;                   // **Initialize USCI state machine**
    UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
    //

    //INICIALIZAR TIMER
    TA0CCTL0 |= CCIE;                          // CCR0 interrupt enabled
    TA0CCTL1 |= CCIE;                          // CCR0 interrupt enabled
    //TA0CCTL2 |= CCIE;                          // CCR0 interrupt enabled

    TA0CTL = TASSEL_1 + MC_1 + TACLR + ID_1;       // SMCLK, contmode, clear TAR
    TA0CCR0 = a * 10;
    TA0CCR1 = a * 1;
    //TA0CCR2 = a*10;
    TA0EX0 = TAIDEX_2;

    TA1CCTL0 |= CCIE;                          // CCR0 interrupt enabled
    TA1CTL = TASSEL_1 + MC_1 + ID_3 + TACLR;       // SMCLK, contmode, clear TAR
    TA1EX0 = TAIDEX_7;
    TA1CCR0 = 51200;



    //PUERTO SALIDA BUZZER
    P3DIR |= BIT2;
    P3OUT &= BIT2;

    __bis_SR_register(GIE); // Enable Global Interrupts

    _delay_cycles(60000);

    while (1)
    {

        // LEER BOTONES
        reg_boton_p8[0] = P5IN & BIT0;
        reg_boton_p8[1] = P5IN & BIT1;
        reg_boton_p8[2] = P5IN & BIT2;
        reg_boton_p8[3] = P5IN & BIT3; //(P1IN & BIT6) >> 3; PLACA 2
        reg_boton_p8[4] = P5IN & BIT4;
        reg_boton_p8[5] = P5IN & BIT5;
        reg_boton_p8[6] = P5IN & BIT6;
        reg_boton_p8[7] = P5IN & BIT7;

        reg_boton_p9[0] = P4IN & BIT0;
        reg_boton_p9[1] = P4IN & BIT1;
        reg_boton_p9[2] = P4IN & BIT2;
        reg_boton_p9[3] = P4IN & BIT3;
        reg_boton_p9[4] = P4IN & BIT4;
        reg_boton_p9[5] = P4IN & BIT5;
        reg_boton_p9[6] = P4IN & BIT6;
        reg_boton_p9[7] = P4IN & BIT7;

        reg_boton_p10[0] = P6IN & BIT0;
        reg_boton_p10[1] = P6IN & BIT1;
        reg_boton_p10[2] = P6IN & BIT2;
        reg_boton_p10[3] = P6IN & BIT3;
        reg_boton_p10[4] = P6IN & BIT4;
        reg_boton_p10[5] = P6IN & BIT5;
        reg_boton_p10[6] = P6IN & BIT6;
        reg_boton_p10[7] = P6IN & BIT7;

        ////////////////////////////////
        reg_botones[0] = reg_boton_p8[0];
        reg_botones[1] = reg_boton_p8[1];
        reg_botones[2] = reg_boton_p8[2];
        reg_botones[3] = reg_boton_p8[3];
        reg_botones[4] = reg_boton_p8[4];
        reg_botones[5] = reg_boton_p8[5];
        reg_botones[6] = reg_boton_p8[6];
        reg_botones[7] = reg_boton_p8[7];

        reg_botones[8] = reg_boton_p9[0];
        reg_botones[9] = reg_boton_p9[1];
        reg_botones[10] = reg_boton_p9[2];
        reg_botones[11] = reg_boton_p9[3];
        reg_botones[12] = reg_boton_p9[4];
        reg_botones[13] = reg_boton_p9[5];
        reg_botones[14] = reg_boton_p9[6];
        reg_botones[15] = reg_boton_p9[7];

        reg_botones[16] = reg_boton_p10[0];
        reg_botones[17] = reg_boton_p10[1];
        reg_botones[18] = reg_boton_p10[2];
        reg_botones[19] = reg_boton_p10[3];
        reg_botones[20] = reg_boton_p10[4];
        reg_botones[21] = reg_boton_p10[5];
        reg_botones[22] = reg_boton_p10[6];
        reg_botones[23] = reg_boton_p10[7];

        for (h2 = 0; h2 < 8; h2++)
        {
            if ((reg_boton_p8[h2] > 0) && (reg_boton_p8_ant[h2] == 0))
            {
                enviar_encender_rele(h2);
                enviar_encender_rele(h2);
                _delay_cycles(30000);
            }
            else if ((reg_boton_p8[h2] == 0) && (reg_boton_p8_ant[h2] > 0))
            {
                enviar_apagar_rele(h2);
                enviar_apagar_rele(h2);
                _delay_cycles(30000);
                //reg_falla[h2]=0;
            }
            reg_boton_p8_ant[h2] = reg_boton_p8[h2];
        }

        for (h3 = 0; h3 < 8; h3++)
        {
            if ((reg_boton_p9[h3] > 0) && (reg_boton_p9_ant[h3] == 0))
            {
                enviar_encender_rele(h3 + 8);
                enviar_encender_rele(h3 + 8);
                _delay_cycles(30000);
            }
            else if ((reg_boton_p9[h3] == 0) && (reg_boton_p9_ant[h3] > 0))
            {
                enviar_apagar_rele(h3 + 8);
                enviar_apagar_rele(h3 + 8);
                _delay_cycles(30000);
            }
            reg_boton_p9_ant[h3] = reg_boton_p9[h3];
        }

        for (h4 = 0; h4 < 8; h4++)
        {
            if ((reg_boton_p10[h4] > 0) && (reg_boton_p10_ant[h4] == 0))
            {
                enviar_encender_rele(h4 + 16);
                enviar_encender_rele(h4 + 16);
                _delay_cycles(30000);
            }
            else if ((reg_boton_p10[h4] == 0) && (reg_boton_p10_ant[h4] > 0))
            {
                enviar_apagar_rele(h4 + 16);
                enviar_apagar_rele(h4 + 16);
                _delay_cycles(30000);
            }
            reg_boton_p10_ant[h4] = reg_boton_p10[h4];
        }

        test_leds();
                if (flag_test==1){
                    P3OUT &= ~BIT2; //Enciende el buzzer para la accion de test
                    flag_test=0;
                }
        for (j6=0;j6<24;j6++){
            if ((reg_falla[j6] == 1) && (reg_falla_ant[j6] == 0))
            {
                buzzer = 1;
                reg_falla_ant[j6] = reg_falla[j6];
            }
            else
            {
                reg_falla_ant[j6] = reg_falla[j6];
            }
        }

        encender_apagar_interfaz(apagar_stat);

    }
}

//INTERRUPCION DCO
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
    UCSCTL5 |= DIVA_2 + DIVS_0 + DIVM_0;

}

//INTERRUPCION UART
#pragma vector=USCI_A0_VECTOR
__interrupt
void USCI0TX_ISR(void)
{
    switch (__even_in_range(UCA0IV, 4))
    {
    case 0:
        break;                             // Vector 0 - no interrupt
    case 2:
        //while (!(UCA0IFG & UCTXIFG))

        while (!(UCTXIFG))
            ;                             // USCI_A0 TX buffer ready?
        msg_rcv = UCA0RXBUF;                  // Se almacena la palabra recibida
        recepcion_senal_alarma(msg_rcv);
        break;
    case 4:
        UCA0IE &= ~UCTXIE; // Disable USCI_A0 TX interrupt
        break;
    default:
        break;
    }
}

//INTERRUPCION TIMER0 A0
#pragma vector=TIMER0_A0_VECTOR
__interrupt
void TIMER0_A0_ISR(void)
{
    for (rb = 0; rb < 24; rb++)
    {
        if (reg_botones[rb] > 0)
        {
            if ((reg_falla[rb] > 0) && (parpadeo == 1))
            {
                flag_a[rb] = 1;
            }
            else if ((reg_falla[rb] > 0) && (parpadeo == 0))
            {
                flag_a[rb] = 0;
            }
            else
            {
                flag_a[rb] = 1;
            }
        }
        else
        {
            flag_a[rb] = 1;
        }

    }

    dimmer = 1;
    P3OUT |= BIT3;

    reg_num_p8 = reg_boton_p8[0] * flag_a[0] * dimmer
            + reg_boton_p8[1] * flag_a[1] * dimmer
            + reg_boton_p8[2] * flag_a[2] * dimmer
            + reg_boton_p8[3] * flag_a[3] * dimmer
            + reg_boton_p8[4] * flag_a[4] * dimmer
            + reg_boton_p8[5] * flag_a[5] * dimmer
            + reg_boton_p8[6] * flag_a[6] * dimmer
            + reg_boton_p8[7] * flag_a[7] * dimmer;
    P8OUT = reg_num_p8;

    reg_num_p9 = reg_boton_p9[0] * flag_a[8] * dimmer
            + reg_boton_p9[1] * flag_a[9] * dimmer
            + reg_boton_p9[2] * flag_a[10] * dimmer
            + reg_boton_p9[3] * flag_a[11] * dimmer
            + reg_boton_p9[4] * flag_a[12] * dimmer
            + reg_boton_p9[5] * flag_a[13] * dimmer
            + reg_boton_p9[6] * flag_a[14] * dimmer
            + reg_boton_p9[7] * flag_a[15] * dimmer;
    P9OUT = reg_num_p9;

    reg_num_p10 = reg_boton_p10[0] * flag_a[16] * dimmer
            + reg_boton_p10[1] * flag_a[17] * dimmer
            + reg_boton_p10[2] * flag_a[18] * dimmer
            + reg_boton_p10[3] * flag_a[19] * dimmer
            + reg_boton_p10[4] * flag_a[20] * dimmer
            + reg_boton_p10[5] * flag_a[21] * dimmer
            + reg_boton_p10[6] * flag_a[22] * dimmer
            + reg_boton_p10[7] * flag_a[23] * dimmer;
    P10OUT = reg_num_p10;

}

//INTERRUPCION TIMER0 A1
#pragma vector=TIMER0_A1_VECTOR
__interrupt
void TIMER0_A1_ISR(void)
{
    switch (TA0IV)
// Read interrupt vector for TA0
    {
    case 0: // No interrupt pending

        break;

    case 2: // TA0CCR1
        dimmer = 0;
        P3OUT &= ~BIT3;

        for (rb = 0; rb < 24; rb++)
        {
            if (reg_botones[rb] > 0)
            {
                if ((reg_falla[rb] > 0) && (parpadeo == 1))
                {
                    flag_a[rb] = 1;
                }
                else if ((reg_falla[rb] > 0) && (parpadeo == 0))
                {
                    flag_a[rb] = 0;
                }
                else
                {
                    flag_a[rb] = 1;
                }
            }
            else
            {
                flag_a[rb] = 1;
            }

        }

        reg_num_p8 = reg_boton_p8[0] * flag_a[0] * dimmer
                + reg_boton_p8[1] * flag_a[1] * dimmer
                + reg_boton_p8[2] * flag_a[2] * dimmer
                + reg_boton_p8[3] * flag_a[3] * dimmer
                + reg_boton_p8[4] * flag_a[4] * dimmer
                + reg_boton_p8[5] * flag_a[5] * dimmer
                + reg_boton_p8[6] * flag_a[6] * dimmer
                + reg_boton_p8[7] * flag_a[7] * dimmer;
        P8OUT = reg_num_p8;

        reg_num_p9 = reg_boton_p9[0] * flag_a[8] * dimmer
                + reg_boton_p9[1] * flag_a[9] * dimmer
                + reg_boton_p9[2] * flag_a[10] * dimmer
                + reg_boton_p9[3] * flag_a[11] * dimmer
                + reg_boton_p9[4] * flag_a[12] * dimmer
                + reg_boton_p9[5] * flag_a[13] * dimmer
                + reg_boton_p9[6] * flag_a[14] * dimmer
                + reg_boton_p9[7] * flag_a[15] * dimmer;
        P9OUT = reg_num_p9;

        reg_num_p10 = reg_boton_p10[0] * flag_a[16] * dimmer
                + reg_boton_p10[1] * flag_a[17] * dimmer
                + reg_boton_p10[2] * flag_a[18] * dimmer
                + reg_boton_p10[3] * flag_a[19] * dimmer
                + reg_boton_p10[4] * flag_a[20] * dimmer
                + reg_boton_p10[5] * flag_a[21] * dimmer
                + reg_boton_p10[6] * flag_a[22] * dimmer
                + reg_boton_p10[7] * flag_a[23] * dimmer;
        P10OUT = reg_num_p10;

        break;

    case 4: // TA0CCR2
        //dimmer=1;
        break;

    default:
        break;

    }
}

//INTERRUPCION TIMER 1 A0
#pragma vector=TIMER1_A0_VECTOR
__interrupt
void TIMER1_A0_ISR(void)
{
    if (parpadeo == 1)
    {
        parpadeo = 0;
    }
    else
    {
        parpadeo = 1;
    }


    if ((buzzer==1) && ((P3OUT & BIT2)>0)){
        P3OUT &=~BIT2;
    }

    else if ((buzzer==1) && ((P3OUT & BIT2)==0)){
        P3OUT |=BIT2;
    }

    else if (buzzer==0){
        P3OUT &=~BIT2;
    }


}

//INTERRUPCION BOTONES
#pragma vector=PORT2_VECTOR
__interrupt
void Port_2(void)
{
    switch (P2IV)
    {

//BTN ENCENDIDO

    case 0x0A:
        n_button = 1;
        debounced_button = debounce_button(n_button);
        if (debounced_button == 1)
        {
            if (apagar_stat == 0)
            {
                apagar_stat = 1;
            }
            else
            {
                apagar_stat = 0;
            }
        }
        break;

    // BTN DIMMER
    case 0x0C:
        n_button = 2;
        debounced_button = debounce_button(n_button);
        if (debounced_button == 1)
        {
            if (intensidad < 2)
            {
                intensidad = intensidad + 1;
            }
            else
            {
                intensidad = 0;
            }
        }

        switch (intensidad)
        {
        case 0:
            TA0CCR1 = a * 1;
            break;

        case 1:
            TA0CCR1 = a * 6;
            break;

        case 2:
            TA0CCR1 = (a * 10) - 10;
            break;

        default:
            break;

        }
        break;

        //BTN BUZZER
    case 0x0E:

         n_button = 3;
         debounced_button = debounce_button(n_button);
         if (debounced_button == 1)
         {
         buzzer = 0;
         P3OUT &=~BIT2;


         }
         break;



    default:
        break;
    }

    P2IFG &= 0x00; // P2.1 IFG clear
}

int debounce_button( n_button)
{
    switch (n_button)
    {
    case 1:
        bit_puerto = BIT4;
        break;
    case 2:
        bit_puerto = BIT5;
        break;
    case 3:
        bit_puerto = BIT7;
        break;

    }

    while (bugged < bouncer_count && debounce < bouncer_thresh)
    {
        debounced_button = (P2IN & bit_puerto);
        if (debounced_button == 0)
        {
            debounce = debounce + 1;
        }
        else
        {
            debounce = debounce;
            bugged = bugged + 1;
        }
    }

    if (debounce < bouncer_thresh - 10 || debounce < bouncer_thresh + 10)
    {
        debounced_button = 1;
    }
    else
    {
        debounced_button = 0;
    }

    debounce = 0;
    bugged = 0;

    return debounced_button;
}

void test_leds()
{
    while ((P2IN & BIT7) > 0)
    {
        P3OUT |= BIT2; //Enciende el buzzer para la accion de test
        P8OUT |= 0xff; //Enciende leds botones puerto 8
        P9OUT |= 0xff; //Enciende leds botones puerto 9
        P10OUT |= 0xff;  //Enciende leds botones puerto 10
        //_delay_cycles(10);
        flag_test=1;
    }
// Se debe encender todos los puertos correspondientes a leds
}

void encender_apagar_interfaz( c4)
{
    switch (c4)
    {
    case 0:
        TA0CTL = TASSEL_1 + MC_1 + ID_1;
        TA1CTL = TASSEL_1 + MC_1 + ID_3;

        break;
    case 1:
        //__bic_SR_register(GIE);
        TA0CTL = MC_0;
        TA1CTL = MC_0;
        P3OUT &= ~BIT3;
        P8OUT &= 0x00;
        P9OUT &= 0x00;
        P10OUT &= 0x00;


        //Apagar todos los reles
        UCA0TXBUF = 0xF0;
        UCA0IE |= UCTXIE;

        //Resetea todos los registros
        for (j3 = 0; j3 < 24; j3++)
        {
            reg_botones[j3] = 0;
            reg_falla_ant[j3] = 0;
        }

        for (j4 = 0; j4 < 8; j4++)
        {
            reg_boton_p8_ant[j4] = 0;
            reg_boton_p9_ant[j4] = 0;
            reg_boton_p10_ant[j4] = 0;
        }

        while (apagar_stat == 1)
        {

            //Apagar buzzer
            buzzer=0;
            P3OUT &=~BIT2;



        }

        _delay_cycles(50000);
        break;



    default:
        break;
    }
}



void enviar_encender_rele(int c1)
{
    switch (c1)
    {
//ENVIA MSG ENCENDER RELE
    case 0:
        UCA0TXBUF = 0x20; //0x20
        UCA0IE |= UCTXIE;
        break;

    case 1:
        UCA0TXBUF = 0x21;
        UCA0IE |= UCTXIE;
        break;

    case 2:
        UCA0TXBUF = 0x22;
        UCA0IE |= UCTXIE;
        break;

    case 3:
        UCA0TXBUF = 0x23;
        UCA0IE |= UCTXIE;
        break;

    case 4:
        UCA0TXBUF = 0x24;
        UCA0IE |= UCTXIE;
        break;

    case 5:
        UCA0TXBUF = 0x25;
        UCA0IE |= UCTXIE;
        break;

    case 6:
        UCA0TXBUF = 0x26;
        UCA0IE |= UCTXIE;
        break;

    case 7:
        UCA0TXBUF = 0x27;
        UCA0IE |= UCTXIE;
        break;

    case 8:
        UCA0TXBUF = 0x28;
        UCA0IE |= UCTXIE;
        break;

    case 9:
        UCA0TXBUF = 0x29;
        UCA0IE |= UCTXIE;
        break;

    case 10:
        UCA0TXBUF = 0x2A;
        UCA0IE |= UCTXIE;
        break;

    case 11:
        UCA0TXBUF = 0x2B;
        UCA0IE |= UCTXIE;
        break;

    case 12:
        UCA0TXBUF = 0x2C;
        UCA0IE |= UCTXIE;
        break;

    case 13:
        UCA0TXBUF = 0x2D;
        UCA0IE |= UCTXIE;
        break;

    case 14:
        UCA0TXBUF = 0x2E;
        UCA0IE |= UCTXIE;
        break;

    case 15:
        UCA0TXBUF = 0x2F;
        UCA0IE |= UCTXIE;
        break;

    case 16:
        UCA0TXBUF = 0x30;
        UCA0IE |= UCTXIE;
        break;

    case 17:
        UCA0TXBUF = 0x31;
        UCA0IE |= UCTXIE;
        break;

    case 18:
        UCA0TXBUF = 0x32;
        UCA0IE |= UCTXIE;
        break;

    case 19:
        UCA0TXBUF = 0x33;
        UCA0IE |= UCTXIE;
        break;

    case 20:
        UCA0TXBUF = 0x34;
        UCA0IE |= UCTXIE;
        break;

    case 21:
        UCA0TXBUF = 0x35;
        UCA0IE |= UCTXIE;
        break;

    case 22:
        UCA0TXBUF = 0x36;
        UCA0IE |= UCTXIE;
        break;

    case 23:
        UCA0TXBUF = 0x37;
        UCA0IE |= UCTXIE;
        break;

    default:
        break;

    }
}

void enviar_apagar_rele(int c2)
{
    switch (c2)
    {
//ENVIA MSG APAGAR RELE
    case 0:
        UCA0TXBUF = 0x00; //0x00
        UCA0IE |= UCTXIE;
        break;

    case 0b00000001:
        UCA0TXBUF = 0x01;
        UCA0IE |= UCTXIE;
        break;

    case 0b00000010:
        UCA0TXBUF = 0x02;
        UCA0IE |= UCTXIE;
        break;

    case 0b00000011:
        UCA0TXBUF = 0x03;
        UCA0IE |= UCTXIE;
        break;

    case 0b00000100:
        UCA0TXBUF = 0x04;
        UCA0IE |= UCTXIE;
        break;

    case 0b00000101:
        UCA0TXBUF = 0x05;
        UCA0IE |= UCTXIE;
        break;

    case 0b00000110:
        UCA0TXBUF = 0x06;
        UCA0IE |= UCTXIE;
        break;

    case 0b00000111:
        UCA0TXBUF = 0x07;
        UCA0IE |= UCTXIE;
        break;

    case 0b00001000:
        UCA0TXBUF = 0x08;
        UCA0IE |= UCTXIE;
        break;

    case 0b00001001:
        UCA0TXBUF = 0x09;
        UCA0IE |= UCTXIE;
        break;

    case 0b00001010:
        UCA0TXBUF = 0x0A;
        UCA0IE |= UCTXIE;
        break;

    case 0b00001011:
        UCA0TXBUF = 0x0B;
        UCA0IE |= UCTXIE;
        break;

    case 0b00001100:
        UCA0TXBUF = 0x0C;
        UCA0IE |= UCTXIE;
        break;

    case 0b00001101:
        UCA0TXBUF = 0x0D;
        UCA0IE |= UCTXIE;
        break;

    case 0b00001110:
        UCA0TXBUF = 0x0E;
        UCA0IE |= UCTXIE;
        break;

    case 0b00001111:
        UCA0TXBUF = 0x0F;
        UCA0IE |= UCTXIE;
        break;

    case 0b00010000:
        UCA0TXBUF = 0x10;
        UCA0IE |= UCTXIE;
        break;

    case 0b00010001:
        UCA0TXBUF = 0x11;
        UCA0IE |= UCTXIE;
        break;

    case 0b00010010:
        UCA0TXBUF = 0x12;
        UCA0IE |= UCTXIE;
        break;

    case 0b00010011:
        UCA0TXBUF = 0x13;
        UCA0IE |= UCTXIE;
        break;

    case 0b00010100:
        UCA0TXBUF = 0x14;
        UCA0IE |= UCTXIE;
        break;

    case 0b00010101:
        UCA0TXBUF = 0x15;
        UCA0IE |= UCTXIE;
        break;

    case 0b00010110:
        UCA0TXBUF = 0x16;
        UCA0IE |= UCTXIE;
        break;

    case 0b00010111:
        UCA0TXBUF = 0x17;
        UCA0IE |= UCTXIE;
        break;

    default:
        break;

    }
}

void recepcion_senal_alarma(int c3)
{
    switch (c3)
    {

    //RECIBE ESTADO FUENTES
    case 0x60:
        P3OUT &= ~BIT4;
        buzzer=1;
        break;

    case 0x61:
        P3OUT &= ~BIT5;
        buzzer=1;
        break;

    case 0x70:
        P3OUT |= BIT4;
        buzzer=0;
        break;

    case 0x71:
        P3OUT |= BIT5;
        buzzer=0;
        break;

        //RECIBE MSG APAGAR PARPADEO
    case 0xA0:
        reg_falla[0] = 0; //0x00
        buzzer=0;
        break;

    case 0xA1:
        reg_falla[1] = 0;
        buzzer=0;
        break;

    case 0xA2:
        reg_falla[2] = 0;
        buzzer=0;
        break;

    case 0xA3:
        reg_falla[3] = 0;
        buzzer=0;
        break;

    case 0xA4:
        reg_falla[4] = 0;
        buzzer=0;
        break;

    case 0xA5:
        reg_falla[5] = 0;
        buzzer=0;
        break;

    case 0xA6:
        reg_falla[6] = 0;
        buzzer=0;
        break;

    case 0xA7:
        reg_falla[7] = 0;
        buzzer=0;
        break;

    case 0xA8:
        reg_falla[8] = 0;
        buzzer=0;
        break;

    case 0xA9:
        reg_falla[9] = 0;
        buzzer=0;
        break;

    case 0xAA:
        reg_falla[10] = 0;
        buzzer=0;
        break;

    case 0xAB:
        reg_falla[11] = 0;
        buzzer=0;
        break;

    case 0xAC:
        reg_falla[12] = 0;
        buzzer=0;
        break;

    case 0xAD:
        reg_falla[13] = 0;
        buzzer=0;
        break;

    case 0xAE:
        reg_falla[14] = 0;
        buzzer=0;
        break;

    case 0xAF:
        reg_falla[15] = 0;
        buzzer=0;
        break;

    case 0xB0:
        reg_falla[16] = 0;
        buzzer=0;
        break;

    case 0xB1:
        reg_falla[17] = 0;
        buzzer=0;
        break;

    case 0xB2:
        reg_falla[18] = 0;
        buzzer=0;
        break;

    case 0xB3:
        reg_falla[19] = 0;
        buzzer=0;
        break;

    case 0xB4:
        reg_falla[20] = 0;
        buzzer=0;
        break;

    case 0xB5:
        reg_falla[21] = 0;
        buzzer=0;
        break;

    case 0xB6:
        reg_falla[22] = 0;
        buzzer=0;
        break;

    case 0xB7:
        reg_falla[23] = 0;
        buzzer=0;
        break;

        // ENCENDER PARPADEO
    case 0xC0:
        reg_falla[0] = 1; //0x00
        buzzer=1;
        break;

    case 0xC1:
        reg_falla[1] = 1;
        //buzzer=1;
        break;

    case 0xC2:
        reg_falla[2] = 1;
        //buzzer=1;
        break;

    case 0xC3:
        reg_falla[3] = 1;
        //buzzer=1;
        break;

    case 0xC4:
        reg_falla[4] = 1;
        buzzer=1;
        break;

    case 0xC5:
        reg_falla[5] = 1;
        buzzer=1;
        break;

    case 0xC6:
        reg_falla[6] = 1;
        buzzer=1;
        break;

    case 0xC7:
        reg_falla[7] = 1;
        buzzer=1;
        break;

    case 0xC8:
        reg_falla[8] = 1;
        buzzer=1;
        break;

    case 0xC9:
        reg_falla[9] = 1;
        buzzer=1;
        break;

    case 0xCA:
        reg_falla[10] = 1;
        buzzer=1;
        break;

    case 0xCB:
        reg_falla[11] = 1;
        buzzer=1;
        break;

    case 0xCC:
        reg_falla[12] = 1;
        buzzer=1;
        break;

    case 0xCD:
        reg_falla[13] = 1;
        buzzer=1;
        break;

    case 0xCE:
        reg_falla[14] = 1;
        buzzer=1;
        break;

    case 0xCF:
        reg_falla[15] = 1;
        buzzer=1;
        break;

    case 0xD0:
        reg_falla[16] = 1;
        buzzer=1;
        break;

    case 0xD1:
        reg_falla[17] = 1;
        buzzer=1;
        break;

    case 0xD2:
        reg_falla[18] = 1;
        buzzer=1;
        break;

    case 0xD3:
        reg_falla[19] = 1;
        buzzer=1;
        break;

    case 0xD4:
        reg_falla[20] = 1;
        buzzer=1;
        //P2OUT|=BIT3;
        break;

    case 0xD5:
        reg_falla[21] = 1;
        buzzer=1;
        break;

    case 0xD6:
        reg_falla[22] = 1;
        buzzer=1;
        break;

    case 0xD7:
        reg_falla[23] = 1;
        buzzer=1;
        break;

    default:
        break;

    }
}
