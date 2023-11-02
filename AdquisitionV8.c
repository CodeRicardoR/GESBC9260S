/*
 * AdquisitionV8.c
 *
 *  Created on: Oct 18, 2023
 *      Author: Ricardo Rojas Quispe
 */

/*	
	Se trabajo con el codigo fuente de AdquisitionV7.c como referencia.
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
	6. 18/10/2023 => Se cambio a nuevo Firmware de adquisicion 5 Sps 16bits (+/- 10V), 8 canales [AUX0, AUX1, AUX2]
*/

#include <stdio.h>   /* Standard input/output definitions */
#include <stdlib.h>
#include <stdbool.h>
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
#define TotalBytes 25		//Trana de bytes de datos "JROADQ...."

//Prototipo de funciones:
int OpenPort(void);
int ClosePort(void);
bool SendCMD(char);
bool ReceiveDAT(char);
int get_time(void);
int CreateFileConf(void);
int ReadFileConf(void);
int UpdateDir(char*);
int InitADC(void);
int FechaHora(void);
int CreateFileSeg(void);
int CreateFileMinv(void);
int CreateFileMinm(void);
int CreateFileFg(void);
int UpdateSetupFile(char*);

//Variables Globales:
int statusGPS;

//Buffers:
//CMD0 [10] = IdCmd (ASCII)
static unsigned char bufferCMD[14] = {165,165,165,165,74,82,79,67,77,68,48,10,13,'\0'};

//DAT
static unsigned char bufferDAT[13];
//[hhmmssdmmyy]
static unsigned char bufferTIME[30];

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

signed long int valorAUX0, valorAUX1, valorAUX2;
signed long int sumAUX0, sumAUX1, sumAUX2;

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
bool DataReady = false;

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
int num, i, conta_line;
int flagfile, flagDAT;
int grados, minutos, segundos;
int contador;
int last_second;
int FlagSetTime;
bool FlagReset;

char latitudGPS[30],longitudGPS[30];
char DataBuffer[30];
char fileconf2[250];
char cmd_line[250];
char comillas[2]={34,'\0'};
char num_GPS[4];

char *puntero;

//Temporal variables
signed long int ValueX, ValueY, ValueZ, ValueTS, ValueTC;
signed long int TempX, TempY, TempZ, TempTS, TempTC; 

signed long int ValueAUX0, ValueAUX1, ValueAUX2; 
signed long int TempAUX0, TempAUX1, TempAUX2;


bool EnaStream = false;

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
		FlagReset = false;
		contador = 0;
		while(SendCMD('5') != true){
			usleep(1300000); // 1.3 segundos
			printf("Stoping Digitizer\n");
			contador++;
			if(contador > 50){
				printf("Digitizer failed!...\n");
				return 0;//Fin de programa
			}
		}
		FlagReset = true;
		usleep(1000000); // 1 segundos
		if(FlagReset & SendCMD('0')){
			//DAT0 - version
			if(ReceiveDAT('0')){
				bufferDAT[3] = '\0';
				printf("DAT0 - Ver: %s\n", bufferDAT);
			}
			printf("Digitizer OK\n");
		}else{
			printf("Digitizer failed!...\n");
			return 0;//Fin de programa
		}
		ClosePort();
		usleep(1000000); // 1 segundos
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
					strcpy(cmd_line,"");
					contador = 0;
					while(fgets(cmd_line,sizeof(cmd_line),f_temp) != NULL){
						puntero = strstr( cmd_line, "/mnt/usb_flash");
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
		}

		//Actualizacionb de latitud y longitud de setuplog.cfg
		if (statusGPS == 1){
			// Envio de CMD8 para sincronizacion con digitalizador:
			flagDAT = 0;
			if(SendCMD('8')){
				if(ReceiveDAT('8')){
					printf("DAT8: %s\n", bufferDAT);
					for(contador=0; contador<12; contador++){
						if(bufferDAT[contador] != 'X'){
							bufferTIME[contador] = bufferDAT[contador];
							flagDAT = 1;
						}else{
							flagDAT = 0;
							break;
						}
					}
				}
			}

			if(flagDAT == 1){
				printf("Time GPS OK => %s.\n", bufferTIME);
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
			
			}else{
				printf("Time GPS Failed => %s.\n", bufferDAT);
				//****************************Actualizacion de hora UTC con servidor de LISN
				strcpy(cmd_line,"/home/magnet/UpdateTime");
				printf("cmd_line : %s\n",cmd_line);
				f_temp = popen(cmd_line,"r");
				while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
				}
				pclose(f_temp);
			}

			// Envio de CMD1 para obtener datos de latiud de GPS:
			if(SendCMD('1')){
				if(ReceiveDAT('1')){
					printf("DAT1: %s\n", bufferDAT);
					for(contador=0; contador<7; contador++){
						latitudGPS[contador] = bufferDAT[contador];
					}
				}
			}

			// Envio de CMD2 para obtener datos de longitud de GPS:
			if(SendCMD('2')){
				if(ReceiveDAT('2')){
					printf("DAT2: %s\n", bufferDAT);
					for(contador=0; contador<8; contador++){
						longitudGPS[contador] = bufferDAT[contador];
					}
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
			}else{
				//Habilitación de conversion continuo
				EnaStream = true;
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
				while(fgets(cmd_line, sizeof(cmd_line), file1) != NULL){
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
						fprintf(file2, "%d°%d'%d%c\n", grados, minutos, segundos, 34);
						printf("Latitude: %d°%d'%d\n",grados, minutos, segundos);

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
						printf("Longitude: %d°%d'%d\n", grados, minutos, segundos);

					}else
						fprintf(file2,"%s", cmd_line);					
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
			}else{
				//Habilitación de conversion continuo
				EnaStream = true;
			}					
		}else{
			printf("ERROR Enable GPS: yes or no?\n");
			return 0; //fin de programa
		}
		num = FechaHora();
		
		//Actualización de carpeta de datos (default)
		num = UpdateDir(dirdata);
		//Actualizacion de Setuplog.cfg
		num = UpdateSetupFile(yearSYS);

		//Actualizacion de carpeta de year
		num = UpdateDir(pathdata);
		//Actualización de carpeta de datos de segundos
		num = UpdateDir(pathseg);
		//Actualización de carpeta de datos de minutos
		num = UpdateDir(pathmin);
		//Actualización de carpeta de datos zipiados
		num = UpdateDir(pathzip);

		//Lectura de datos digitalizados
		DataReady = false;
		if(statusGPS == 1){
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
			//Inicializacion de variables:
			conta_seg = 0;
			conta_min = 0;
			sumX = 0;
			sumY = 0;
			sumZ = 0;
			sumTs = 0;
			sumTc = 0;
			sumAUX0 = 0;
			sumAUX1 = 0;
			sumAUX2 = 0;
			
			printf("Starting receive data...\n");
			num = FechaHora();
			last_second = ss_SYS;
			while(last_second == ss_SYS){
				num = FechaHora();
				usleep(100000);
			}
			last_second = ss_SYS;
			//Inicio de lectura de datos
			if(SendCMD('4')){
				while(EnaStream){
					TempX = 0;
					TempY = 0;
					TempZ = 0;
					TempTC = 0;
					TempTS = 0;
					TempAUX0 = 0;
					TempAUX1 = 0;
					TempAUX2 = 0;
					//Data Time "G-JRODATE[12 bytes]\n\r"
					num = read(ID, DataBuffer, 23);
					for(contador=0; contador<12; contador++){
						bufferTIME[contador] = DataBuffer[contador + 9];
					}

					for(contador=0; contador<5; contador++){
						num = read(ID, DataBuffer, TotalBytes);
						ValueX = DataBuffer[7] + 256*DataBuffer[8];
						ValueY = DataBuffer[9] + 256*DataBuffer[10];
						ValueZ = DataBuffer[11] + 256*DataBuffer[12];
						ValueTC = DataBuffer[13] + 256*DataBuffer[14];
						ValueTS = DataBuffer[15] + 256*DataBuffer[16];
						ValueAUX0 = DataBuffer[17] + 256*DataBuffer[18];
						ValueAUX1 = DataBuffer[19] + 256*DataBuffer[20];
						ValueAUX2 = DataBuffer[21] + 256*DataBuffer[22];

						if(ValueX > 32767){
							ValueX = ValueX - 65536;
						}
						if(ValueY > 32767){
							ValueY = ValueY - 65536;
						}
						if(ValueZ > 32767){
							ValueZ = ValueZ - 65536;
						}
						if(ValueTC > 32767){
							ValueTC = ValueTC - 65536;
						}
						if(ValueTS > 32767){
							ValueTS = ValueTS - 65536;
						}
						if(ValueAUX0 > 32767){
							ValueAUX0 = ValueAUX0 - 65536;
						}
						if(ValueAUX1 > 32767){
							ValueAUX1 = ValueAUX1 - 65536;
						}
						if(ValueAUX2 > 32767){
							ValueAUX2 = ValueAUX2 - 65536;
						}

						TempX = TempX + ValueX;
						TempY = TempY + ValueY;
						TempZ = TempZ + ValueZ;
						TempTC = TempTC + ValueTC;
						TempTS = TempTS + ValueTS;
						TempAUX0 = TempAUX0 + ValueAUX0;
						TempAUX1 = TempAUX1 + ValueAUX1;
						TempAUX2 = TempAUX2 + ValueAUX2;
					}
					valorX = TempX/5;
					valorY = TempY/5;
					valorZ = TempZ/5;
					valorTc = TempTC/5;
					valorTs = TempTS/5;
					valorAUX0 = TempAUX0/5;
					valorAUX1 = TempAUX1/5;
					valorAUX2 = TempAUX2/5;
					DataReady = true;

					//printf("time : %s  ",fecha4);
					num = FechaHora();
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
						sumAUX0 = sumAUX0/conta_seg;
						sumAUX1 = sumAUX1/conta_seg;
						sumAUX2 = sumAUX2/conta_seg;

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
						sumAUX0 = 0;
						sumAUX1 = 0;
						sumAUX2 = 0;
						conta_min = conta_min + 1;
					}

					if(timeset == conta_min){
						if(ss_SYS == 5){
							//-------------------------------------------------------------------------------------
							//Envio de archivo min-M, server 1:
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
							f_temp = popen(cmd_line,"r");
							while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
							}
							pclose(f_temp);

							//Envio de archivo min-M, server 2:
							strcpy(cmd_line, "python ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/SendfileFTP.py ");
							strcat(cmd_line, pathserver2);
							strcat(cmd_line, " ");
							strcat(cmd_line, filesminm);
							strcat(cmd_line, " ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/lisn_logon2.txt");
							strcat(cmd_line, " >/dev/null 2>&1 &");
							f_temp = popen(cmd_line,"r");
							while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
							}
							pclose(f_temp);
							//-------------------------------------------------------------------------------------

							//-------------------------------------------------------------------------------------
							//Envio de archivo min-V, server 1:
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
							f_temp = popen(cmd_line,"r");
							while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
							}
							pclose(f_temp);

							//Envio de archivo min-V, server 2:
							strcpy(cmd_line, "python ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/SendfileFTP.py ");
							strcat(cmd_line, pathserver2);
							strcat(cmd_line, " ");
							strcat(cmd_line, filesminv);
							strcat(cmd_line, " ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/lisn_logon2.txt");
							strcat(cmd_line, " >/dev/null 2>&1 &");
							f_temp = popen(cmd_line,"r");
							while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
							}
							pclose(f_temp);
							//-------------------------------------------------------------------------------------
							printf("Minute V-M files FTP sent...\n");

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
						}
					}
					if(hh_SYS == 23){
						if(mm_SYS == 50){
							if(ss_SYS == 50){
								//Actualizando fecha y hora del sistema
								FlagSetTime = 1;
								for(i=0; i<12; i++){
									if (bufferTIME[i + 9] == 'X'){
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
					}
				}
				//Rutina to stop digitizer:
				while(SendCMD('5') != true){
					usleep(700000); // 15 milisegundos
					printf("Stoping Digitizer\n");
				}
				EnaStream = false;
				DataReady = false;
				printf("Finish receive data.\n");
				ClosePort();
				return 0; //fin de programa
			}else
				printf("Error al recibir datos CMD4!\n");
				
		}else if (statusGPS == 0){
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

			//Inicializacion de variables:
			conta_seg = 0;
			conta_min = 0;
			sumX = 0;
			sumY = 0;
			sumZ = 0;
			sumTs = 0;
			sumTc = 0;
			sumAUX0 = 0;
			sumAUX1 = 0;
			sumAUX2 = 0;

			printf("Starting receive data...in 1 seconds\n");
			//Waiting for change of one second
			num = FechaHora();
			last_second = ss_SYS;
			while(last_second == ss_SYS){
				num = FechaHora();
				usleep(100000);
			}
			last_second = ss_SYS;
			while(EnaStream){
				if(SendCMD('9')){
					TempX = 0;
					TempY = 0;
					TempZ = 0;
					TempTC = 0;
					TempTS = 0;
					TempAUX0 = 0;
					TempAUX1 = 0;
					TempAUX2 = 0;
					//Data Time "XXXXXX..."
					num = read(ID, DataBuffer, 23);
					for(contador=0; contador<5; contador++){
						num = read(ID, DataBuffer, TotalBytes);
						ValueX = DataBuffer[7] + 256*DataBuffer[8];
						ValueY = DataBuffer[9] + 256*DataBuffer[10];
						ValueZ = DataBuffer[11] + 256*DataBuffer[12];
						ValueTC = DataBuffer[13] + 256*DataBuffer[14];
						ValueTS = DataBuffer[15] + 256*DataBuffer[16];
						ValueAUX0 = DataBuffer[17] + 256*DataBuffer[18];
						ValueAUX1 = DataBuffer[19] + 256*DataBuffer[20];
						ValueAUX2 = DataBuffer[21] + 256*DataBuffer[22];

						if(ValueX > 32767){
							ValueX = ValueX - 65536;
						}
						if(ValueY > 32767){
							ValueY = ValueY - 65536;
						}
						if(ValueZ > 32767){
							ValueZ = ValueZ - 65536;
						}
						if(ValueTC > 32767){
							ValueTC = ValueTC - 65536;
						}
						if(ValueTS > 32767){
							ValueTS = ValueTS - 65536;
						}
						if(ValueAUX0 > 32767){
							ValueAUX0 = ValueAUX0 - 65536;
						}
						if(ValueAUX1 > 32767){
							ValueAUX1 = ValueAUX1 - 65536;
						}
						if(ValueAUX2 > 32767){
							ValueAUX2 = ValueAUX2 - 65536;
						}

						TempX = TempX + ValueX;
						TempY = TempY + ValueY;
						TempZ = TempZ + ValueZ;
						TempTC = TempTC + ValueTC;
						TempTS = TempTS + ValueTS;
						TempAUX0 = TempAUX0 + ValueAUX0;
						TempAUX1 = TempAUX1 + ValueAUX1;
						TempAUX2 = TempAUX2 + ValueAUX2;
					}
					valorX = TempX/5;
					valorY = TempY/5;
					valorZ = TempZ/5;
					valorTc = TempTC/5;
					valorTs = TempTS/5;
					valorAUX0 = TempAUX0/5;
					valorAUX1 = TempAUX1/5;
					valorAUX2 = TempAUX2/5;
					DataReady = true;

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
						sumAUX0 = sumAUX0/conta_seg;
						sumAUX1 = sumAUX1/conta_seg;
						sumAUX2 = sumAUX2/conta_seg;

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
						sumAUX0 = 0;
						sumAUX1 = 0;
						sumAUX2 = 0;
						conta_min = conta_min + 1;
					}

					if(timeset == conta_min){
						if(ss_SYS == 5){
							//-------------------------------------------------------------------------------------
							//Envio de archivo min-M, server 1:
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
							f_temp = popen(cmd_line,"r");
							while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
							}
							pclose(f_temp);

							//Envio de archivo min-M, server 2:
							strcpy(cmd_line, "python ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/SendfileFTP.py ");
							strcat(cmd_line, pathserver2);
							strcat(cmd_line, " ");
							strcat(cmd_line, filesminm);
							strcat(cmd_line, " ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/lisn_logon2.txt");
							strcat(cmd_line, " >/dev/null 2>&1 &");
							f_temp = popen(cmd_line,"r");
							while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
							}
							pclose(f_temp);
							//-------------------------------------------------------------------------------------

							//-------------------------------------------------------------------------------------
							//Envio de archivo min-V, server 1:
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
							f_temp = popen(cmd_line,"r");
							while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
							}
							pclose(f_temp);

							//Envio de archivo min-V, server 2:
							strcpy(cmd_line, "python ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/SendfileFTP.py ");
							strcat(cmd_line, pathserver2);
							strcat(cmd_line, " ");
							strcat(cmd_line, filesminv);
							strcat(cmd_line, " ");
							strcat(cmd_line, dirwork);
							strcat(cmd_line, "/lisn_logon2.txt");
							strcat(cmd_line, " >/dev/null 2>&1 &");
							f_temp = popen(cmd_line,"r");
							while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
							}
							pclose(f_temp);
							//-------------------------------------------------------------------------------------
							printf("Minute M-V files FTP sent...\n");

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
					if(hh_SYS == 23){
						if(mm_SYS == 50){
							if(ss_SYS == 50){
								//****************************Actualizacion de hora UTC con servidor de LISN
								strcpy(cmd_line, "/home/magnet/UpdateTime");
								printf("cmd_line : %s\n",cmd_line);
								f_temp = popen(cmd_line,"r");
								while(fgets( cmd_line, sizeof(cmd_line), f_temp)){
								}
								pclose(f_temp);
							}
						}
					}
				}else
					printf("Error al recibir datos CMD9!\n");
				
				//Wait 1 second
				while(last_second == ss_SYS){
					num = FechaHora();
					usleep(100000);
				}
				last_second = ss_SYS;
			}
			//Rutina to stop digitizer:
			while(SendCMD('5') != true){
				usleep(700000); // 15 milisegundos
				printf("Stoping Digitizer\n");
			}
			EnaStream = false;
			DataReady = false;
			printf("Finish receive data.\n");
			ClosePort();
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

	newtio.c_cc[VTIME] = 20;				/* espera 2 seg.*/
	newtio.c_cc[VMIN] = TotalBytes;			/*tamaño de trama de datos ADQ*/

	tcsetattr(ID,TCSANOW, &newtio);
	return 1;
}

int ClosePort(void){
	tcsetattr(ID,TCSANOW, &oldtio);
	close(ID);
	return 1;
}

bool SendCMD(char idCMD){
	int num_byte, i;
	char bufferrx[41];

	bufferCMD[10] = idCMD;
	num_byte = write(ID, bufferCMD, 13);

	if(num_byte == 13){
		num_byte = read(ID, bufferrx, 13);
		for(i=0;i<10;i++){
			if((bufferrx[i]=='J') && (bufferrx[i+1]=='R') && (bufferrx[i+2]=='O')){
				if (bufferrx[i+6] == idCMD)
					return true;
			}
		}
	}
	return false;
}

bool ReceiveDAT(char idDAT){
	int num_byte, i, j;
	char bufferrx[41];

	num_byte = read(ID, bufferrx, 25);
	//printf("rx:%s\n", bufferrx);
	for(i=0;i<10;i++){
		if((bufferrx[i]=='J') && (bufferrx[i+1]=='R') && (bufferrx[i+2]=='O')){
			if (bufferrx[i+6] == idDAT){
				for(j= 0; j<12; j++){
					bufferDAT[j] =  bufferrx[i + 7 + j];
				}
				return true;
			}	
		}
	}
	return false;
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
		fprintf(setuplog,"%cPathD datos             %c,%c/mnt/usb_flash/magnet/year%c\n",34,34,34,34);
		fprintf(setuplog,"%cPathS segundos          %c,%c/mnt/usb_flash/magnet/year/dataseg%c\n",34,34,34,34);
		fprintf(setuplog,"%cPathM minutos           %c,%c/mnt/usb_flash/magnet/year/datamin%c\n",34,34,34,34);
		fprintf(setuplog,"%cPathZ zipdata           %c,%c/mnt/usb_flash/magnet/year/datazip%c\n",34,34,34,34);
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

int UpdateDir(char* dir){
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

int UpdateSetupFile(char* year){
	char fileconf2[250], cmd_line[250];
	FILE *f_temp;
	FILE *file1, *file2, *fp;
	int num, conta_line;
	
	strcpy(pathdata, "/mnt/usb_flash/magnet/20");
	strcat(pathdata, year);

	strcpy(pathmin, pathdata);
	strcat(pathmin, "/datamin");

	strcpy(pathseg, pathdata);
	strcat(pathseg, "/dataseg");

	strcpy(pathzip, pathdata);
	strcat(pathzip, "/datazip");

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
		printf("No se logro hacer copia de setuplog.cfg ---> ERROR:010\n");
		fp = fopen(filelog,"a");
		if(fp == NULL)
			printf("No se pudo crear historial.log\n");
		else{
			num = get_time();
			fprintf(fp,"%cFile setuplog_old.cfg failed at : %c,%s---> ERROR:010\n",34,34,tiempo);
			fprintf(fp,"%cProgram closed at : %c,%s ---> ERROR:010\n",34,34,tiempo);
			fclose(fp);
		}
	}else{
		conta_line = 0;
		while(fgets(cmd_line, sizeof(cmd_line), file1) != NULL){
			conta_line = conta_line + 1;			
			if(conta_line == 49){
				fprintf(file2,"%cPathD datos             %c,%c%s%c\n",34,34,34,pathdata,34);
				printf("DataPath: %s\n", pathdata);
			}else if(conta_line == 50){
				fprintf(file2,"%cPathS segundos          %c,%c%s%c\n",34,34,34,pathseg,34);
				printf("DataSeg: %s\n", pathseg);
			}else if(conta_line == 51){
				fprintf(file2,"%cPathM minutos           %c,%c%s%c\n",34,34,34,pathmin,34);
				printf("DataMin: %s\n", pathmin);
			}else if(conta_line == 52){
				fprintf(file2,"%cPathZ zipdata           %c,%c%s%c\n",34,34,34,pathzip,34);
				printf("DataZip: %s\n", pathzip);
			}else
				fprintf(file2,"%s", cmd_line);					
		}
		fclose(file1);
		fclose(file2);
	}

	return 1;
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
	signed long int valorAUX0b, valorAUX1b, valorAUX2b; 
	int num;
	bool flagfile = false;	
	FILE *fp1;
	FILE *fp2;

	//verificacion de existenia de carpeta:
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
			flagfile = true;
		}else{
			fclose(fp1);
		}

		if(flagfile){
			fp2 = fopen(filesseg,"w");
			if(fp2 == NULL){
				flagfile = false;
				return 0;
			}else{
				fprintf(fp2,"%s %s <%s> 1 Sec. Raw data\n", station, magnetometer, copy_numday);
				fprintf(fp2,"ValCte1:%11.9f ValCte2:%11.9f Temp_Ref:%3.2f TH:%3.2f TZ:%3.2f\n\n", cte1, cte2, TempRef, thFactor, tzFactor);
				fprintf(fp2,"HH MM SS  ch(H)  ch(D)  ch(Z)  ch(C)  ch(S)  ch(0)  ch(1)  ch(2)\n\n");
			}
			fclose(fp2);
		}

		if(DataReady){
			//Agregando contenido de datos al archivo
			fp1 = fopen(filesseg, "a");
			valorXb = valorX;
			valorYb = valorY;
			valorZb = valorZ;
			valorT1b = valorTc;
			valorT2b = valorTs;
			valorAUX0b = valorAUX0;
			valorAUX1b = valorAUX1;
			valorAUX2b = valorAUX2;
			fprintf(fp1,"%s %+06li %+06li %+06li %+06li %+06li %+06li %+06li %+06li\n",fecha4, valorXb, valorYb, valorZb, valorT1b, valorT2b, valorAUX0b, valorAUX1b, valorAUX2b);
			fclose(fp1);

			//Incrementa contador de segundos:
			conta_seg = conta_seg + 1;
			sumX = sumX + valorXb;
			sumY = sumY + valorYb;
			sumZ = sumZ + valorZb;
			sumTc = sumTc + valorT1b;
			sumTs = sumTs + valorT2b;
			sumAUX0 = sumAUX0 + valorAUX0b;
			sumAUX1 = sumAUX1 + valorAUX1b;
			sumAUX2 = sumAUX2 + valorAUX2b;
		}
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
	double mvAux0, mvAux1, mvAux2;
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
				fprintf(fp2,"DD MM YYYY HH MM     X(mv)      Y(mv)       Z(mv)       Tc(mv)      Ts(mv)      AUX0(mv)    AUX1(mv)    AUX2(mv)\n\n");
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
		mvAux0 = sumAUX0*cte1 + cte2;
		mvAux1 = sumAUX1*cte1 + cte2;
		mvAux2 = sumAUX2*cte1 + cte2;
		fprintf(fp1,"%s %+011.4f %+011.4f %+011.4f %+011.4f %+011.4f %+011.4f %+011.4f %+011.4f\n", fecha5, mvX, mvY, mvZ, mvTc, mvTs, mvAux0, mvAux0, mvAux0);
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
				fprintf(fp2,"DD MM YYYY HH MM    D(Deg)   H(nt)    Z(nt)    I(Deg)   F(nt)\n\n");
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
		fprintf(fp1,"%s  %+08.4f %+08.1f %+08.1f %+08.4f %+08.1f\n", fecha5, datachannelY, datachannelX, datachannelZ, datachannelI, datachannelF);
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
