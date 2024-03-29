/******************************************************************************/
/*                                                                            */
/*  Laboratorni uloha c. 1 z predmetu IMP.                                    */
/*                                                                            */
/*  Komunikace pres UART.                                                     */
/*                                                                            */
/*  Reseni vytvoril a odevzdava: (Dominik Harmim, xharmi00).                  */
/*                                                                            */
/******************************************************************************/


#include <string.h>

#include "MKL05Z4.h"


/**
 * Inicializace MCU - zakladni nastaveni hodin, vypnuti watchdogu.
 */
void MCUInit(void)
{
	MCG->C4 |= MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x01);
	SIM->CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x00);
	SIM->COPC = SIM_COPC_COPT(0x00); // watchdog vypnut
}


/**
 * Inicializace pinu pro vysilani a prijem pres UART - RX a TX.
 */
void PinInit(void)
{
	SIM->SOPT2 |= SIM_SOPT2_UART0SRC(0x01);
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK; // Zapnout hodiny pro PORTA a PORTB.
	SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;

	PORTB->PCR[1] = 0 | PORT_PCR_MUX(0x02); // UART0_TX
	PORTB->PCR[2] = 0 | PORT_PCR_MUX(0x02); // UART0_RX

	PORTB->PCR[13] = 0 | PORT_PCR_MUX(0x01); // Beeper (PTB13).
	PTB->PDDR = GPIO_PDDR_PDD(0x2000); // "1" znamena, ze pin bude vystupni.
}


/**
 * Inicializace UART - nastaveni prenosove rychlosti 115200Bd,
 * 8 bitu, bez parity.
 */
void UART0Init(void)
{
	UART0->C2 &= ~(UART0_C2_TE_MASK | UART0_C2_RE_MASK);

	UART0->BDH = 0x00;
	UART0->BDL = 0x1A; // Baud rate 115 200 Bd, 1 stop bit.
	UART0->C4 = 0x0F; // Oversampling ratio 16, match address mode disabled.

	UART0->C1 = 0x00; // 8 data bitu, bez parity.
	UART0->C3 = 0x00;
	UART0->MA1 = 0x00; // no match address (mode disabled in C4)
	UART0->MA2 = 0x00; // no match address (mode disabled in C4)
	UART0->S1 |= 0x1F;
	UART0->S2 |= 0xC0;

	// Zapnout vysilac i prijimac.
	UART0->C2 |= UART0_C2_TE_MASK | UART0_C2_RE_MASK;
}


/**
 * Vyslani jednoho znaku pres UART - funkce vycka,
 * az je vysilaci buffer prazdny, pak posle.
 */
void SendCh(char ch)
{
	while (
		!(UART0->S1 & UART0_S1_TDRE_MASK)
		&& !(UART0->S1 & UART0_S1_TC_MASK)
	);
	UART0->D = ch;
}


/**
 * Prijeti jednoho znaku pres UART - funkce ceka na prichozi znak a
 * ten vrati jako vysledek.
 */
char ReceiveCh(void)
{
	while (!(UART0->S1 & UART0_S1_RDRF_MASK));
	return UART0->D;
}


/**
 * Vysilani retezce ukonceneho 0.
 */
void SendStr(char *s)
{
	int i = 0;
	while (s[i] != '\0')
		SendCh(s[i++]);
}


/**
 * Funkce zpozdeni - funkce skonci po nastavenem case.
 */
void delay(int bound)
{
	for (int i = 0; i < bound; i++);
}


/**
 * Pipnuti pres bzucak na PTB13 - generuje 500 period obdelnikoveho signalu.
 */
void beep(void)
{
	for (int q = 0; q < 500; q++)
	{
		PTB->PDOR = GPIO_PDOR_PDO(0x2000);
		delay(500);
		PTB->PDOR = GPIO_PDOR_PDO(0x0000);
		delay(500);
	}
}


/**
 * Hlavni funkce.
 */
int main(void)
{
	// Zakladni inicializace vlastniho MCU - hodiny, watchdog.
	MCUInit();
	// Inicializace vstupu a vystupu potrebnych pro tuto ulohu -
	// piny RX a TX UART0.
	PinInit();
	// Inicializace modulu UART0 - nastaveni prenosove rychlosti,
	// parametru prenosu.
	UART0Init();

	SendStr("Login> "); // Vyslani vyzvy k zadani loginu

	char login[9] = {'\0'}; // misto pro prijaty retezec
	for(int n = 0; n < 8; n++)
	{
		char c = ReceiveCh();
		SendCh(c); // Prijaty znak se hned vysle - echo linky.
		login[n] = c; // Postupne se uklada do pole.
	}

	if (strcmp(login, "xharmi00") == 0)
	{
		beep(); // potvrzeni pristupu
	}
	else
	{
		SendStr("\nSTOP!"); // zamitnuti pristupu
	}

	// Potom jiz neni treba nabizet dalsi zadani loginu,
	// typicky zde skoncime v prazdnem nekonecnem cyklu.

	while (1);

	// Vestavna app by nikdy nemela opustit main, ale bezet na MCU v nekonecne
	// smycce; v pripade potreby je mozno v nastrojove liste debuggeru zvolit
	// "Suspend" pro pozastaveni behu, "Resume" pro pokracovani, pripadne
	// "Restart....", po nemz je nasledne mozno app znovu spustit od pocatku
	// (pomoci "Resume") bez nutnosti znovu prekladat a programovat MCU.
}
