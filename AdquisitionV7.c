/*
 * AdquisitionV7.c
 *
 *  Created on: Ago 05, 2014
 *      Author: Ricardo Rojas Quispe
 */

/*	
	Se trabajo con el codigo fuente de AdquisitionV2.c como referencia.
	Modificaciones:
	1. Se actualizo el formato de setuplog.cfg
	2. 09/09/2016 => Se agrego comando para lectura de datos sin GPS
	3. 24/07/2017 => Se hizo cambios segun:
		- Actualizacion de comunicacion con nuevo Firmware
		- Solo se toma la hora del GPS cuando este es valido
		- Los datos de hora de los archivos de datos son guardados con
		  hora del sistema, hora de GPS solo para actualizar la hora
		  del sistema.
	4. 20/04/2023 => Se agrego solo comentarios
	5. 23/08/2023 => Se cambio rutinas de lectura para Digitalizador de 16bits (+/- 10V)
*/

#include <stdio.h>   /* Standard input/output definitions */
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <sys/types.h>
#include <sys/stat.h>

//Definicion de constantes:
#define	device	"/dev/ttyS1"
#define	CMD	0
#define	DAT	3
#define TotalBytes 88		//Trana de bytes de datos "NNJROADQ...."

//Prototipo de funciones:
int OpenPort(void);
int ClosePort(void);
int TransmitCMD(char);
int ReceiveCMD(void);
int ReceiveString(void);
int ReceiveData(void);
int PromData(void);
int get_time(void);
int CreateFileConf(void);
int ReadFileConf(void);
int UpdateDir(char dir[]);
int InitADC(void);
int FechaHora(void);
int CreateFileSeg(void);
int CreateFileMinv(void);
int CreateFileMinm(void);
int CreateFileFg(void);

//Variables Globales:
int statusGPS;

//Buffers:
//CMD0 [10] = IdCmd (ASCII)
static unsigned char bufferCMD[14] = {165,165,165,165,74,82,79,67,77,68,48,10,13,'\0'};
//DAT0 [10] = IdDat (ASCII) [11:22] = Data
//static unsigned char bufferDAT[26] = {165,165,165,165,74,82,79,68,65,84,48,0,0,0,0,0,0,0,0,0,0,0,0,10,13,'\0'};

static unsigned char bufferTIME[12];
static unsigned char bufferX[20];
static unsigned char bufferY[20];
static unsigned char bufferZ[20];
static unsigned char bufferT1[2];
static unsigned char bufferT2[2];
static unsigned char bufferGND[2];

static unsigned char bufferRX[15];
//static unsigned char bufferTX[12];

//static unsigned char string_numday[4];
static char tiempo[50];

//Para control de fechas
static char fecha1[100]; // [IAGA]ddmmm.yy
static char fecha2[100]; // [IAGA]DDDhh.yy
static char fecha3[100]; // ddmmyy
static char fecha4[100]; // hh mm ss
static char fecha5[100]; // DD MM YYY HH mm
static char fechazip[100]; //.gz
static char copy_numday[10];
static char fecha_update[100]="2014-01-01 00:00:00"; //Para actualizar reloj de sistema 


signed long int valorX,valorY,valorZ,valorTs,valorTc;
signed long int sumX,sumY,sumZ,sumTs,sumTc;
double mvX,mvY,mvZ;

//Para ruta de archivos:
//donde sta : estacion, dd : dia, mmm : mes en letras y yy: año. DDD : dia del año, hh : hora
static char filesminm[250];
// /mnt/usb_flash/magnet/datamin/staddmmm.yym
static char filesminv[250];
// /mnt/usb_flash/magnet/datamin/staddmmm.yyv
static char filesseg[250];
// /mnt/usb_flash/magnet/dataseg/staDDDhh.yys
static char filelog[250];
// /mnt/usb_flash/magnet/historial.log
static char filefg[250];
// /mnt/usb_flash/magnet/fgxfiles.txt
static char outzip[250];
// /mnt/usb_flash/magnet/dataseg/staDDDhh.gz"
static char fileconf[250];
// /home/magnet/setuplog.cfg
static char fileszip[250];

static char yearSYS[10];
static char *dirwork = "/home/magnet";
static char *dirdata = "/mnt/usb_flash/magnet";
	
//Para para control de programa
int ss_SYS,mm_SYS,hh_SYS;
int conta_seg,conta_min;
int current_second;

//Variables de configuracion:

static char station[20];
static char IAGA[20];
static char magnetometer[20];
static char latitude[20];
static char longitude[20];
static int timeset;
static char enableGPS[20];
static double ScaleH;
static double ScaleD;
static double ScaleZ;
static double OffsetH;
static double OffsetD;
static double OffsetZ;
static double OffsetTs;
static double OffsetTc;
static double LBaseH;
static double LBaseD;
static double LBaseZ;
static double Hmean;
static double Dmean;
static double Zmean;
static double cte1;
static double cte2;
static double TempRef;
static double thFactor;
static double tzFactor;
static char pathdata[250];
static char pathseg[250];
static char pathmin[250];
static char pathzip[250];
static char pathserver1[250];
static char pathserver2[250];
static char USBdevice[250];

char buffer_str[2];
char data_str[8];

//Para comunicaciòn serial:
int ID;
struct termios oldtio,newtio;

  	
int main(void){

//Declaracion de Variables
int num, i, j, conta_line;
int flagfile, flagDAT;
int grados, minutos, segundos;
int contador;
int last_second;
int FlagReset,FlagSetTime;

char latitudGPS[30],longitudGPS[30],timeGPS[30];
char fileconf2[250];
char buffer_line[250];
char cmd_line[250];
char comillas[2]={34,'\0'};
char num_GPS[4];
char cmd_line2[250];

char *puntero;

FILE *fp;
FILE *f_temp;
FILE *file1, *file2;

	//DirWork => /home/magnet
	strcpy(fileconf, dirwork);
	strcat(fileconf, "/setuplog.cfg");
	
	//DirData => /mnt/usb_flash/magnet
	strcpy(filelog, dirdata);
	strcat(filelog,"/historial.log");
	
	//DirData => /mnt/usb_flash/magnet
	strcpy(filefg,  dirdata);
	strcat(filefg, "/fgxfiles.txt");
	
	// Verificación de puerto serial
	num = OpenPort();
	if(num){
		// rutina para detener la adquisicion
		contador = 0;
		FlagReset = 1;
		while(FlagReset){
			num = TransmitCMD('5');
			if(num){
				printf("CMD5 sended.\n");
			}else{
				printf("CMD5 sended.\n");
			}
			usleep(1300000); // 1.3 segundos

			num = TransmitCMD('0');
			if(num){
				printf("CMD0 sended.\n");
			}else{
				printf("CMD0 sended.\n");
			}
			num = ReceiveCMD();
			if(num){
				printf("CMD0 received : OK.\n");
				contador++;
				if((bufferRX[0] == CMD) && (bufferRX[1] == '0')){
					FlagReset = 0;
					printf("Digitizer Stoped.\n");
				}else{
					FlagReset = 1;
					printf("Digitizer No stoped.\n");
					if(contador > 10){
						ClosePort();
						printf("Program finished\n");
						return 0;//Fin de programa
					}
				}
			}else{
				printf("CMD0 received : Failed.\n");
				FlagReset = 1;
				usleep(700000); // 0.5 segundos
			}		
		}
		ClosePort();
	}else{
		printf("Failed Serial Port ---> ERROR:Unable open\n");
		printf("Program finished\n");
		num = get_time();
		fp = fopen(filelog,"a");
		fprintf(fp,"%cFailed Serial Port ---> ERROR:Unable open at : %c,%s ---> ERROR:001\n",34,34,tiempo);
		fprintf(fp,"%cProgram closed at : %c,%s ---> ERROR:001\n",34,34,tiempo);
		fclose(fp);
		return 0;//Fin de programa
	}

	printf("Esperando 10s...\n");
	usleep(10000000); // 10 segundos
	
	//****************************Inicio de programa de adquisicion
	printf("Inicia adquisicion!!!\n");
	num = OpenPort();
	if(num){
		//Revision de existencia de archivo de configuracion y escritura el archivo de log
		flagfile = 0;
		fp = fopen(fileconf,"r");
		if(fp == NULL){
			printf("Archivo setuplog.cfg no existe.\n");
			flagfile = 1;
		}else{
			printf("Archivo setuplog.cfg existe.\n");
			fclose(fp);
		}
		
		//Creacion de nuevo archivo setuplog.cfg:
		if(flagfile == 1){
			num = CreateFileConf();
			if(num){
				printf("Archivo setuplog.cfg creado correctamente.\n");
				flagfile = 0;
			}else{
				printf("Archivo setuplog.cfg no se pudo crear. ---> ERROR:002\n");
				fp = fopen(filelog,"a");
				if(fp == NULL)
					printf("No se pudo crear historial.log\n");
				else{
					num = get_time();
					fprintf(fp,"No se pudo crear archivo setuplog.cfg ---> ERROR:002\n");
					fprintf(fp,"%cProgram closed at : %c,%s ---> ERROR:002\n",34,34,tiempo);
					fclose(fp);
				}
				printf("Program finished\n");
				return 0;//Fin de programa
			}
		}

		//Lectura de datos de archivo setuplog.cfg
		if(flagfile == 0){
			num = ReadFileConf();
			if(num){
				printf("Imprimiendo datos de prueba:\n");
				printf("Cte1: %11.9f\n", cte1);
				printf("Cte2: %11.9f\n", cte2);
				printf("ScaleH: %4.2f\n", ScaleH);
				printf("ScaleD: %4.2f\n", ScaleD);
				printf("ScaleZ: %4.2f\n", ScaleZ);
				printf("TempRef: %3.2f\n",TempRef);
				printf("Th Factor: %3.2f\n",thFactor);
				printf("Tz Factor: %3.2f\n",tzFactor);
				printf("USB device: %s\n",USBdevice);
				num = strlen(enableGPS);
				if (num == 3)
					statusGPS = 1;
				else if (num == 2)
					statusGPS = 0;
				else
					statusGPS = 10;

				//Montaje de dispositivo USB
				strcpy(cmd_line,"");
				strcat(cmd_line,"mount ");
				strcat(cmd_line,USBdevice);
				strcat(cmd_line," /mnt/usb_flash/");
				f_temp = popen(cmd_line,"r");

				if (f_temp == NULL){
					//perror ("No se pudo ejecutar comando : %s\n",cmd_line);
					printf("Failed command %s ---> ERROR:007\n",cmd_line);
					fp = fopen(filelog,"a");
					if(fp == NULL)
						printf("No se pudo crear historial.log\n");
					else{
						num = get_time();
						fprintf(fp,"%cCommand %s failed at : %c,%s ---> ERROR:007\n",34,cmd_line,34,tiempo);
						fprintf(fp,"%cProgram closed at : %c,%s ---> ERROR:007\n",34,34,tiempo);
						fclose(fp);
					}
					printf("Program finished\n");
					return 0;//fin de programa
				}else{
					while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
					}
					pclose(f_temp);	
				}
				printf("Esperando 30s...\n");
				usleep(30000000); // 30 segundos

				//Verificacion de dispositivo USB
				strcpy(cmd_line,"");
				strcat(cmd_line,"df -h");
				f_temp = popen(cmd_line,"r");

				if (f_temp == NULL){
					//perror ("No se pudo ejecutar comando : %s\n",cmd_line);
					printf("Failed command %s ---> ERROR:008\n",cmd_line);
					fp = fopen(filelog,"a");
					if(fp == NULL)
						printf("No se pudo crear historial.log\n");
					else{
						num = get_time();
						fprintf(fp,"%cCommand %s failed at : %c,%s ---> ERROR:008\n",34,cmd_line,34,tiempo);
						fprintf(fp,"%cProgram closed at : %c,%s ---> ERROR:008\n",34,34,tiempo);
						fclose(fp);
					}
					printf("Program finished\n");
					return 0;//fin de programa
				}else{
					strcpy(cmd_line2,"");
					contador = 0;
					while(fgets(cmd_line2,sizeof(cmd_line2),f_temp) != NULL){
						puntero = strstr( cmd_line2, "/mnt/usb_flash");
						if (puntero != NULL){
							contador = contador + 1;
							printf("Memoria USB montado!\n");
							break;		
						}
					}
					pclose(f_temp);

					if (contador == 0){
						printf("Memoria USB no montado. ---> ERROR:009\n");
						fp = fopen(filelog,"a");
						if(fp == NULL)
							printf("No se pudo crear historial.log\n");
						else{
							num = get_time();
							fprintf(fp,"%cMemoria USB no montado : %c,%s ---> ERROR:009\n",34,34,tiempo);
							fprintf(fp,"%cProgram closed at : %c,%s ---> ERROR:009\n",34,34,tiempo);
							fclose(fp);
						}
						printf("Program finished\n");
						return 0;//fin de programa
					}
				}

			}else{
				printf("Archivo setuplog.cfg no se pudo leer. ---> ERROR:003\n");
				fp = fopen(filelog,"a");
				if(fp == NULL)
					printf("No se pudo crear historial.log\n");
				else{
					num = get_time();
					fprintf(fp,"No se pudo leer archivo setuplog.cfg ---> ERROR:003\n");
					fprintf(fp,"%cProgram closed at : %c,%s ---> ERROR:003\n",34,34,tiempo);
					fclose(fp);
				}
				printf("Program finished\n");
				return 0;//Fin de programa
			}

			//Actualización de carpeta de datos
			num = UpdateDir(pathdata);
			//Actualización de carpeta de datos de segundos
			num = UpdateDir(pathseg);
			//Actualización de carpeta de datos de minutos
			num = UpdateDir(pathmin);
			//Actualización de carpeta de datos zipiados
			num = UpdateDir(pathzip);
		}

		//Actualizacionb de latitud y longitud de setuplog.cfg
		if (statusGPS == 1){
			// Envio de CMD8 para sincronizacion con digitalizador:
			flagDAT = 0;
			j = 0;
			while((flagDAT == 0) && (j<10)){
				j = j + 1;
				num = TransmitCMD('8');
				if(num)
					printf("CMD8 enviado...\nEsperando DAT8 de PC...\n");
				else
					printf("CMD8 enviado...ERROR.\n");

				num = ReceiveString();
				if(num){
					if ((bufferRX[0] == DAT) && (bufferRX[1] == '4')){
						flagDAT = 1;
						break;
					}
				}else
					flagDAT = 0;
			}


			if((flagDAT == 1) && (j<10)){
				for(i=2;i<14;i++){
					if(bufferRX[i] != 'X'){
						timeGPS[i-2] = bufferRX[i];
						bufferTIME[i-2] = bufferRX[i];
					}else{
						flagDAT = 0;
						break;
					}
				}
				timeGPS[i-2] = '\0';
			}else{
				printf("Fallo envio de CMD8 ---> ERROR:004\n");
				fp = fopen(filelog,"a");
				if(fp == NULL)
					printf("No se pudo crear historial.log\n");
				else{
					num = get_time();
					fprintf(fp,"%cCommand CMD8 failed at : %c,%s ---> ERROR:004\n",34,34,tiempo);
					fprintf(fp,"%cProgram closed at : %c,%s ---> ERROR:004\n",34,34,tiempo);
					fclose(fp);
				}

				//****************************Actualizacion de hora UTC con servidor de LISN
				strcpy(cmd_line,"/home/magnet/UpdateTime");
				printf("cmd_line : %s\n",cmd_line);
				f_temp = popen(cmd_line,"r");
				while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
				}
				pclose(f_temp);
				flagDAT = 0;
			}

			if(flagDAT == 1){
				printf("Time GPS OK => %s.\n",timeGPS);
				fecha_update[2] = timeGPS[10];
				fecha_update[3] = timeGPS[11];
				fecha_update[5] = timeGPS[8];
				fecha_update[6] = timeGPS[9];
				fecha_update[8] = timeGPS[6];
				fecha_update[9] = timeGPS[7];
				fecha_update[11] = timeGPS[0];
				fecha_update[12] = timeGPS[1];
				fecha_update[14] = timeGPS[2];
				fecha_update[15] = timeGPS[3];
				fecha_update[17] = timeGPS[4];
				fecha_update[18] = timeGPS[5];
				
				//Actualizando hora de sistema:
				strcpy(cmd_line,"date -s ");
				strcat(cmd_line,comillas);
				strcat(cmd_line,fecha_update);
				strcat(cmd_line,comillas);
				printf("cmd: %s\n",cmd_line);			
				f_temp = popen(cmd_line,"r");
				pclose(f_temp);
				strcpy(cmd_line,"");
				strcat(cmd_line,"hwclock -w");
				printf("cmd: %s\n",cmd_line);
				f_temp = popen(cmd_line,"r");
				pclose(f_temp);
			
			}else{
				printf("Time GPS Failed => %s.\n", timeGPS);
				//****************************Actualizacion de hora UTC con servidor de LISN
				strcpy(cmd_line,"/home/magnet/UpdateTime");
				printf("cmd_line : %s\n",cmd_line);
				f_temp = popen(cmd_line,"r");
				while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
				}
				pclose(f_temp);
			}

			// Envio de CMD1 para obtener datos de latiud de GPS:
			num = TransmitCMD('1');
			if(num)
				printf("CMD1 enviado...\nEsperando DAT1 de PC...\n");
			else
				printf("CMD1 enviado...ERROR.\n");
			
			flagDAT = 0;
			for(i=0;i<5;i++){			
				num = ReceiveString();
				if(num){
					if ((bufferRX[0] == DAT) && (bufferRX[1] == '1')){
						flagDAT = 1;
						break;
					}
				}else
					flagDAT = 0;
			}
			if(flagDAT){
				for(i=2;i<14;i++){
					if(bufferRX[i]!= 'X')
						latitudGPS[i-2] = bufferRX[i];
					else{
						flagDAT = 0;
						break;
					}
				}
				latitudGPS[i-2] = '\0';
				printf("Latitud => %s.\n",latitudGPS);
			}

			if(flagDAT == 0){
				printf("Fallo recepcion de DAT1. ---> WARNING:001\n");
				strcpy(latitudGPS,"0000.0000,S ");
				fp = fopen(filelog,"a");
				if(fp == NULL)
					printf("No se pudo crear historial.log\n");
				else{
					num = get_time();
					fprintf(fp,"%cCommand CMD1 failed at : %c,%s ---> WARNING:001\n",34,34,tiempo);
					fclose(fp);
				}
			}

			// Envio de CMD2 para obtener datos de longitud de GPS:
			num = TransmitCMD('2');
			if(num)
				printf("CMD2 enviado...\nEsperando DAT2 de PC...\n");
			else
				printf("CMD2 enviado...ERROR.\n");

			flagDAT = 0;
			for(i=0;i<5;i++){			
				num = ReceiveString();
				if(num){
					if ((bufferRX[0] == DAT) && (bufferRX[1] == '2')){
						flagDAT = 1;
						break;
					}
				}else
					flagDAT = 0;
			}
			if(flagDAT){
				for(i=2;i<14;i++){
					if(bufferRX[i] != 'X')
						longitudGPS[i-2] = bufferRX[i];
					else{
						flagDAT = 0;
						break;
					}
				}
				longitudGPS[i-2] = '\0';
				printf("Longitud => %s.\n",longitudGPS);
			}
			if (flagDAT == 0){
				printf("Fallo recepcion de DAT2.---> WARNING:002\n");
				strcpy(longitudGPS,"00000.0000,W");
				fp = fopen(filelog,"a");
				if(fp == NULL)
					printf("No se pudo crear historial.log\n");
				else{
					num = get_time();
					fprintf(fp,"%cCommand CMD2 failed at : %c,%s ---> WARNING:002\n",34,34,tiempo);
					fclose(fp);
				}
			}

			// Inicialización de ADC
			num = InitADC();
			if(num == 0){
				printf("No se logro inicializar el ADC. ---> ERROR:005\n");
				fp = fopen(filelog,"a");
				if(fp == NULL)
					printf("No se pudo crear historial.log\n");
				else{
					num = get_time();
					fprintf(fp,"No se logro inicializar el ADC ---> ERROR:005\n");
					fprintf(fp,"%cProgram closed at : %c,%s ---> ERROR:005\n",34,34,tiempo);
					fclose(fp);
				}
				printf("Program finished\n");
				return 0; //fin de programa
			}
			// Actualizacion de archivo de configuracion setuplog.cfg:
			strcpy(fileconf2, dirwork);
			strcat(fileconf2, "/setuplog_old.cfg");
			strcpy(cmd_line, "cp ");
			strcat(cmd_line, fileconf);
			strcat(cmd_line, " ");
			strcat(cmd_line, fileconf2);
			printf("cmd_line : %s\n", cmd_line);
			f_temp = popen(cmd_line, "r");
			pclose(f_temp);
			strcpy(cmd_line,"rm ");
			strcat(cmd_line, fileconf);
			printf("cmd_line : %s\n", cmd_line);
			f_temp = popen(cmd_line, "r");
			pclose(f_temp);
			file1 = fopen(fileconf2, "r");
			file2 = fopen(fileconf, "w");
			if((file1 == NULL) || (file2 == NULL)){
				printf("No se logro hacer copia de setuplog.cfg ---> ERROR:006\n");
				fp = fopen(filelog,"a");
				if(fp == NULL)
					printf("No se pudo crear historial.log\n");
				else{
					num = get_time();
					fprintf(fp,"%cFile setuplog_old.cfg failed at : %c,%s---> ERROR:006\n",34,34,tiempo);
					fprintf(fp,"%cProgram closed at : %c,%s ---> ERROR:006\n",34,34,tiempo);
					fclose(fp);
				}
				printf("Program finished\n");
				return 0; //fin de programa
			}else{
				conta_line = 0;
				while(fgets(buffer_line,sizeof(buffer_line),file1) != NULL){
					conta_line = conta_line + 1;			
					if(conta_line == 10){
						//Grados de latitud
						num_GPS[0] = latitudGPS[0];
						num_GPS[1] = latitudGPS[1];
						num_GPS[2] = '\0';
						grados = atoi(num_GPS);
						//Minutos de latitud
						num_GPS[0] = latitudGPS[2];
						num_GPS[1] = latitudGPS[3];
						num_GPS[2] = '\0';
						minutos = atoi(num_GPS);
						//Segundos de latitud
						num_GPS[0] = latitudGPS[5];
						num_GPS[1] = latitudGPS[6];
						num_GPS[2] = '\0';
						segundos = atoi(num_GPS);
						segundos = segundos*60/100;
						if(latitudGPS[10] == 'S')
							grados = grados*(-1);
						fprintf(file2,"%cLatitude                %c,%c",34,34,34);
						fprintf(file2,"%d°%d'%d%c\n",grados,minutos,segundos,34);

					}else if(conta_line == 11){
						//Grados de longitud
						num_GPS[0] = longitudGPS[0];
						num_GPS[1] = longitudGPS[1];
						num_GPS[2] = longitudGPS[2];
						num_GPS[3] = '\0';
						grados = atoi(num_GPS);
						//Minutos de longitud
						num_GPS[0] = longitudGPS[3];
						num_GPS[1] = longitudGPS[4];
						num_GPS[2] = '\0';
						minutos = atoi(num_GPS);
						//Segundos de longitud
						num_GPS[0] = longitudGPS[6];
						num_GPS[1] = longitudGPS[7];
						num_GPS[2] = '\0';
						segundos = atoi(num_GPS);
						segundos = segundos*60/100;
						if(latitudGPS[10] == 'W')
							grados = grados*(-1);
						fprintf(file2,"%cLongitude               %c,%c",34,34,34);
						fprintf(file2,"%d°%d'%d%c\n",grados,minutos,segundos,34);

					}else
						fprintf(file2,"%s",buffer_line);					
				}
				fclose(file1);
				fclose(file2);
			}
						
		}else if (statusGPS == 0){
			//****************************Actualizacion de hora UTC con servidor de LISN
			strcpy(cmd_line, "/home/magnet/UpdateTime");
			printf("cmd_line : %s\n",cmd_line);
			f_temp = popen(cmd_line,"r");
			while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
			}
			pclose(f_temp);
			
			//Configuración sin usar GPS:
			num = InitADC();
			if(num == 0){
				printf("No se logro inicializar el ADC. ---> ERROR:007\n");
				fp = fopen(filelog,"a");
				if(fp == NULL)
					printf("No se pudo crear historial.log\n");
				else{
					num = get_time();
					fprintf(fp,"No se logro inicializar el ADC ---> ERROR:007\n");
					fprintf(fp,"%cProgram closed at : %c,%s ---> ERROR:007\n",34,34,tiempo);
					fclose(fp);
				}
				printf("Program finished\n");
				return 0; //fin de programa
			}					
		}else{
			printf("ERROR Enable GPS: yes or no?\n");
			return 0; //fin de programa
		}

		//Lectura de datos digitalizados
		num = FechaHora();

		last_second = ss_SYS;
		if(statusGPS == 1){
			//Inicializacion de variables:
			conta_seg = 0;
			conta_min = 0;
			sumX = 0;
			sumY = 0;
			sumZ = 0;
			sumTs = 0;
			sumTc = 0;
			
			strcat(pathserver1, "/20");
			strcat(pathserver1, yearSYS);
			printf("Server1 : %s\n", pathserver1);
			strcat(pathserver2,"/");
			strcat(pathserver2, IAGA);
			strcat(pathserver2, yearSYS);
			printf("Server2 : %s\n", pathserver2);
			num = CreateFileSeg();
			if(num){
				strcpy(fileszip, filesseg);
				printf("Initial file to compress: OK : %s\n", filesseg);			
			}			
			strcpy(outzip, pathzip);
			strcat(outzip, "/");
			strcat(outzip, fecha3);
			num = UpdateDir(outzip);
			if(num){
				strcat(outzip, "/");
				strcat(outzip, fechazip);
				printf("File OK : %s \n", outzip);
			}else{	
				printf("Error al crear carpeta ZIP/fecha3\n");
				fp = fopen(filelog,"a");
				if(fp == NULL)
					printf("No se pudo crear historial.log\n");
				else{
					num = get_time();
					fprintf(fp,"%cFunction UpdateDir(pathzip/fecha3) failed at : %c,%s\n",34,34,tiempo);
					fclose(fp);
				}
			}
	
			//Creación de archivo fgxfiles.txt
			num = CreateFileFg();
			printf("Starting receive data...\n");
			//Inicio de lectura de datos
			num = TransmitCMD('4');	
			while(1){
				num = ReceiveData();
				if(num == TotalBytes){
					num = PromData();
					//---------------------------------------
					//Verificando cambio de segundos:	
					num = get_time();
					while(current_second == last_second){
						num = get_time();
						usleep(5000); // 5 milisegundos
					}
					//---------------------------------------
					last_second = current_second;
					num = FechaHora();
					//printf("time : %s  ",fecha4);
					num = CreateFileSeg();
					if(num == 0){
						printf("Error a escribir archivo %s.\n",filesseg);
						fp = fopen(filelog,"a");
						if(fp == NULL)
							printf("No se pudo crear historial.log\n");
						else{
							num = get_time();
							fprintf(fp,"%cFile seg failed at : %c,%s\n",34,34,tiempo);
							fclose(fp);
						}
					}

					if(ss_SYS == 59){
						sumX = sumX/conta_seg;
						sumY = sumY/conta_seg;
						sumZ = sumZ/conta_seg;
						sumTc = sumTc/conta_seg;
						sumTs = sumTs/conta_seg;

						printf("Data in last minute: %d\n", conta_seg);
						num = CreateFileMinv();
						if(num == 0){
							printf("Error a escribir archivo %s.\n",filesminv);
							fp = fopen(filelog,"a");
							if(fp == NULL)
								printf("No se pudo crear historial.log\n");
							else{
								num = get_time();
								fprintf(fp,"%cFile minv failed at : %c,%s\n",34,34,tiempo);
								fclose(fp);
							}
						}
						num = CreateFileMinm();
						if(num == 0){
							printf("Error a escribir archivo %s.\n",filesminm);
							fp = fopen(filelog,"a");
							if(fp == NULL)
								printf("No se pudo crear historial.log\n");
							else{
								num = get_time();
								fprintf(fp,"%cFile minm failed at : %c,%s\n",34,34,tiempo);
								fclose(fp);
							}
						}
						conta_seg = 0;
						sumX = 0;
						sumY = 0;
						sumZ = 0;
						sumTs = 0;
						sumTc = 0;
						conta_min = conta_min + 1;
					}

					if(timeset == conta_min){
						if(ss_SYS == 5){
							//Envio de archivo minm:
							strcpy(cmd_line, "python ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/SendfileFTP.py ");
							strcat(cmd_line, pathserver1);
							strcat(cmd_line, " ");
							strcat(cmd_line, filesminm);
							strcat(cmd_line, " ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/lisn_logon1.txt");
							strcat(cmd_line, " >/dev/null 2>&1 &");
							//printf("cmd_line : %s\n",cmd_line);
							f_temp = popen(cmd_line,"r");
							while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
							}
							pclose(f_temp);
						
							strcpy(cmd_line, "python ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/SendfileFTP.py ");
							strcat(cmd_line, pathserver1);
							strcat(cmd_line, " ");
							strcat(cmd_line, filesminv);
							strcat(cmd_line, " ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/lisn_logon1.txt");
							strcat(cmd_line, " >/dev/null 2>&1 &");
							//printf("cmd_line : %s\n",cmd_line);
							f_temp = popen(cmd_line,"r");
							while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
							}
							pclose(f_temp);
							printf("Minute files FTP sent...\n");

							//Actualizacion de datos setuplog.cfg
							num = ReadFileConf();
							strcat(pathserver1, "/20");
							strcat(pathserver1, yearSYS);
							printf("Server1 : %s\n", pathserver1);
							strcat(pathserver2, "/");
							strcat(pathserver2, IAGA);
							strcat(pathserver2, yearSYS);
							printf("Server2 : %s\n", pathserver2);
							conta_min = 0;
						}
					}
					if(mm_SYS == 0){
						if(ss_SYS == 30){
							//Generacion de archivo comprimido
							strcpy(cmd_line, "gzip -c ");
							strcat(cmd_line, fileszip);
							strcat(cmd_line, " > ");
							strcat(cmd_line, outzip);
							strcat(cmd_line, " &");
							//printf("cmd_line : %s\n",cmd_line);
							f_temp = popen(cmd_line,"r");
							while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
							}
							pclose(f_temp);
							printf("File SEC compressed...\n");
						}

						if(ss_SYS == 50){
							//Envio de archivo comprimido
							strcpy(cmd_line, "python ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/SendfileFTP.py ");
							strcat(cmd_line, pathserver1);
							strcat(cmd_line, " ");
							strcat(cmd_line, outzip);
							strcat(cmd_line, " ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/lisn_logon1.txt");
							strcat(cmd_line, " >/dev/null 2>&1 &");
							//printf("cmd_line : %s\n",cmd_line);
							f_temp = popen(cmd_line,"r");
							while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
							}
							pclose(f_temp);

							strcpy(cmd_line, "python ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/SendfileFTP.py ");
							strcat(cmd_line, pathserver2);
							strcat(cmd_line, " ");
							strcat(cmd_line, outzip);
							strcat(cmd_line, " ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/lisn_logon2.txt");
							strcat(cmd_line, " >/dev/null 2>&1 &");
							//printf("cmd_line : %s\n",cmd_line);
							f_temp = popen(cmd_line,"r");
							while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
							}
							pclose(f_temp);
							printf("Second ZIP files FTP sent...\n");

							//Actualizacion de nombre de archivo zip
							strcpy(fileszip,filesseg);
							strcpy(outzip,pathzip);
							strcat(outzip,"/");
							strcat(outzip,fecha3);

							num = UpdateDir(outzip);
							if(num){
								strcat(outzip,"/");
								strcat(outzip,fechazip);
								//printf("File OK : %s \n",outzip);
								//Creación de archivo fgxfiles.txt
								num = CreateFileFg();
							}else{	
								printf("Error al crear carpeta ZIP/fecha3\n");
								fp = fopen(filelog,"a");
								if(fp == NULL)
									printf("No se pudo crear historial.log\n");
								else{
									num = get_time();
									fprintf(fp,"%cFunction UpdateDir(pathzip/fecha3) failed at : %c,%s\n",34,34,tiempo);
									fclose(fp);
								}
							}
							//Actualizacion de datos setuplog.cfg
							num = ReadFileConf();
							strcat(pathserver1, "/20");
							strcat(pathserver1, yearSYS);
							printf("Server1 : %s\n", pathserver1);
							strcat(pathserver2, "/");
							strcat(pathserver2, IAGA);
							strcat(pathserver2, yearSYS);
							printf("Server2 : %s\n", pathserver2);
							conta_min = 0;

							//Actualizando fecha y hora del sistema
							FlagSetTime = 1;
							for(i=0; i<12; i++){
								if (bufferTIME[i] == 'X'){
									FlagSetTime = 0;
									break;
								}
							}
							if(FlagSetTime){
								fecha_update[2] = bufferTIME[10];
								fecha_update[3] = bufferTIME[11];
								fecha_update[5] = bufferTIME[8];
								fecha_update[6] = bufferTIME[9];
								fecha_update[8] = bufferTIME[6];
								fecha_update[9] = bufferTIME[7];
								fecha_update[11] = bufferTIME[0];
								fecha_update[12] = bufferTIME[1];
								fecha_update[14] = bufferTIME[2];
								fecha_update[15] = bufferTIME[3];
								fecha_update[17] = bufferTIME[4];
								fecha_update[18] = bufferTIME[5];
								//Actualizando hora de sistema:
								strcpy(cmd_line,"date -s ");
								strcat(cmd_line,comillas);
								strcat(cmd_line,fecha_update);
								strcat(cmd_line,comillas);
								printf("cmd: %s\n",cmd_line);			
								f_temp = popen(cmd_line,"r");
								pclose(f_temp);
								strcpy(cmd_line,"");
								strcat(cmd_line,"hwclock -w");
								printf("cmd: %s\n",cmd_line);
								f_temp = popen(cmd_line,"r");
								pclose(f_temp);

							}
						}
						
					}
				}else
					printf("Error al recibir datos = %d\n", num);
			}
			num = TransmitCMD('5');
			if(num)			
				printf("CMD5 enviado...\n***Fin de adquisicion...\n");
			else
				printf("CMD5 enviado...ERROR.");

			printf("Program finished\n");
			return 0; //fin de programa		
		
		}else if (statusGPS == 0){
			//Inicializacion de variables:
			conta_seg = 0;
			conta_min = 0;
			sumX = 0;
			sumY = 0;
			sumZ = 0;
			sumTs = 0;
			sumTc = 0;

			strcat(pathserver1, "/20");
			strcat(pathserver1, yearSYS);
			printf("Server1 : %s\n", pathserver1);
			strcat(pathserver2, "/");
			strcat(pathserver2, IAGA);
			strcat(pathserver2, yearSYS);
			printf("Server2 : %s\n", pathserver2);
			num = CreateFileSeg();
			if(num){
				strcpy(fileszip,filesseg);
				printf("Initial file OK : %s\n",filesseg);			
			}			
			strcpy(outzip, pathzip);
			strcat(outzip, "/");
			strcat(outzip, fecha3);
			num = UpdateDir(outzip);
			if(num){
				strcat(outzip,"/");
				strcat(outzip,fechazip);
				printf("File OK : %s \n",outzip);
			}else{	
				printf("Error al crear carpeta ZIP/fecha3\n");
				fp = fopen(filelog,"a");
				if(fp == NULL)
					printf("No se pudo crear historial.log\n");
				else{
					num = get_time();
					fprintf(fp,"%cFunction UpdateDir(pathzip/fecha3) failed at : %c,%s\n",34,34,tiempo);
					fclose(fp);
				}
			}
			
			//Creación de archivo fgxfiles.txt
			num = CreateFileFg();
			printf("Starting receive data...in 1 seconds\n");
			while(1){
				num = FechaHora();
				if (last_second != ss_SYS){
					last_second = ss_SYS;
					num = TransmitCMD('9');
					num = ReceiveData();
					if(num == TotalBytes){
						num = PromData();
						//printf("time : %s  ",fecha4);
						num = CreateFileSeg();
						if(num == 0){
							printf("Error a escribir archivo %s.\n",filesseg);
							fp = fopen(filelog,"a");
							if(fp == NULL)
								printf("No se pudo crear historial.log\n");
							else{
								num = get_time();
								fprintf(fp,"%cFile seg failed at : %c,%s\n",34,34,tiempo);
								fclose(fp);
							}
						}

						if(ss_SYS == 59){
							sumX = sumX/conta_seg;
							sumY = sumY/conta_seg;
							sumZ = sumZ/conta_seg;
							sumTc = sumTc/conta_seg;
							sumTs = sumTs/conta_seg;

							printf("Data in last minute : %d\n",conta_seg);
							num = CreateFileMinv();
							if(num == 0){
								printf("Error a escribir archivo %s.\n",filesminv);
								fp = fopen(filelog,"a");
								if(fp == NULL)
									printf("No se pudo crear historial.log\n");
								else{
									num = get_time();
									fprintf(fp,"%cFile minv failed at : %c,%s\n",34,34,tiempo);
									fclose(fp);
								}
							}
							num = CreateFileMinm();
							if(num == 0){
								printf("Error a escribir archivo %s.\n",filesminm);
								fp = fopen(filelog,"a");
								if(fp == NULL)
									printf("No se pudo crear historial.log\n");
								else{
									num = get_time();
									fprintf(fp,"%cFile minm failed at : %c,%s\n",34,34,tiempo);
									fclose(fp);
								}
							}
							conta_seg = 0;
							sumX = 0;
							sumY = 0;
							sumZ = 0;
							sumTs = 0;
							sumTc = 0;
							conta_min = conta_min + 1;
						}

						if(timeset == conta_min){
							if(ss_SYS == 5){
								//Envio de archivo minm:
								strcpy(cmd_line, "python ");
								strcat(cmd_line, dirwork);
								strcat(cmd_line, "/SendfileFTP.py ");
								strcat(cmd_line, pathserver1);
								strcat(cmd_line, " ");
								strcat(cmd_line, filesminm);
								strcat(cmd_line, " ");
								strcat(cmd_line, dirwork);
								strcat(cmd_line, "/lisn_logon1.txt");
								strcat(cmd_line, " >/dev/null 2>&1 &");
								//printf("cmd_line : %s\n", cmd_line);
								f_temp = popen(cmd_line,"r");
								while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
								}
								pclose(f_temp);
						
								strcpy(cmd_line, "python ");
								strcat(cmd_line, dirwork);
								strcat(cmd_line, "/SendfileFTP.py ");
								strcat(cmd_line, pathserver1);
								strcat(cmd_line, " ");
								strcat(cmd_line, filesminv);
								strcat(cmd_line, " ");
								strcat(cmd_line, dirwork);
								strcat(cmd_line, "/lisn_logon1.txt");
								strcat(cmd_line, " >/dev/null 2>&1 &");
								//printf("cmd_line : %s\n", cmd_line);
								f_temp = popen(cmd_line,"r");
								while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
								}
								pclose(f_temp);
								printf("Minute files FTP sent...\n");

								//Actualizacion de datos setuplog.cfg
								num = ReadFileConf();
								strcat(pathserver1, "/20");
								strcat(pathserver1, yearSYS);
								printf("Server1 : %s\n", pathserver1);
								strcat(pathserver2, "/");
								strcat(pathserver2, IAGA);
								strcat(pathserver2, yearSYS);
								printf("Server2 : %s\n", pathserver2);
								conta_min = 0;
							}
						}
						if(mm_SYS == 0){
							if(ss_SYS == 30){
								//Generacion de archivo comprimido
								strcpy(cmd_line, "gzip -c ");
								strcat(cmd_line, fileszip);
								strcat(cmd_line, " > ");
								strcat(cmd_line, outzip);
								strcat(cmd_line, " &");
								//printf("cmd_line : %s\n", cmd_line);
								f_temp = popen(cmd_line,"r");
								while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
								}
								pclose(f_temp);
								printf("File SEC compressed...\n");
							}

							if(ss_SYS == 50){
								//Envio de archivo comprimido
								strcpy(cmd_line, "python ");
								strcat(cmd_line, dirwork);
								strcat(cmd_line, "/SendfileFTP.py ");
								strcat(cmd_line, pathserver1);
								strcat(cmd_line, " ");
								strcat(cmd_line, outzip);
								strcat(cmd_line, " ");
								strcat(cmd_line, dirwork);
								strcat(cmd_line, "/lisn_logon1.txt");
								strcat(cmd_line, " >/dev/null 2>&1 &");
								//printf("cmd_line : %s\n", cmd_line);
								f_temp = popen(cmd_line,"r");
								while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
								}
								pclose(f_temp);

								strcpy(cmd_line, "python ");
								strcat(cmd_line, dirwork);
								strcat(cmd_line, "/SendfileFTP.py ");
								strcat(cmd_line, pathserver2);
								strcat(cmd_line, " ");
								strcat(cmd_line, outzip);
								strcat(cmd_line, " ");
								strcat(cmd_line, dirwork);
								strcat(cmd_line, "/lisn_logon2.txt");
								strcat(cmd_line, " >/dev/null 2>&1 &");
								//printf("cmd_line : %s\n", cmd_line);
								f_temp = popen(cmd_line, "r");
								while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
								}
								pclose(f_temp);
								printf("Second ZIP files FTP sent...\n");

								//Actualizacion de nombre de archivo zip
								strcpy(fileszip, filesseg);
								strcpy(outzip, pathzip);
								strcat(outzip, "/");
								strcat(outzip, fecha3);
								num = UpdateDir(outzip);
								if(num){
									strcat(outzip, "/");
									strcat(outzip, fechazip);
									//printf("File OK : %s \n", outzip);
									//Creación de archivo fgxfiles.txt
									num = CreateFileFg();
								}else{	
									printf("Error al crear carpeta ZIP/fecha3\n");
									fp = fopen(filelog,"a");
									if(fp == NULL)
										printf("No se pudo crear historial.log\n");
									else{
										num = get_time();
										fprintf(fp,"%cFunction UpdateDir(pathzip/fecha3) failed at : %c,%s\n",34,34,tiempo);
										fclose(fp);
									}
								}

								//Actualizacion de datos setuplog.cfg
								num = ReadFileConf();
								strcat(pathserver1, "/20");
								strcat(pathserver1, yearSYS);
								printf("Server1 : %s\n", pathserver1);
								strcat(pathserver2, "/");
								strcat(pathserver2, IAGA);
								strcat(pathserver2, yearSYS);
								printf("Server2 : %s\n", pathserver2);
								conta_min = 0;
							}
						
						}

					}else
						printf("Error al recibir datos = %d\n",num);
				}
				//Retardo de 15 ms
				usleep(15000); // 15 milisegundos
			}
			num = TransmitCMD('5');
			if(num)			
				printf("CMD5 enviado...\n***Fin de adquisicion...\n");
			else
				printf("CMD5 enviado...ERROR.\n");
		
			printf("Program finished\n");
			return 0; //fin de programa	
		}else{
			printf("ERROR Enable GPS: yes or no?\n");
			printf("Program finished\n");
			return 0; //fin de programa
		}

	}else{
		printf("No se pudo abrir puerto.\n");
		fp = fopen(filelog,"a");
		if(fp == NULL)
			printf("No se pudo crear historial.log\n");
		else{
			num = get_time();
			fprintf(fp,"%cSerial Port failed : %c,%s\n",34,34,tiempo);
			fclose(fp);
		}		
	}
	printf("Program finished\n");
	return 0;//fin de programa
}


/*FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF*/
//Funciones:
int OpenPort(void){
	ID = open(device,O_RDWR | O_NOCTTY | O_NDELAY);
	if (ID < 0){
		perror("Can not open device.\n");
		return -1;
	}
	fcntl(ID,F_SETFL,0);
	tcgetattr(ID,&oldtio);					/*Salvamos configuracion actual del puerto*/

	//Configuratio serial port: "115200,8,N,1"
	bzero(&newtio,sizeof(newtio));				/*Limpiamos struct para recibir los nuevos parametros del puerto.*/
	//cfsetispeed(&newtio, B115200);
	//cfsetospeed(&newtio, B115200);
	cfsetispeed(&newtio, B38400);
	cfsetospeed(&newtio, B38400);
	newtio.c_cflag |= (CREAD | CLOCAL);
	newtio.c_cflag &= ~PARENB;				/*None bit parity*/
	newtio.c_cflag &= ~CSTOPB;				/* 1 bit STOP */
	newtio.c_cflag &= ~CSIZE; 				/* Mask the character size bits */
	newtio.c_cflag |= CS8;    				/* Select 8 data bits */
	newtio.c_cflag &= ~CRTSCTS;				/*Disable flow control*/

	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);	/*Mode Raw input*/
	newtio.c_iflag |= IGNPAR;				/*Ignorar bytes con error de paridad*/
	newtio.c_iflag &= ~INPCK;				/* deshabiliar check de bit de paridad*/
	newtio.c_iflag &= ~(IXON | IXOFF | IXANY);		/*Disable Xon and Xoff*/
	newtio.c_oflag &= ~OPOST;				/* output in raw mode */

	newtio.c_cc[VTIME] = 15;				/* espera 2 seg.*/
	newtio.c_cc[VMIN] = TotalBytes;				/*tamaño de trama de datos ADQ*/

	tcsetattr(ID,TCSANOW, &newtio);
	return 1;
}

int ClosePort(void){
	tcsetattr(ID,TCSANOW, &oldtio);
	close(ID);
	return 1;
}

int TransmitCMD(char idCMD){
	int num_byte;

	bufferCMD[10] = idCMD;
	num_byte = write(ID, bufferCMD, 13);

	if(num_byte == 13)
		return 1;
	else
		return -1;
}

int ReceiveCMD(void){
	unsigned char bufferrx[41];
	int num_byte;
	int i;
	int flag;

	bufferRX[0] = 255;
	num_byte = read(ID, bufferrx, 13);
	for(i=0;i<10;i++){
		if((bufferrx[i]=='J') && (bufferrx[i+1]=='R') && (bufferrx[i+2]=='O')){
			bufferRX[1]=bufferrx[i+6];
			if (bufferrx[i+3] == 'C')
				bufferRX[0] = CMD;
			flag = 1;
			break;
		}
		flag = 0;
	}
	return flag;
}

int ReceiveString(void){
	unsigned char bufferrx[41];
	int num_byte;
	int i,j;
	int flag;

	bufferRX[0] = 255;
	num_byte = read(ID, bufferrx, 25);
	for(i=0; i<10; i++){
		if((bufferrx[i]=='J') && (bufferrx[i+1]=='R') && (bufferrx[i+2]=='O')){
			bufferRX[1]=bufferrx[i+6];
			if (bufferrx[i+3] == 'D'){
				bufferRX[0] = DAT;
				for(j=0;j<12;j++)
					bufferRX[j+2] = bufferrx[i+7+j];
			}
			flag = 1;
			break;
		}
		flag = 0;
	}
	return flag;
}

int ReceiveData(void){
	unsigned char bufferrx[255];
	int num_byte, FlagReset, num, j;
	
	num_byte = read(ID, bufferrx, TotalBytes);

	if(bufferrx[2] == 'J'){
		//Extraccion de datos de tiempo y fecha:
		for(j=0; j<12; j++)
			bufferTIME[j] = bufferrx[8 + j];
		//Extraccion de datos de canal X:
		for(j=0; j<20; j++)
			bufferX[j] = bufferrx[20 + j];
		//Extraccion de datos de canal Y:
		for(j=0; j<20; j++)
			bufferY[j] = bufferrx[40 + j];
		//Extraccion de datos de canal Z:
		for(j=0; j<20; j++)
			bufferZ[j] = bufferrx[60 + j];
		//Extraccion de datos de canal T1:
		for(j=0; j<2; j++)
			bufferT1[j] = bufferrx[80 + j];
		//Extraccion de datos de canal T2:
		for(j=0;j<2;j++)
			bufferT2[j] = bufferrx[82 + j];
		//Extraccion de datos de canal GND:
		for(j=0; j<2; j++)
			bufferGND[j] = bufferrx[84 + j];
	}else{
		//Dato incorrecto
		printf("Error al recibir datos.\nResetenando adquisicion...\n");
		// rutina para detener la adquisicion
		usleep(500000); // 0.5 segundos
		FlagReset = 1;
		while(FlagReset){
			num = TransmitCMD('5');
			if(num){
				printf("CMD5 sended.\n");
			}else{
				printf("CMD5 sended.\n");
			}
			usleep(500000); // 0.5 segundos

			num = TransmitCMD('0');
			if(num){
				printf("CMD0 sended.\n");
			}else{
				printf("CMD0 sended.\n");
			}
			num = ReceiveCMD();
			if(num){
				printf("CMD0 received : OK.\n");
				if((bufferRX[0] == CMD) && (bufferRX[1] == '0')){
					FlagReset = 0;
					printf("Digitizer Stoped.\n");
				}else{
					FlagReset = 1;
					printf("Digitizer No stoped.\n");
				}
			}else{
				printf("CMD0 received : Failed.\n");
				FlagReset = 1;
				usleep(500000); // 0.5 segundos
			}		
		}
	}

	return num_byte;
}

int PromData(void){
	int j;
	signed long int resul_tempX, resul_tempY, resul_tempZ, resul_tempT1, resul_tempT2, resul_tempGND;
	signed long int resulX[10];
	signed long int resulY[10];
	signed long int resulZ[10];
	
	//------------------------------------------------------------------
	//Datos promediados de X:
	for(j=0; j<10; j++){
		resul_tempX = bufferX[2*j + 1] + 256*bufferX[2*j];
		resul_tempY = bufferY[2*j + 1] + 256*bufferY[2*j];
		resul_tempZ = bufferZ[2*j + 1] + 256*bufferZ[2*j];
		
		if(resul_tempX > 32767){
			resul_tempX = resul_tempX - 65536;
		}
		if(resul_tempY > 32767){
			resul_tempY = resul_tempY - 65536;
			}
		if(resul_tempZ > 32767){
			resul_tempZ = resul_tempZ - 65536;
			}

		resulX[j] = resul_tempX;
		resulY[j] = resul_tempY;
		resulZ[j] = resul_tempZ;
	}

	resul_tempX = 0;
	resul_tempY = 0;
	resul_tempZ = 0;
	for(j=0; j<10; j++){
		resul_tempX = resul_tempX + resulX[j];
		resul_tempY = resul_tempY + resulY[j];
		resul_tempZ = resul_tempZ + resulZ[j];
	}
	valorX = resul_tempX/10;
	valorY = resul_tempY/10;
	valorZ = resul_tempZ/10;
	//------------------------------------------------------------------
	//------------------------------------------------------------------
	//------------------------------------------------------------------
	//Datos de T1:
	resul_tempT1 = bufferT1[1] + 256*bufferT1[0];
	if(resul_tempT1 > 32767){
		resul_tempT1 = resul_tempT1 - 65536;
	}
	valorTc = resul_tempT1;
	//------------------------------------------------------------------
	//Datos de T2:
	resul_tempT2 = bufferT2[1] + 256*bufferT2[0];
	if(resul_tempT2 > 32767){
		resul_tempT2 = resul_tempT2 - 65536;
	}
	valorTs = resul_tempT2;
	//------------------------------------------------------------------
	//Datos de GND:
	resul_tempGND = bufferGND[1] + 256*bufferGND[0];
	if(resul_tempGND > 32767){
		resul_tempGND = resul_tempT2 - 65536;
	}
	//------------------------------------------------------------------
	valorX = valorX - resul_tempGND;
	valorY = valorY - resul_tempGND;
	valorZ = valorZ - resul_tempGND;
	valorTc = valorTc - resul_tempGND;
	valorTs = valorTs - resul_tempGND;
	return 1;
}

int get_time(void){
	char tiempo_seg[10];

	time_t tiempo_complejo = time(0);
	struct tm *tlocal = localtime(&tiempo_complejo);
	strftime(tiempo, 30, "#20%y-%m-%d %H:%M:%S#", tlocal);
	
	strftime(tiempo_seg, 10, "%S", tlocal);
	current_second = atoi(tiempo_seg);
	return 1;
}

int CreateFileConf(void){
	FILE *setuplog;
	int num;
	setuplog = fopen(fileconf,"w");
	if(setuplog == NULL)
		return 0;
	else{
		num = get_time();
		fprintf(setuplog,"%cLISN Magnetometer logging setup file Ver.24.07.17%c\n",34,34);
		if (num)
			fprintf(setuplog,"%cUpdated : %c,%s\n\n",34,34,tiempo);
		else
			fprintf(setuplog,"%cUpdated : %c,2023-07-20 00:00:00\n\n",34,34);
		fprintf(setuplog,"%cNo borrar las comillas ni las comas.%c\n",34,34);
		fprintf(setuplog,"%cSirven para separar tipos de datos%c\n\n",34,34);
		
		fprintf(setuplog,"%cStation                 %c,%cSTATION%c\n",34,34,34,34);
		fprintf(setuplog,"%cIAGA code               %c,%cSTA%c\n",34,34,34,34);
		fprintf(setuplog,"%cMagnetometer            %c,%cMAGLISN-XX%c\n",34,34,34,34);
		fprintf(setuplog,"%cLatitude                %c,%cxx°xx'xx%c\n",34,34,34,34);
		fprintf(setuplog,"%cLongitude               %c,%cyy°yy'yy%c\n",34,34,34,34);
		fprintf(setuplog,"%cAltitude(m)             %c,%c100%c\n",34,34,34,34);
		fprintf(setuplog,"%cttySX Number Port       %c,%cttyS1%c\n",34,34,34,34);
		fprintf(setuplog,"%cSet interval time (min) %c,%c3%c\n",34,34,34,34);
		fprintf(setuplog,"%cEnable GPS              %c,%cno%c\n\n",34,34,34,34);
		
		fprintf(setuplog,"%cValores de calibracion instrumental%c\n",34,34);
		fprintf(setuplog,"%cScaleH value(mV/nT)     %c,20.0\n",34,34);
		fprintf(setuplog,"%cScaleD value(mV/nT)     %c,20.0\n",34,34);
		fprintf(setuplog,"%cScaleZ value(mV/nT)     %c,20.0\n\n",34,34);
		
		fprintf(setuplog,"%cOffset Electronico%c\n",34,34);
		fprintf(setuplog,"%cOffsetH(v)              %c,0.0\n",34,34);
		fprintf(setuplog,"%cOffsetD(v)              %c,0.0\n",34,34);
		fprintf(setuplog,"%cOffsetZ(v)              %c,0.0\n",34,34);
		fprintf(setuplog,"%cOffsetTs(v)             %c,0.0\n",34,34);
		fprintf(setuplog,"%cOffsetTc(v)             %c,0.0\n\n",34,34);
		
		fprintf(setuplog,"%cLinea de base, valor de referencia del lugar (IGRF)%c\n",34,34);
		fprintf(setuplog,"%cLBaseH(nT)              %c,24854\n",34,34);
		fprintf(setuplog,"%cLBaseD(Grados)          %c,-3.855\n",34,34);
		fprintf(setuplog,"%cLBaseZ(nT)              %c,-840\n\n",34,34);
		
		fprintf(setuplog,"%cValor promedio anual para la epoca 2015%c\n",34,34);
		fprintf(setuplog,"%cHmean(nT)               %c,24870\n",34,34);
		fprintf(setuplog,"%cDmean(Grados)           %c,-2.4\n",34,34);
		fprintf(setuplog,"%cZmean(nT)               %c,-85\n\n",34,34);

		fprintf(setuplog,"%cConstantes de ajuste lineal%c\n",34,34);
		fprintf(setuplog,"%ccte1 Pendiente          %c,0.305185095\n",34,34);
		fprintf(setuplog,"%ccte2 Intersecto         %c,0.000000001\n\n",34,34);	

		fprintf(setuplog,"%cConstantes de ajuste por temperatura (25 °C)%c\n",34,34);
		fprintf(setuplog,"%cTemp reference(mv)      %c,250\n",34,34);
		fprintf(setuplog,"%cth factor(mv)           %c,0.0\n",34,34);	
		fprintf(setuplog,"%ctz facor(mv)            %c,0.0\n\n",34,34);

		fprintf(setuplog,"%cRuta de almacenamiento local (Fijo, no alterar)%c\n",34,34);
		fprintf(setuplog,"%cPathD datos             %c,%c/mnt/usb_flash/magnet%c\n",34,34,34,34);
		fprintf(setuplog,"%cPathS segundos          %c,%c/mnt/usb_flash/magnet/dataseg%c\n",34,34,34,34);
		fprintf(setuplog,"%cPathM minutos           %c,%c/mnt/usb_flash/magnet/datamin%c\n",34,34,34,34);
		fprintf(setuplog,"%cPathZ zipdata           %c,%c/mnt/usb_flash/magnet/datazip%c\n",34,34,34,34);
		fprintf(setuplog,"%cRuta de almacenamiento remoto (Cambiar segun usuario)%c\n",34,34);
		fprintf(setuplog,"%cServer1 Path            %c,%c/home/cesar/isr/magnetometer/test/jica%c\n",34,34,34,34);
		fprintf(setuplog,"%cServer2 Path            %c,%c/home/ricardo/datos/magnet/datamin%c\n",34,34,34,34);
		fprintf(setuplog,"%cUSB Device              %c,%c/dev/sda1%c\n",34,34,34,34);
		fclose(setuplog);
	}
	return 1;
}


int ReadFileConf(void){

	//Varible para archivo
	FILE *setuplog;
	char buffer_line[250];
	int conta_line;
	char *pointer;
	int numchar,i;
	char buffer_string[20];

	setuplog = fopen(fileconf,"r");
	if(setuplog == NULL)
		return 0;
	else{
		//Lectura de archivo:
		conta_line = 0;
		while(fgets(buffer_line,sizeof(buffer_line),setuplog) != NULL){
			conta_line = conta_line + 1;
			switch(conta_line){
				case 7://Station
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						station[i-2] = pointer[i];
					station[i-2] = '\0';
					break;

				case 8://IAGA CODE
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						IAGA[i-2] = tolower(pointer[i]);
					IAGA[i-2] = '\0';
					break;

				case 9://magnetometer
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						magnetometer[i-2] = pointer[i];
					magnetometer[i-2] = '\0';
					break;

				case 10://latitude
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						latitude[i-2] = pointer[i];
					latitude[i-2] = '\0';
					break;

				case 11://longitude
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						longitude[i-2] = pointer[i];
					longitude[i-2] = '\0';
					break;

				case 14://Set send file time
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						buffer_string[i-2] = pointer[i];
					buffer_string[i-2] = '\0';
					timeset = atoi(buffer_string);
					break;

				case 15://enableGPS
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						enableGPS[i-2] = pointer[i];
					enableGPS[i-2] = '\0';
					break;
				
				case 18://ScaleH
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					ScaleH = atof(buffer_string);
					break;

				case 19://ScaleD
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					ScaleD = atof(buffer_string);
					break;

				case 20://ScaleZ
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					ScaleZ = atof(buffer_string);
					break;

				case 23://OffsetH
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					OffsetH = atof(buffer_string);
					break;

				case 24://OffsetD
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					OffsetD = atof(buffer_string);
					break;

				case 25://OffsetZ
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					OffsetZ = atof(buffer_string);
					break;

				case 26://OffsetT1 = Tsensor
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					OffsetTs = atof(buffer_string);
					break;

				case 27://OffsetT2 = tcontrol
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					OffsetTc = atof(buffer_string);
					break;

				case 30://LBaseH
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					LBaseH = atof(buffer_string);
					break;

				case 31://LBaseD
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					LBaseD = atof(buffer_string);
					break;

				case 32://LBaseZ
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					LBaseZ = atof(buffer_string);
					break;

				case 35://Hmean
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					Hmean = atof(buffer_string);
					break;

				case 36://Dmean
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					Dmean = atof(buffer_string);
					break;

				case 37://Dmean
					pointer = (char *)memchr(buffer_line,',',79);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					Zmean = atof(buffer_string);
					break;

				case 40://cte1
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					cte1 = atof(buffer_string);
					break;

				case 41://cte2
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					cte2 = atof(buffer_string);
					break;

				case 44://TempRef
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					TempRef = atof(buffer_string);
					break;

				case 45://thFactor
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					thFactor = atof(buffer_string);
					break;

				case 46://tzFactor
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 1;
					for(i=1;i<numchar;i++)
						buffer_string[i-1] = pointer[i];
					buffer_string[i-1] = '\0';
					tzFactor = atof(buffer_string);
					break;

				case 49://pathdata
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						pathdata[i-2] = pointer[i];
					pathdata[i-2] = '\0';
					break;

				case 50://pathseg
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						pathseg[i-2] = pointer[i];
					pathseg[i-2] = '\0';
					break;

				case 51://pathmin
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						pathmin[i-2] = pointer[i];
					pathmin[i-2] = '\0';
					break;

				case 52://pathzip
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						pathzip[i-2] = pointer[i];
					pathzip[i-2] = '\0';
					break;

				case 54://pathserver1
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						pathserver1[i-2] = pointer[i];
					pathserver1[i-2] = '\0';
					break;

				case 55://pathserver2
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						pathserver2[i-2] = pointer[i];
					pathserver2[i-2] = '\0';
					break;

				case 56://USB Device
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						USBdevice[i-2] = pointer[i];
					USBdevice[i-2] = '\0';
					break;
			}
		}
		fclose(setuplog);
	}
	return 1;
}

int UpdateDir(char dir[]){
	int e;
	int flag_dir;
	struct stat sb;

	flag_dir = 0;
	e = stat(dir, &sb);
	if (e == 0){
		if (sb.st_mode & S_IFDIR){
			//printf("Existe directorio %s.\n",dir);
			flag_dir = 1;		
		}
	}else{
		if (errno == ENOENT){
			printf("The directory does not exist. Creating new directory...\n");
			e = mkdir(dir, S_IRWXU);
			if (e != 0){
				printf("mkdir failed; errno=%d\n",errno);
				flag_dir = 0;
			}else{
				printf("created the directory %s\n",dir);
				flag_dir = 1;
			}
		}
	}
	return flag_dir;
}

int InitADC(void){
    printf("No se realiza configuracion del digitalizador!\n");
	return 1;
}

int FechaHora(void){
	char char_hh[10],char_mm[10],char_ss[10],char_numday[10];
	char char_year[10], char_mes[10], char_dia[10];
	char string_mes[10],string_numday[10];
	int int_year,int_mes,int_dia,resto,i,numday;
	int nday[12]={0,31,28,31,30,31,30,31,31,30,31,30};

	time_t tiempo_complejo = time(0);
	struct tm *tlocal = localtime(&tiempo_complejo);
	//day current:
	strftime(char_dia,10,"%d",tlocal);
	strftime(char_mes,10,"%m",tlocal);
	strftime(char_year,10,"%y",tlocal);
	strcpy(yearSYS,char_year);
	
	int_year = atoi(char_year);
	int_mes = atoi(char_mes);
	int_dia = atoi(char_dia);

	//time current
	strftime(char_hh,10,"%H",tlocal);
	strftime(char_mm,10,"%M",tlocal);
	strftime(char_ss,10,"%S",tlocal);

	hh_SYS = atoi(char_hh);
	mm_SYS = atoi(char_mm);
	ss_SYS = atoi(char_ss);

	//calculo de string_mes:
	switch(int_mes){
			case 1:
				strcpy(string_mes,"");
				strcat(string_mes,"jan");
				break;
			case 2:
				strcpy(string_mes,"");
				strcat(string_mes,"feb");
				break;
			case 3:
				strcpy(string_mes,"");
				strcat(string_mes,"mar");
				break;
			case 4:
				strcpy(string_mes,"");
				strcat(string_mes,"apr");
				break;
			case 5:
				strcpy(string_mes,"");
				strcat(string_mes,"may");
				break;
			case 6:
				strcpy(string_mes,"");
				strcat(string_mes,"jun");
				break;
			case 7:
				strcpy(string_mes,"");
				strcat(string_mes,"jul");
				break;
			case 8:
				strcpy(string_mes,"");
				strcat(string_mes,"aug");
				break;
			case 9:
				strcpy(string_mes,"");
				strcat(string_mes,"sep");
				break;
			case 10:
				strcpy(string_mes,"");
				strcat(string_mes,"oct");
				break;
			case 11:
				strcpy(string_mes,"");
				strcat(string_mes,"nov");
				break;
			case 12:
				strcpy(string_mes,"");
				strcat(string_mes,"dec");
				break;
		}
	//calculo de string_numday:
	resto = int_year % 4;
	if(resto==0)
		nday[2] = 29; //año biciesto:
	numday = 0;
	for(i=0;i < int_mes;i++)
		numday = numday + nday[i];
	numday = numday + int_dia;
	sprintf(char_numday,"%d",numday);
	if(numday<10){
		string_numday[0] = '0';
		string_numday[1] = '0';
		string_numday[2] = char_numday[0];
		string_numday[3] = '\0';
	}else if(numday<100){
		string_numday[0] = '0';
		string_numday[1] = char_numday[0];
		string_numday[2] = char_numday[1];
		string_numday[3] = '\0';
	}else if(numday<1000){
		string_numday[0] = char_numday[0];
		string_numday[1] = char_numday[1];
		string_numday[2] = char_numday[2];
		string_numday[3] = '\0';
	}
	//Copia del valor de numday
	strcpy(copy_numday,string_numday);
	//printf("STRING : %s\n",copy_numday);

	//Para archivos de minutos:
	strcpy(fecha1,"");
	strcat(fecha1,IAGA);
	strcat(fecha1,char_dia);
	strcat(fecha1,string_mes);
	strcat(fecha1,".");
	strcat(fecha1,char_year);

	//Para archivos de segundos:
	strcpy(fecha2,"");
	strcat(fecha2,IAGA);
	strcat(fecha2,string_numday);
	strcat(fecha2,char_hh);
	strcat(fecha2,".");
	strcat(fecha2,char_year);

	//Para archivos zipiados:
	strcpy(fechazip,"");
	strcat(fechazip,IAGA);
	strcat(fechazip,string_numday);
	strcat(fechazip,char_hh);
	strcat(fechazip,".gz");

	//Para carpeta de segundos:
	strcpy(fecha3,"");
	strcat(fecha3,char_dia);
	strcat(fecha3,char_mes);
	strcat(fecha3,"20");	
	strcat(fecha3,char_year);

	//Para mostrar hora archivos de segundos:
	strcpy(fecha4,"");
	strcat(fecha4,char_hh);
	strcat(fecha4," ");
	strcat(fecha4,char_mm);
	strcat(fecha4," ");
	strcat(fecha4,char_ss);

	//Para mostrar hora archivos de minutos:
	strcpy(fecha5,"");
	strcat(fecha5,char_dia);
	strcat(fecha5," ");
	strcat(fecha5,char_mes);
	strcat(fecha5," 20");
	strcat(fecha5,char_year);
	strcat(fecha5," ");
	strcat(fecha5,char_hh);
	strcat(fecha5," ");
	strcat(fecha5,char_mm);
	return 1;
}

int CreateFileSeg(void){
	signed long int valorXb, valorYb, valorZb, valorT1b, valorT2b;
	int num,flagfile;	
	FILE *fp1;
	FILE *fp2;

	//verificacion de existenia de carpeta:
	flagfile = 0;
	strcpy(filesseg, pathseg);
	strcat(filesseg, "/");
	strcat(filesseg, fecha3);
	num = UpdateDir(filesseg);
	if(num){
		strcat(filesseg, "/");
		strcat(filesseg, fecha2);
		strcat(filesseg, "s");

		fp1 = fopen(filesseg, "r");
		if(fp1 == NULL){
			printf("Archivo: %s NO existe. Creando ...\n",filesseg);
			flagfile = 1;
		}else{
			fclose(fp1);
		}

		if(flagfile == 1){
			fp2 = fopen(filesseg,"w");
			if(fp2 == NULL){
				flagfile = 0;
				return 0;
			}else{
				fprintf(fp2,"%s %s <%s> 1 Sec. Raw data\n", station, magnetometer, copy_numday);
				fprintf(fp2,"HH MM SS   ch(H)  ch(D)  ch(Z)  ch(Tc)  ch(Ts)\n\n");
			}
			fclose(fp2);
		}

		//Agregando contenido de datos al archivo
		fp1 = fopen(filesseg, "a");
		valorXb = valorX;
		valorYb = valorY;
		valorZb = valorZ;
		valorT1b = valorTc;
		valorT2b = valorTs;
		fprintf(fp1,"%s  %+06li %+06li %+06li %+06li %+06li\n",fecha4, valorXb, valorYb, valorZb, valorT1b, valorT2b);
		fclose(fp1);

		//Incrementa contador de segundos:
		conta_seg = conta_seg + 1;
		sumX = sumX + valorXb;
		sumY = sumY + valorYb;
		sumZ = sumZ + valorZb;
		sumTc = sumTc + valorT1b;
		sumTs = sumTs + valorT2b;
	}else{
		fp2 = fopen(filelog,"a");
		if(fp2 == NULL)
			printf("No se pudo crear historial.log\n");
		else{
			num = get_time();
			fprintf(fp2,"%cFunction UpdateDir(pathseg/fecha3) failed at : %c,%s\n",34,34,tiempo);
			fclose(fp2);
			return 0;
		}
	}
	return 1;
}

int CreateFileMinv(void){
	int flagfile,num;
	double mvTc, mvTs;
	double tch,tcz;
	FILE *fp1;
	FILE *fp2;

	//verificacion de existenia de carpeta:
	flagfile = 0;
	num = UpdateDir(pathmin);
	if(num){
		strcpy(filesminv, pathmin);
		strcat(filesminv, "/");
		strcat(filesminv, fecha1);
		strcat(filesminv,"v");

		fp1 = fopen(filesminv,"r");
		if(fp1 == NULL){
			printf("Archivo: %s NO existe. Creando ...\n",filesminv);
			flagfile = 1;
		}else{
			fclose(fp1);
		}
		

		if(flagfile == 1){
			fp2 = fopen(filesminv,"w");
			if(fp2 == NULL){
				flagfile = 0;
				return 0;
			}else{
				fprintf(fp2,"%s %s <%s> 1 Min. Reported data\n\n",station,magnetometer,copy_numday);
				fprintf(fp2," DD MM YYYY  HH MM   X(mv)   Y(mv)   Z(mv)   Tc(mv)   Ts(mv)\n\n");
			}
			fclose(fp2);
		}
		//Agregando contenido de datos al archivo:
		fp1 = fopen(filesminv,"a");
		mvTc = (sumTc*cte1 + cte2) + OffsetTc;
		tch = thFactor*(TempRef - mvTc);
		tcz = tzFactor*(TempRef - mvTc);
		mvX = (sumX*cte1 + cte2) + OffsetH + tch;
		mvZ = (sumZ*cte1 + cte2) + OffsetZ + tcz;
		mvY = (sumY*cte1 + cte2) + OffsetD;
		mvTs = (sumTs*cte1 + cte2) + OffsetTs;
		fprintf(fp1," %s  %+010.4f %+010.4f %+010.4f %+010.4f %+010.4f\n", fecha5, mvX, mvY, mvZ, mvTc, mvTs);
		fclose(fp1);
	}else{
		fp2 = fopen(filelog,"a");
		if(fp2 == NULL)
			printf("No se pudo crear historial.log\n");
		else{
			num = get_time();
			fprintf(fp2,"%cFunction UpdateDir(pathmin) failed at : %c,%s\n",34,34,tiempo);
			fclose(fp2);
			return 0;
		}
	}
	return 1;	
}

int CreateFileMinm(void){
	double datachannelX, datachannelY, datachannelZ, datachannelI, datachannelF;
	int flagfile,num;
	FILE *fp1;
	FILE *fp2;

	//verificacion de existenia de carpeta:
	flagfile = 0;
	num = UpdateDir(pathmin);
	if(num){
		strcpy(filesminm, pathmin);
		strcat(filesminm, "/");
		strcat(filesminm, fecha1);
		strcat(filesminm, "m");
		fp1 = fopen(filesminm, "r");
		if(fp1 == NULL){
			printf("Archivo: %s NO existe. Creando ...\n",filesminm);
			flagfile = 1;
		}else{
			fclose(fp1);
		}

		if(flagfile == 1){
			fp2 = fopen(filesminm, "w");
			if(fp2 == NULL){
				flagfile = 0;
				return 0;
			}else{
				fprintf(fp2,"%s %s <%s> 1 Min. Reported data\n\n",station,magnetometer,copy_numday);
				fprintf(fp2," DD MM YYYY  HH MM   D(Deg)  H(nt)   Z(nt)   I(Deg)   F(nt)\n\n");
			}
			fclose(fp2);
		}
		//Agregando contenido de datos al archivo:
		fp1 = fopen(filesminm,"a");
		datachannelX = LBaseH + mvX/ScaleH;
		datachannelY = LBaseD + (mvY/ScaleD)*(3438/(Hmean*60));
		datachannelZ = LBaseZ + mvZ/ScaleZ;
		datachannelI = atan2(datachannelZ,datachannelX)*(180/3.1416);
		datachannelF = sqrt(pow(datachannelX,2) + pow(datachannelZ,2));
		fprintf(fp1," %s  %+08.4f %+08.1f %+08.1f %+08.4f %+08.1f\n", fecha5, datachannelY, datachannelX, datachannelZ, datachannelI, datachannelF);
		fclose(fp1);
	}else{
		fp2 = fopen(filelog,"a");
		if(fp2 == NULL)
			printf("No se pudo crear historial.log\n");
		else{
			num = get_time();
			fprintf(fp2,"%cFunction UpdateDir(pathmin) failed at : %c,%s\n",34,34,tiempo);
			fclose(fp2);
			return 0;
		}
	}
	return 1;	
}

int CreateFileFg(void){
	FILE *fp;
	int num;
	
	fp = fopen(filefg,"w");
	if(fp == NULL){
		return 0;
	}else{
		num = get_time();
		fprintf(fp,"Current FG files list ready for ZIP and FTP\n");
		fprintf(fp,"Updated : %s\n",tiempo);
		fprintf(fp,"File seg : %s\n",filesseg);
		fprintf(fp,"File min : %s\n",filesminm);
		fprintf(fp,"File zip : %s\n",outzip);
		fprintf(fp,"Path server1 : %s\n",pathserver1);
		fprintf(fp,"Path server2 : %s\n",pathserver2);
		fclose(fp);
	}
	return 1;
}
