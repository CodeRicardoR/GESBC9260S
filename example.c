/*
    Ejemplo 2 de strtok en C
    @author parzibyte
*/
#include <stdio.h>
#include <string.h>

int main(){
    char cadena[] = "/mnt/usb_flash/magnet/year/dataseg",
    delimitador[] = "/";
    char *token = strtok(cadena, delimitador);
    char letras[100];
    if(token != NULL){
        while(token != NULL){
            // Sólo en la primera pasamos la cadena; en las siguientes pasamos NULL
            printf("Token: %s\n", token);
            strcpy(letras, token);
            token = strtok(NULL, delimitador);
        }
    }
    printf("%s\n", letras);
}