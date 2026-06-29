/* =========================================================================
 * Voltimetro Digital de Duas Escalas - PIC18F4520
 * Segunda Atividade Avaliativa - Proposta 2
 *
 * Com 4 DISPLAYS, para exibir 3 casas decimais nas duas escalas:
 * Escala 1: 0 a 999 mV  -> exibido como "XXX.X" V (1 casa decimal)
 * Escala 2: 1 V a 5 V   -> exibido como "X.XXX" V (3 casas decimais)
 * A troca de escala e automatica, baseada no valor medido (define apenas
 * o digito inteiro d0 e aciona o LED indicador).
 *
 * Hardware:
 *  - RA0/AN0          -> entrada analogica (tensao a medir), Vref = Vdd = 5V
 *  - RD0..RD7         -> segmentos dos displays (a,b,c,d,e,f,g,dp) via
 *                        resistores de 220 ohm (mesma fiacao em paralelo
 *                        para os 4 displays)
 *  - RB0,RB1,RB2,RB4  -> selecao do digito (displays catodo comum - CC),
 *                        ATIVO EM NIVEL BAIXO
 *                        RB0 = digito 0 (inteiro), RB1 = digito 1 (decimo),
 *                        RB2 = digito 2 (centesimo), RB4 = digito 3 (milesimo)
 *  - RB3              -> LED D1 indicador de escala: APAGADO = medindo em mV
 *                        (Escala 1), LIGADO = medindo em V (Escala 2)
 *  - Cristal X2 = 4 MHz, C2 e C3 = 22 pF -> oscilador HS
 *
 * ========================================================================= */

#include <xc.h>

#pragma config OSC   = HS
#pragma config WDT   = OFF
#pragma config LVP   = OFF
#pragma config PWRT  = OFF
#pragma config BOREN = OFF
#pragma config DEBUG = OFF
#pragma config MCLRE = ON
#pragma config PBADEN = OFF

#define _XTAL_FREQ 4000000UL   /* cristal de 4 MHz, conforme X2 */

/* ----------------------------------------------------------------------
 * Tabela de segmentos para display catodo comum (1 = segmento ligado)
 * bit0=a  bit1=b  bit2=c  bit3=d  bit4=e  bit5=f  bit6=g  bit7=dp
 * --------------------------------------------------------------------*/
const unsigned char SEG[10] = {
    0x3F, /* 0 */
    0x06, /* 1 */
    0x5B, /* 2 */
    0x4F, /* 3 */
    0x66, /* 4 */
    0x6D, /* 5 */
    0x7D, /* 6 */
    0x07, /* 7 */
    0x7F, /* 8 */
    0x6F  /* 9 */
};

/* Variaveis globais usadas pela rotina de multiplexacao (ISR do Timer0) */
volatile unsigned char g_digito[4] = {0, 0, 0, 0}; /* [0]=inteiro, [1..3]=decimais */
volatile signed char   g_dpPos     = 0;            /* posicao do ponto decimal (sempre apos d0) */
volatile unsigned char g_indiceMux = 0;

/* ---------------------- Prototipos ---------------------- */
void IO_Init(void);
void ADC_Init(void);
unsigned int ADC_Read(void);
void Timer0_Init(void);
void AtualizaDisplay(float tensao);

/* ---------------------- Interrupcao do Timer0 (refresh dos displays) --- */
void __interrupt() ISR_TMR0(void)
{
    if (INTCONbits.TMR0IF)
    {
        INTCONbits.TMR0IF = 0;

        /* desliga todos os digitos antes de trocar o padrao (evita "ghosting") */
        PORTBbits.RB0 = 1;
        PORTBbits.RB1 = 1;
        PORTBbits.RB2 = 1;
        PORTBbits.RB4 = 1;

        /* monta o padrao do digito atual */
        unsigned char padrao = SEG[g_digito[g_indiceMux]];
        if (g_dpPos == g_indiceMux) padrao |= 0x80; /* liga o ponto decimal */

        PORTD = padrao;

        /* habilita (nivel baixo) apenas o digito atual - catodo comum */
        switch (g_indiceMux)
        {
            case 0: PORTBbits.RB0 = 0; break;
            case 1: PORTBbits.RB1 = 0; break;
            case 2: PORTBbits.RB2 = 0; break;
            case 3: PORTBbits.RB4 = 0; break;
        }

        g_indiceMux++;
        if (g_indiceMux > 3) g_indiceMux = 0;

        /* recarrega Timer0 para ~2 ms (refresh ~125 Hz / 4 digitos -> sem flicker) */
        TMR0H = 0xF8;
        TMR0L = 0x30;
    }
}

void Timer0_Init(void)
{
    T0CON = 0b10001000; /* TMR0ON=1, modo 16 bits, prescaler 1:1, clock=Fosc/4=1MHz */
    TMR0H = 0xF8;
    TMR0L = 0x30;        /* 65536-2000 = 63536 = 0xF830 -> ~2 ms por digito */
    INTCONbits.TMR0IF = 0;
    INTCONbits.TMR0IE = 1;
    RCONbits.IPEN = 0;   /* sem niveis de prioridade de interrupcao */
    INTCONbits.GIE = 1;
}

void IO_Init(void)
{
    TRISD = 0x00;        /* PORTD = segmentos (saida) */
    PORTD = 0x00;

    TRISB &= 0xE0;       /* RB0,RB1,RB2,RB3,RB4 como saida (3 sel. digito + LED + 1 sel. digito) */
    PORTB |= 0x07;       /* digitos 0,1,2 desligados (nivel alto = apagado, catodo comum) */
    PORTBbits.RB4 = 1;   /* digito 3 desligado tambem */
    PORTBbits.RB3 = 0;   /* LED D1 apagado no inicio (parte de mV) */

    TRISA = 0x01;        /* RA0 = entrada (AN0) */
}

void ADC_Init(void)
{
    ADCON1 = 0x0E; /* VCFG=00 (Vref+ = Vdd, Vref- = Vss); PCFG=1110 -> so AN0 e analogico */
    ADCON2 = 0x91; /* ADFM=1 (result. justif. a direita), ACQT=010 (4 Tad), ADCS=001 (Fosc/8) */
    ADCON0 = 0x01; /* CHS=0000 (AN0), ADON=1 */
}

unsigned int ADC_Read(void)
{
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO_NOT_DONE);
    return ((unsigned int)ADRESH << 8) | ADRESL;
}

/* Converte a tensao medida (volts) para os 4 digitos a exibir, aplicando
 * a troca automatica de escala:
 *
 *  Escala 2 (tensao >= 1 V): formato "d0.d1d2d3" V  (ex: 2.340)
 *    dpPos = 0  -> ponto apos d0
 *    d0 = parte inteira (1..5)
 *    d1..d3 = casas decimais em volts
 *
 *  Escala 1 (tensao < 1 V):  formato "d0d1d2.d3" mV (ex: 945.0)
 *    dpPos = 2  -> ponto apos d2, exibindo XYZ.W mV
 *    d0..d2 = centenas/dezenas/unidades de mV (0..999)
 *    d3 = decimo de mV (sempre 0, pois resolucao do ADC ~4,89 mV)
 */
void AtualizaDisplay(float tensao)
{
    if (tensao < 0.0f) tensao = 0.0f;
    if (tensao > 5.0f) tensao = 5.0f;

    unsigned char d0, d1, d2, d3;
    signed char   dpPos;

    if (tensao >= 1.0f)
    {
        /* --- Escala 2: X.XXX V --- */
        PORTBbits.RB3 = 1; /* LED aceso = escala V */

        unsigned int milesimos = (unsigned int)(tensao * 1000.0f + 0.5f);
        if (milesimos > 5000) milesimos = 5000;

        d0    = milesimos / 1000;           /* parte inteira: 1..5        */
        unsigned int resto = milesimos % 1000;
        d1    = resto / 100;                /* 1a decimal em V            */
        d2    = (resto / 10) % 10;          /* 2a decimal em V            */
        d3    = resto % 10;                 /* 3a decimal em V            */
        dpPos = 0;                          /* ponto apos d0: "d0.d1d2d3" */
    }
    else
    {
        /* --- Escala 1: XXX.X mV --- */
        PORTBbits.RB3 = 0; /* LED apagado = escala mV */

        unsigned int decimos_mV = (unsigned int)(tensao * 10000.0f + 0.5f);
        if (decimos_mV > 9999) decimos_mV = 9999;

        d0    = (decimos_mV / 1000) % 10;  /* centenas de mV (0..9)      */
        d1    = (decimos_mV / 100)  % 10;  /* dezenas  de mV (0..9)      */
        d2    = (decimos_mV / 10)   % 10;  /* unidades de mV (0..9)      */
        d3    =  decimos_mV         % 10;  /* decimos  de mV (0..9)      */
        dpPos = 2;                          /* ponto apos d2: "d0d1d2.d3" */
    }

    /* atualizacao "atomica" das variaveis usadas pela ISR */
    INTCONbits.GIE = 0;
    g_digito[0] = d0;
    g_digito[1] = d1;
    g_digito[2] = d2;
    g_digito[3] = d3;
    g_dpPos     = dpPos;
    INTCONbits.GIE = 1;
}

void main(void)
{
    IO_Init();
    ADC_Init();
    Timer0_Init();

    while (1)
    {
        /* media de 8 leituras para reduzir ruido na medida */
        unsigned long soma = 0;
        unsigned char i;
        for (i = 0; i < 8; i++)
        {
            soma += ADC_Read();
            __delay_ms(2);
        }
        unsigned int adc_media = (unsigned int)(soma / 8);

        /* converte contagem ADC para tensao: Vref=5V, resolucao 10 bits */
        float tensao = ((float)adc_media * 5.0f) / 1023.0f;

        AtualizaDisplay(tensao);

        __delay_ms(50); /* taxa de atualizacao da medida exibida */
    }
}