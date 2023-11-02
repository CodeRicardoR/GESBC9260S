//Area : CIELO
//Desarrollado por Ricardo V. Rojas Quispe
// e-mail: net85.ricardo@gmail.com

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "at91gpio.h"

int main(void){
//Configuro las mascaras
	unsigned int mask_pines = PC28 + PC30;
	unsigned int pin_out = PC28;
	unsigned int pin_in = PC30;
	int ack;
	int conta;
	#define bits_in 0x44000000

	//unsigned int i;
	AT91S_PIO *pioc;

	//onfig pin:
	pioc = pio_map(PIOC_BASE);
	pio_enable(pioc, mask_pines);
	pio_disable_irq(pioc, mask_pines);
	pio_disable_multiple_driver(pioc, mask_pines);
	pio_disable_pull_ups(pioc, mask_pines);
	//Salidas:
	pio_synchronous_data_output(pioc, pin_out);
	pio_output_enable(pioc, pin_out);
	//Entradas
	pio_input_enable(pioc, pin_in);

    //SBC => '1' y espera estado de ACK
    printf("Enviando SBC : '1'\n");
    conta = 0;
    ack = 0;
    while((ack == 0) && (conta < 150)){
        pio_out(pioc,pin_out, 1);
        conta = conta + 1;
        usleep(100000);
        ack = pio_in(pioc, pin_in) && PC30;
    }
    if(ack == 1)
        printf("ACK : '1'\n");

    if(ack == 0)
        printf("ACK : '0'\n");
    
    usleep(10000000);

    //SBC => '0' y espera estado de ACK
    printf("Enviando SBC : '0'\n");
    conta = 0;
    ack = 1;
    while((ack == 1) && (conta < 150)){
        pio_out(pioc,pin_out, 0);
        conta = conta + 1;
        usleep(100000);
        ack = pio_in(pioc, pin_in) && PC30;
    }
    if(ack == 1)
        printf("ACK : '1'\n");

    if(ack == 0)
        printf("ACK : '0'\n");
    
    return 0;
}






