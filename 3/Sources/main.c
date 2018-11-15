/******************************************************************************/
/*                                                                            */
/* Laboratorni uloha c. 3 z predmetu IMP                                      */
/*                                                                            */
/* Demonstrace rizeni jasu/barvy RGB LED pomoci PWM signalu generovaneho      */
/* casovacem.                                                                 */
/*                                                                            */
/* Reseni vytvoril(a) a odevzdava: (Dominik Harmim, xharmi00)                 */
/*                                                                            */
/******************************************************************************/


#include "MKL05Z4.h"


// Doporucena hodnota preteceni casovace.
#define OVERFLOW 0xFF


/* Funkce zpozdeni - funkce skonci po nastavenem poctu cyklu. */
void delay(long long bound)
{
	for (long long i = 0; i < bound; i++);
}


// Initialize hodin MCU, zastaveni Watchdogu (COP).
void MCUInit(void)
{
	MCG->C4 |= MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x01);
	SIM->CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x00);
	SIM->COPC = SIM_COPC_COPT(0x00);
}


// Povoleni hodin do modulu, ktere budou pouzivany.
void SysInit(void)
{
	// zdroj hodin do casovace TPM
	SIM->SOPT2 |= SIM_SOPT2_TPMSRC(0x01);
	// povoleni hodin do portu B (pro RGB LED)
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
	// povoleni hodin do casovace TPM0
	SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;
}


// Inicializace portu s RGB LED jednotlive barevne slozky LED
// budou rizeny prislusnymi kanalovymi vystupy casovace TPM0
// a to konkretne signalem PWM, krery bude casovacem generovan.
// Cisla kanalu pro LED vizte schema laboratorniho pripravku.
void PortInit(void)
{
	PORTB->PCR[9] = PORT_PCR_MUX(0x02); // PTB9: TPM0_CH2 -> red
	PORTB->PCR[10] = PORT_PCR_MUX(0x02); // PTB10: TPM0_CH1 -> green
	PORTB->PCR[11] = PORT_PCR_MUX(0x02); // PTB11: TPM0_CH0 -> blue
}


void Timer0Init(void)
{
	// 1. Vynulujte registr citace (Counter) casovace TPM0.
	TPM0_CNT = 0x0;

	// 2. Nastavte hodnotu preteceni do Modulo registru TPM0 (doporucena
	//    hodnota je definovana vyse uvedenou konstantou OVERFLOW, pripadne
	//    experimentujte s jinymi hodnotami v pripustnem rozsahu zmenou hodnoty
	//    teto konstanty).
	TPM0_MOD = OVERFLOW;

	// 3. Nastavte rezim generovani PWM na zvolenem kanalu (n) casovace v
	//    ridicim registru TPM0_CnSC tohoto kanalu, konkretne:
	//    Edge-aligned PWM: High-true pulses (clear Output on match, set
	//    Output on reload), preruseni ani DMA requests nebudou vyuzivany.
	TPM0_CnSC(0) = 0x28; // modra

	// 4. Nastavte konfiguraci casovace v jeho stavovem a ridicim registru (SC):
	//    (up counting mode pro Edge-aligned PWM, Clock Mode Selection (01),
	//    Prescale Factor Selection (Divide by 8), bez vyuziti preruseni ci DMA.
	//    POZN.: !!!!! Budete-li masky v SC registru nastavovat postupne,
	//    je NUTNO toto provadet pri Clock Mode Selection = 00 (tj.
	//    v rezimu TPM disabled).
	TPM0_SC = 0xB;
}


int main(void)
{
	// Priznak zvysovani (1) ci snizovani (0) hodnoty compare.
	int increment = 1;
	// Hodnota pro komparacni registr (urcujici stridu PWM).
	unsigned int compare = 0;

	MCUInit();
	SysInit();
	PortInit();
	Timer0Init();

	while (1)
	{
		// Zvysuj stridu (compare), dokud neni dosazeno zvolene
		// maximalni hodnoty (postupne zvysovani jasu LED).
		// Po negaci priznaku increment bude strida snizovana.
		if (increment)
		{
			compare++;
			increment = compare < OVERFLOW;
		}
		// Snizuj stridu (compare), dokud neni dosazeno nulove hodnoty
		// (postupne snizovani jasu LED), nasledne bude strida opet zvysovana.
		else
		{
			compare--;
			increment = compare == 0x00;
		}

		// 5. Priradte aktualni hodnotu compare do komparacniho registru
		//    zvoleneho kanalu casovace TPM0 (napr. kanal c. 2 pro manipulaci
		//    s cervenou slozkou RGB LED).
		TPM0_CnV(0) = compare; // modra

		// 6. LEDku nechte urcity cas svitit dle aktualni hodnoty stridy.
		//    Ve skutecnosti LED velmi rychle blika, pricemz vhodnou frekvenci
		//    signalu PWM (danou hodnotou modulo registru casovace) zajistime,
		//    ze blikani neni pro lidske oko patrne a LEDka se tak jevi, ze
		//    sviti intenzitou odpovidajici aktualni stride PWM. ZDE VYUZIJTE
		//    PRIPRAVENOU FUNKCI delay, EXPERIMENTALNE NASTAVTE HODNOTU CEKANI
		//    TAK, ABY BYLY PLYNULE ZMENY JASU LED DOBRE PATRNE.
		delay(10000);
	}
}
