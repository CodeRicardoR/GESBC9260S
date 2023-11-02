# targets
all: main clean

CC = arm-unknown-linux-gnu-gcc
AR = arm-unknown-linux-gnu-ar
 
INCLUDES = -I. 

# C++ compiler flags (-g -O2 -Wll)
CCFLAGS = -g0 -O2 -Wall
CCFLAGS2 = -g0 -O2 -Wall -lm

Libreria: at91gpio.c
	$(CC) $(INCLUDES) $(CCFLAGS) -c at91gpio.c -o at91gpio.o

clean:
	rm -f *.o  *~ *.a main

main: CheckDataV8.c
#main: AdquisitionV8.c
#main: Libreria CheckData.c
#main: SendVoltage.c

#main: Libreria GPIOresetV2.c
#	$(CC) $(INCLUDES) $(CCFLAGS) at91gpio.o map_gpio.c  -o /var/lib/tftpboot/map_gpio_dw_up
#	$(CC) $(INCLUDES) $(CCFLAGS) at91gpio.o StartSirena.c  -o /var/lib/tftpboot/GPIO_init
#	$(CC) $(INCLUDES) $(CCFLAGS) at91gpio.o CheckData.c -o /var/lib/tftpboot/CheckDataMRH
	$(CC) $(INCLUDES) $(CCFLAGS) CheckDataV8.c -o /var/lib/tftpboot/CheckDataV8
#	$(CC) $(INCLUDES) $(CCFLAGS) SendVoltage.c -o /var/lib/tftpboot/SendVoltage
#	$(CC) $(INCLUDES) $(CCFLAGS2) AdquisitionV5B.c -o /home/ricardo/Data/MagnetometerV5B
#	$(CC) $(INCLUDES) $(CCFLAGS2) AdquisitionV8.c -o /var/lib/tftpboot/MagnetometerV8_10V
#	$(CC) $(INCLUDES) $(CCFLAGS) at91gpio.o GPIOresetV2.c  -o /var/lib/tftpboot/GPIOreset

