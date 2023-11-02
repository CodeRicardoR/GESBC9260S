/*
 *  This programmer uses AT91' GPIO lines
 *
 *  2006 by Carlos Camargo
 *  2007.May.10 Andres Calderon
 *  2009.Aug.26 Jose Francisco Quenta
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "at91gpio.h"

void pio_out(AT91S_PIO * pio, int mask, int val)
{
    if (val == 0)
        pio->PIO_CODR = mask;
    else
        pio->PIO_SODR = mask;
}


int pio_in(AT91S_PIO * pio, int mask)
{
    return (pio->PIO_PDSR & mask);
}


AT91S_PIO *pio_map(unsigned int piobase)
{
    int fd;
    void *base;

    AT91S_PIO *pio;

    off_t addr = piobase;

    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
        fprintf(stderr, "Cannot open /dev/mem.\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "/dev/mem opened.\n");

    base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr & ~MAP_MASK);

    if (base == (void *) -1) {
        fprintf(stderr, "Cannot open /dev/mem.\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Memory mapped at address %p.\n", base);

    pio = base + (addr & MAP_MASK);

    return pio;
}


void pio_enable(AT91S_PIO * pio, int mask)
{
    pio->PIO_PER = mask;        /* Enable PIO */
}

void pio_output_enable(AT91S_PIO * pio, int mask)
{
    pio->PIO_OER = mask;        /* Set TDI, TMS and TCK as outputs */
}

void pio_input_enable(AT91S_PIO * pio, int mask)
{
    pio->PIO_ODR = mask;        /* Set TDO as input */
    pio->PIO_IFER = mask;       /* Enable Input Filter */
}

void pio_disable_irq(AT91S_PIO * pio, int mask)
{
    pio->PIO_IDR = mask;        /* Disable pin interrupts */
}

void pio_disable_multiple_driver(AT91S_PIO * pio, int mask)
{
    pio->PIO_MDDR = mask;       /* Disable Multiple Diver */
}

void pio_disable_pull_ups(AT91S_PIO * pio, int mask)
{
    pio->PIO_PUDR = mask;      /* Disable Pull-Ups */
}

void pio_synchronous_data_output(AT91S_PIO * pio, int mask)
{
    pio->PIO_OWDR = mask;       /* Synchronous Data Output Write in PIO_ */
}

//Nuevas funciones:
int ver_registro(AT91S_PIO * pio){
	return (pio->PIO_PSR);
}
//Habilitar pines para usar con ADC
void pin_enable(AT91S_PIO * pio, int mask){
	pio->PIO_PDR = mask;
}




