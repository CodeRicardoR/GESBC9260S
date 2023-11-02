/*
 *  This programmer uses AT91' GPIO lines
 *
 *  2006 by Carlos Camargo
 *  2007.May.10 Andres Calderon
 *  2009.Aug.26 Jose Francisco Quenta
 */

#ifndef ECB_AT91_H
#define ECB_AT91_H


#define MAP_SIZE 4096Ul
#define MAP_MASK (MAP_SIZE - 1)

#define PIOA_BASE 0xFFFFF400
#define PIOB_BASE 0xFFFFF600
#define PIOC_BASE 0xFFFFF800
#define PIOD_BASE 0xFFFFFA00

#define PB0			(1 << 0)
#define PB1			(1 << 1)
#define PB2			(1 << 2)
#define PB3			(1 << 3)
#define PB8			(1 << 8)
#define PB9			(1 << 9)
#define PB10		(1 << 10)
#define PB11		(1 << 11)
#define PB16		(1 << 16)
#define PB17		(1 << 17)
#define PB18		(1 << 18)
#define PB19		(1 << 19)
#define PB20		(1 << 20)
#define PB21		(1 << 21)
#define PB22		(1 << 22)
#define PB23		(1 << 23)
#define PB24		(1 << 24)
#define PB25		(1 << 25)
#define PB30		(1 << 30)
#define PB31		(1 << 31)

#define PC0			(1 << 0)
#define PC1			(1 << 1)
#define PC4			(1 << 4)
#define PC5			(1 << 5)
#define PC6			(1 << 6)
#define PC7			(1 << 7)
#define PC8			(1 << 8)
#define PC9			(1 << 9)
#define PC10		(1 << 10)
#define PC11		(1 << 11)
#define PC16		(1 << 16)
#define PC17		(1 << 17)
#define PC18		(1 << 18)
#define PC19		(1 << 19)
#define PC20		(1 << 20)
#define PC21		(1 << 21)
#define PC22		(1 << 22)
#define PC23		(1 << 23)
#define PC24		(1 << 24)
#define PC25		(1 << 25)
#define PC26		(1 << 26)
#define PC27		(1 << 27)
#define PC28		(1 << 28)
#define PC29		(1 << 29)
#define PC30		(1 << 30)
#define PC31		(1 << 31)


typedef volatile unsigned int AT91_REG;
/* Hardware register definition */

typedef struct _AT91S_PIO {
    AT91_REG PIO_PER;           /* PIO Enable Register */
    AT91_REG PIO_PDR;           /* PIO Disable Register */
    AT91_REG PIO_PSR;           /* PIO Status Register */
    AT91_REG Reserved0[1];
    AT91_REG PIO_OER;           /* Output Enable Register */
    AT91_REG PIO_ODR;           /* Output Disable Registerr */
    AT91_REG PIO_OSR;           /* Output Status Register */
    AT91_REG Reserved1[1];
    AT91_REG PIO_IFER;          /* Input Filter Enable Register */
    AT91_REG PIO_IFDR;          /* Input Filter Disable Register */
    AT91_REG PIO_IFSR;          /* Input Filter Status Register */
    AT91_REG Reserved2[1];
    AT91_REG PIO_SODR;          /* Set Output Data Register */
    AT91_REG PIO_CODR;          /* Clear Output Data Register */
    AT91_REG PIO_ODSR;          /* Output Data Status Register */
    AT91_REG PIO_PDSR;          /* Pin Data Status Register */
    AT91_REG PIO_IER;           /* Interrupt Enable Register */
    AT91_REG PIO_IDR;           /* Interrupt Disable Register */
    AT91_REG PIO_IMR;           /* Interrupt Mask Register */
    AT91_REG PIO_ISR;           /* Interrupt Status Register */
    AT91_REG PIO_MDER;          /* Multi-driver Enable Register */
    AT91_REG PIO_MDDR;          /* Multi-driver Disable Register */
    AT91_REG PIO_MDSR;          /* Multi-driver Status Register */
    AT91_REG Reserved3[1];
    AT91_REG PIO_PUDR;         /* Pull-up Disable Register */
    AT91_REG PIO_PUER;         /* Pull-up Enable Register */
    AT91_REG PIO_PUSR;         /* Pad Pull-up Status Register */
    AT91_REG Reserved4[1];
    AT91_REG PIO_ASR;           /* Select A Register */
    AT91_REG PIO_BSR;           /* Select B Register */
    AT91_REG PIO_ABSR;          /* AB Select Status Register */
    AT91_REG Reserved5[9];
    AT91_REG PIO_OWER;          /* Output Write Enable Register */
    AT91_REG PIO_OWDR;          /* Output Write Disable Register */
    AT91_REG PIO_OWSR;          /* Output Write Status Register */
} AT91S_PIO, *AT91PS_PIO;

void pio_out(AT91S_PIO * pio, int mask, int val);

int pio_in(AT91S_PIO * pio, int mask);

AT91S_PIO *pio_map(unsigned int piobase);

void pio_enable(AT91S_PIO * pio, int mask);

void pio_output_enable(AT91S_PIO * pio, int mask);

void pio_input_enable(AT91S_PIO * pio, int mask);

void pio_disable_irq(AT91S_PIO * pio, int mask);

void pio_disable_multiple_driver(AT91S_PIO * pio, int mask);

void pio_disable_pull_ups(AT91S_PIO * pio, int mask);

void pio_synchronous_data_output(AT91S_PIO * pio, int mask);

//funciones agregadas para la realizar el estado de algunos registros:
int ver_registro(AT91S_PIO * pio);
void pin_enable(AT91S_PIO * pio, int mask);
#endif



















