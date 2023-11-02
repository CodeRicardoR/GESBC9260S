/*
 * CheckData.c
 *
 *  Created on: July 05, 2015
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



//Prototipo de funciones:
int ReadFileConf(void);
int current_file(void);

//Variables de configuracion:
static char *dirwork = "/home/magnet";
static char fileconf[250];
static char station[20];
static char IAGA[20];
static char pathmin[250];
static char filemin[250];
static char pathserver1[250];
static char pathserver2[250];

static char current_time[250];
static char string_time[250];
static char string_time_n[250];

int main(void){

//Declaracion de Variables
	int num;
	char *pointer;
	char logon1[250];
	char logon2[250];
	char cmd_line1[250];
	char cmd_line2[250];
	FILE *f_cmd;

	strcpy(fileconf,dirwork);
	strcat(fileconf,"/setuplog.cfg");
	//--------------------------------------
	strcpy(logon1,dirwork);
	strcat(logon1,"/lisn_logon1.txt");
	//--------------------------------------
	strcpy(logon2,dirwork);
	strcat(logon2,"/lisn_logon2.txt");

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
		//-------------------------------generando los comandos.
		strcpy(cmd_line1,"python /home/magnet/SendfileFTP.py ");
		strcat(cmd_line1,pathserver1);
		strcat(cmd_line1," ");
		strcat(cmd_line1,filemin);
		strcat(cmd_line1," ");
		strcat(cmd_line1,logon1);
		strcat(cmd_line1, " >/dev/null 2>&1 &");

		strcpy(cmd_line2,"python /home/magnet/SendfileFTP.py ");
		strcat(cmd_line2,pathserver2);
		strcat(cmd_line2," ");
		strcat(cmd_line2,filemin);
		strcat(cmd_line2," ");
		strcat(cmd_line2,logon2);
		strcat(cmd_line2, " >/dev/null 2>&1 &");

		printf("Comandos:\n%s\n%s\n",cmd_line1,cmd_line2);

		f_cmd = popen(cmd_line1,"r");
		if (f_cmd == NULL){
			//perror ("No se pudo ejecutar comando : %s\n",cmd_line);
			printf("Failed command sendfile1---> ERROR \n");
		}
		while (fgets( cmd_line1, sizeof(cmd_line1), f_cmd)){
		}
		pclose(f_cmd);	

		f_cmd = popen(cmd_line2,"r");
		if (f_cmd == NULL){
			//perror ("No se pudo ejecutar comando : %s\n",cmd_line);
			printf("Failed command sendfile2---> ERROR \n");
		}
		while (fgets( cmd_line2, sizeof(cmd_line2), f_cmd)){
		}
		pclose(f_cmd);	

		
	}
	printf("***FIN***\n");
	return 0;//Fin de Programa
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

				case 49://pathmin
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						pathmin[i-2] = pointer[i];
					pathmin[i-2] = '\0';
					break;

				case 52://pathserver1
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						pathserver1[i-2] = pointer[i];
					pathserver1[i-2] = '\0';
					break;

				case 53://pathserver2
					pointer = (char *)memchr(buffer_line,',',250);
					numchar = strlen(pointer);
					numchar = numchar - 2;
					for(i=2;i<numchar;i++)
						pathserver2[i-2] = pointer[i];
					pathserver2[i-2] = '\0';
					break;
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
		strftime(string_time,30," %d %m 20%y %H %M",tlocal);
		//---------------------------------------------------
		strcpy(string_time_n,string_time);
		if (string_time[16] == 48){
			if (string_time[15] == 48){
				if (string_time[13] == 48){												
					string_time_n[16] = 57;
					string_time_n[15] = 53;	
					string_time_n[13] = 57;
					string_time_n[12] = string_time[12] - 1;
				}else{
					string_time_n[16] = 57;
					string_time_n[15] = 53;	
					string_time_n[13] = string_time[13] - 1;
				}			
			}else{
				string_time_n[16] = 57;
				string_time_n[15] = string_time[15] - 1;
			}
		}else
			string_time_n[16] = string_time[16] - 1;
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
		strcat(filemin,"v");
		//Servidores
		strcat(pathserver1,"/20");
		strcat(pathserver1,year_string);
		strcat(pathserver2,"/");
		strcat(pathserver2,IAGA);
		strcat(pathserver2,year_string);
		return 1;
	}else
		return 0;
	
}





