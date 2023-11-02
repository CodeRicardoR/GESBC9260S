/*
 * CheckDataV6.c
 *
 *  Created on: Febrery 19, 2020
 *      Author: Ricardo Rojas Quispe
 *      e-mail: net85.ricardo@gmail.com
 */

/*	
	Se encarga de supervisar el funcionamiento del programa de 
	adquisicion y el archivo de minuto generado en el día.
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
#include <stdbool.h>

//Definicion de constantes:


//Prototipo de funciones:
int ReadFileConf(void);
int current_file(void);

//Variables de configuracion:
static char *dirwork = "/home/magnet";
static char fileconf[250];
static char filestatus[250];
static char filelog[250];
static char station[20];
static char IAGA[20];
static char pathmin[250];
static char filemin[250];
static char USBdevice[250];
static char Linepid[250];
static char* Magnetpid;
static char current_time[250];
static char string_time[250];
static char string_time_n[250];

bool TestEnable = false;
  	
int main(void){

//Declaracion de Variables
	int num;
	FILE *f_cmd;
	FILE *f_status;
	FILE *f_log;
	char aux[250];
	int flag_conta;
	int flag_status;
	int reset_system;
	char cmd_line[250];
	char *pointer;
	int flag_data = 0;
	

	strcpy(fileconf,dirwork);
	strcat(fileconf,"/setuplog.cfg");
	//--------------------------------------
	strcpy(filestatus,dirwork);
	strcat(filestatus,"/statusmagnet.txt");
	//--------------------------------------
	strcpy(filelog,dirwork);
	strcat(filelog,"/statuslog.txt");
	
	num = current_file();
	if (num){
		//Verificacion de la hora, si es 00:00 no hacer nada
		printf("Hora local: %s\n",current_time);
		//No hacer nada en caso de Hora 00
		pointer = strstr(string_time,"00 00");
		if (pointer != NULL){
			printf("Hora es 00:00 no hacer nada.\n *****FIN de PROGRAMA*****\n");
			return 0;
		}
		//No hacer nada en caso de no haber seteado el reloj (01 01 1970)
		pointer = strstr(string_time,"01 01 2070");
		if (pointer != NULL){
			printf("Fecha 01/01/1970 no hacer nada.\n *****FIN de PROGRAMA*****\n");
			return 0;
		}
		//Delay de 30 segundos
		usleep(30000000);		
		
		//Verificacion si esta en ejecucion Magnetometer:
		f_cmd = popen("ps","r");
		if (f_cmd == NULL){
			perror ("No se pudo ejecutar comando : ps");
			exit (-1);
		}
		flag_conta = 0;
		strcpy(Linepid,"");
		while(fgets(aux,sizeof(aux),f_cmd) != NULL){
			pointer = strstr( aux, "Magnetometer");
			if (pointer != NULL){
				strcpy(Linepid,aux);
				flag_conta = flag_conta + 1;
				Magnetpid = strtok(Linepid," ");
				printf("ID PID:%s\n",Magnetpid);
				break;		
			}
		}
		pclose(f_cmd);
		//------------------------------------------------

		//Verificar si el archivo de status existe:
		flag_status = 255;
		f_status = fopen(filestatus,"r");
		if (f_status){
			fclose(f_status);
			//Borramos archivo si existe
			flag_status = remove(filestatus);
		}
		f_status = fopen(filestatus,"w");
		if(f_status == NULL){
			f_log = fopen(filelog,"a");
			if (f_log == NULL)
				return 0;
			else{
				fprintf(f_log,"  MSG  :  fallo escritura de archivo statusmagnet.txt => ERROR 002.\n");
				fclose(f_log);
				return 0;		
			}			
		}else{
			//Escritura de archivo de statusmagnet.txt
			fprintf(f_status,"-----------------------------------------------------------------------\n\n");
			//-----------------------------------------------------------------
			//Verificacmos existencia de archivo de minutos
			reset_system = 0;				
			f_cmd = fopen(filemin,"r");
			if (f_cmd == NULL){
				//Reseteamos sistema
				reset_system = reset_system + 1;
				fprintf(f_status,"Current data file  => %s NO EXISTE.\n",filemin);
				//Escribimos motivo del reset
				f_log = fopen(filelog,"a");
				if (f_log == NULL)
					printf("No se puedo abrir statuslog.txt\n");
				else{
					fprintf(f_log,"  MSG  :  Archivo de minuto no encontrado  en %s\n",current_time);
					fclose(f_log);		
				}
			}else{
				fclose(f_cmd);
				fprintf(f_status,"Current data file  => %s EXISTE.\n",filemin);
				//leemos cabecera del archivo:
				strcpy(cmd_line,"");
				strcat(cmd_line,"head -n 3 ");
				strcat(cmd_line,filemin);
				f_cmd = popen(cmd_line,"r");
				if (f_cmd == NULL){
					perror ("No se puede abrir ls -l");
					exit (-1);
				}
				while(fgets(aux,sizeof(aux),f_cmd) != NULL){
					fprintf(f_status,"%s",aux);
				}
				pclose(f_cmd);
				
				//leemos datos del archivo (5 ultimos):
				strcpy(cmd_line,"");
				strcat(cmd_line,"tail -n 5 ");
				strcat(cmd_line,filemin);
				f_cmd = popen(cmd_line,"r");
				if (f_cmd == NULL){
					perror ("No se puede abrir ls -l");
					exit (-1);
				}
				flag_data = 0;
				while(fgets(aux,sizeof(aux),f_cmd) != NULL){
					//analisis de la linea de aux => string de datos:
					// Formato:
					// 04 03 2015 00 00  -01.4585 +25695.2 +00370.9 +00.8271 +25697.9
					printf("hora a buscar : %s",string_time_n);
					pointer = strstr( aux, string_time_n);
					if (pointer != NULL){
						flag_data = flag_data + 1;
						printf("  ENCONTRADO\n");		
					}
					printf("\n");
					fprintf(f_status,"%s",aux);
				}
				pclose(f_cmd);
			}
			//-----------------------------------------------------------------
			
			//Escribimos informacion de hora:
			fprintf(f_status,"-----------------------------------------------------------------------\n");
			fprintf(f_status,"REMOTE SYSTEM TIME => %s\n",current_time);
		
			//Escribimos información del pid del proceso 			
			if (flag_conta > 0){
					fprintf(f_status,"PID process        => %s",Linepid);
					fclose(f_status);
			}else{
				fprintf(f_status,"PID process        => No found\n");
				fclose(f_status);
				reset_system = reset_system + 1;
				//Escribimos motivo del reset
				f_log = fopen(filelog,"a");
				if (f_log == NULL)
					printf("No se puedo abrir statuslog.txt\n");
				else{
					fprintf(f_log,"  MSG  :  Programa de adquisición cerrado  en %s\n",current_time);
					fclose(f_log);		
				}
			}

			if (flag_data == 0){
				reset_system = reset_system + 1;
				//Escribimos motivo del reset
				f_log = fopen(filelog,"a");
				if (f_log == NULL)
					printf("No se puedo abrir statuslog.txt\n");
				else{
					fprintf(f_log,"  MSG  :  Archivo de minuto no actualizado en %s\n",current_time);
					fclose(f_log);		
				}
			}
				
			if (reset_system == 0){
				strcpy(cmd_line,"");
				strcat(cmd_line,"cp /home/magnet/setuplog.cfg /home/www/files/setuplog.txt");
				f_cmd = popen(cmd_line,"r");
				while (fgets( cmd_line, sizeof(cmd_line), f_cmd)){
				}
				pclose(f_cmd);

				strcpy(cmd_line,"");
				strcat(cmd_line,"cp /home/magnet/statusmagnet.txt /home/www/files/statusmagnet.txt");
				f_cmd = popen(cmd_line,"r");
				while (fgets( cmd_line, sizeof(cmd_line), f_cmd)){
				}
				pclose(f_cmd);

				strcpy(cmd_line,"");
				strcat(cmd_line,"cp /mnt/usb_flash/magnet/fgxfiles.txt /home/www/files/fgxfiles.txt");
				f_cmd = popen(cmd_line,"r");
				while (fgets( cmd_line, sizeof(cmd_line), f_cmd)){
				}
				pclose(f_cmd);
			}else{
				//Reset de sistema
				f_log = fopen(filelog,"a");
				if (f_log == NULL)
					printf("No se puedo abrir statuslog.txt\n");
				else{
					fprintf(f_log,"  MSG  :  Sistema de adquisicion reseteado en %s\n",current_time);
					fclose(f_log);		
				}
				//Cerramos Magnetometer
				if (flag_conta > 0){
					strcpy(cmd_line,"");
					strcat(cmd_line,"kill ");
					strcat(cmd_line,Magnetpid);
					printf("CMD ejecutando: %s\n",cmd_line);
					f_cmd = popen(cmd_line,"r");
					if (f_cmd == NULL){
						perror ("No se pudo ejecutar comando : kill");
						exit (-1);
					}
					while (fgets( cmd_line, sizeof(cmd_line), f_cmd)){
					}
					pclose(f_cmd);
				}
				//****************************desmontaje de USB	
				strcpy(cmd_line,"");
				strcat(cmd_line,"umount ");
				strcat(cmd_line, USBdevice);
				printf("CMD ejecutando: %s\n",cmd_line);
				f_cmd = popen(cmd_line,"r");
				if (f_cmd == NULL){
					//perror ("No se pudo ejecutar comando : %s\n",cmd_line);
					printf("Failed command %s ---> ERROR:00\n",cmd_line);
					f_log = fopen(filelog,"a");
					if(f_log == NULL)
						printf("No se pudo crear historial.log\n");
					else{
						fprintf(f_log,"%cCommand %s umount failed : %c,%s ---> ERROR:\n",34,cmd_line,34,current_time);
						fclose(f_log);
					}
					return 0;//fin de programa
				}
				while (fgets( cmd_line, sizeof(cmd_line), f_cmd)){
				}
				pclose(f_cmd);	
				//****************************Ejecutando MagnetReset
				strcpy(cmd_line,"");
				strcat(cmd_line,"/home/magnet/GPIOreset");
				printf("CMD ejecutando: %s\n",cmd_line);
				if(TestEnable){
                    f_cmd = popen(cmd_line,"r");
                    if (f_cmd == NULL){
                        perror ("No se pudo ejecutar comando : Magnetreset");
                        exit (-1);
                    }
                    while (fgets( cmd_line, sizeof(cmd_line), f_cmd)){
                    }
                    pclose(f_cmd);
                }
				//*********************** Apagando sistema embebido
				printf("Apagando sistema embebido...\n");
				strcpy(cmd_line,"");
				strcat(cmd_line,"halt");
				if(TestEnable){
                    f_cmd = popen(cmd_line,"r");
                    if (f_cmd == NULL){
                        //perror ("No se pudo ejecutar comando : %s\n",cmd_line);
                        printf("Failed command halt %s ---> ERROR \n",cmd_line);
                        f_log = fopen(filelog,"a");
                        if(f_log == NULL)
                            printf("No se pudo crear historial.log\n");
                        else{
                            fprintf(f_log,"%cCommand %s halt failed : %c,%s ---> ERROR:\n",34,cmd_line,34,current_time);
                            fclose(f_log);
                        }
                        return 0;//fin de programa
                    }
                    while (fgets( cmd_line, sizeof(cmd_line), f_cmd)){
                    }
                    pclose(f_cmd);
                }	
				//***********************
				return 0; //Fin del programa
			}
		}
		return 0;

	}else{
		f_log = fopen(filelog,"a");
		if (f_log == NULL)
			return 0;
		else{
			fprintf(f_log,"  MSG  :  No se pudo leer archivo de configurcion setuplog.cfg => ERROR 001.\n");
			fclose(f_log);
			return 0;		
		}
	}
	
}


/*FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF*/
//Funciones:

int ReadFileConf(void){

	//Varible para archivo
	FILE *setuplog;
	char buffer_line[250];
	int conta_line;
	char *pointer;
	int numchar,i;

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

				case 51://pathmin
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						pathmin[i-2] = pointer[i];
					pathmin[i-2] = '\0';
					break;
                
                case 56://USB Device
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						USBdevice[i-2] = pointer[i];
					USBdevice[i-2] = '\0';
					break;
				
				default:
					continue;
			}
		}
		fclose(setuplog);
		return 1;
	}
}

int current_file(void){
	char day_string[10];
	char mes_string[10];
	char year_string[10];
	int num;
	int num_mes;

	num = ReadFileConf();
	if (num){
		time_t tiempo_complejo = time(0);
		struct tm *tlocal = localtime(&tiempo_complejo);
		//day current:
		strftime(day_string,10,"%d",tlocal);
		strftime(mes_string,10,"%m",tlocal);
		strftime(year_string,10,"%y",tlocal);
		strftime(current_time,30,"20%y-%m-%d %H:%M:%S",tlocal);
		strftime(string_time,30,"%d %m 20%y %H %M",tlocal);
		//---------------------------------------------------
		strcpy(string_time_n,string_time);
		if (string_time[15] == 48){
			if (string_time[14] == 48){
				if (string_time[12] == 48){												
					string_time_n[15] = 57;
					string_time_n[14] = 53;	
					string_time_n[12] = 57;
					string_time_n[11] = string_time[11] - 1;
				}else{
					string_time_n[15] = 57;
					string_time_n[14] = 53;	
					string_time_n[12] = string_time[12] - 1;
				}			
			}else{
				string_time_n[15] = 57;
				string_time_n[14] = string_time[14] - 1;
			}
		}else
			string_time_n[15] = string_time[15] - 1;
		//---------------------------------------------------

		num_mes = atoi(mes_string);
		switch(num_mes){
			case 1:
				strcpy(mes_string,"");
				strcat(mes_string,"jan");
				break;
			case 2:
				strcpy(mes_string,"");
				strcat(mes_string,"feb");
				break;
			case 3:
				strcpy(mes_string,"");
				strcat(mes_string,"mar");
				break;
			case 4:
				strcpy(mes_string,"");
				strcat(mes_string,"apr");
				break;
			case 5:
				strcpy(mes_string,"");
				strcat(mes_string,"may");
				break;
			case 6:
				strcpy(mes_string,"");
				strcat(mes_string,"jun");
				break;
			case 7:
				strcpy(mes_string,"");
				strcat(mes_string,"jul");
				break;
			case 8:
				strcpy(mes_string,"");
				strcat(mes_string,"aug");
				break;
			case 9:
				strcpy(mes_string,"");
				strcat(mes_string,"sep");
				break;
			case 10:
				strcpy(mes_string,"");
				strcat(mes_string,"oct");
				break;
			case 11:
				strcpy(mes_string,"");
				strcat(mes_string,"nov");
				break;
			case 12:
				strcpy(mes_string,"");
				strcat(mes_string,"dec");
				break;
		}
		strcpy(filemin,pathmin);
		strcat(filemin,"/");
		strcat(filemin,IAGA);
		strcat(filemin,day_string);
		strcat(filemin,mes_string);
		strcat(filemin,".");
		strcat(filemin,year_string);
		strcat(filemin,"m");
		return 1;
	}else
		return 0;
	
}





