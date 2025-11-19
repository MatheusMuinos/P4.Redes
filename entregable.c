#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX 30 //Constante para el tamaño de línea a leer de la tabla de renvío

int main(int argc, char **argv) {
    if (argc != 3) { //Comprobamos el número de argumentos
        fprintf(stderr, "Uso: %s <archivo> <IP entrada>\n", argv[0]);
        return(EXIT_FAILURE);
    }

    //Convertimos a binario la IP de entrada (segundo argumento) y calculamos su sufijo
    struct in_addr IPentrada;
    IPentrada.s_addr = 0; //Inicializamos a 0 el campo de la estructura

    //Pasamos como parámetros:
    //AF_INET, especificamos versión IPv4
    //argv[2], cadena de caracteres que contiene la IP a convertir a binario
    //&IPentrada, deirección de la estructura que contiene el campo para la IP en binario
    //sizeof(struct in_addr), tamaño del tipo de dato del puntero anterior
    if (inet_net_pton(AF_INET, argv[2], (void *) &IPentrada, sizeof(struct in_addr)) < 0) { //Comprobamos posible error
        fprintf(stderr, "Formato de la direccion IP '%s' incorrecto\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    //Imprimimos IP pasada como argumento
    char IPpresentable[INET_ADDRSTRLEN]; //Buffer con el tamaño para IPv4 predefinido en la macro INET_ADDRSTLEN
    inet_ntop(AF_INET, (void *) &IPentrada.s_addr, IPpresentable, INET_ADDRSTRLEN);
    printf("IP: %s\n\n", IPpresentable);

    //Abrimos la tabla de renvío, pasada como argumento, en modo lectura y comprobamos
    FILE *tablaRenvio = fopen(argv[1], "r");
    if (!tablaRenvio) {
        fprintf(stderr, "Error en la apertura del archivo '%s'\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    char buffer[MAX]; //Buffer para la lectura de las líneas del archivo
    int sufijo = 0, interfaz = 0;

    printf("Buscando ruta...\n\n");
    while (fgets(buffer, MAX, tablaRenvio)) {
        struct in_addr IPtabla;
        IPtabla.s_addr = 0; //Inicializamos el campo a 0

        //Calculamos el sufijo con la función empleada al inicio del programa
        //strtok(buffer, ","), capturamos el contenido del buffer hasta la primera ',', que corresponde a IP/sufijo
        int temp_sufijo = inet_net_pton(AF_INET, strtok(buffer, ","), (void *) &IPtabla, sizeof(struct in_addr));
        if (temp_sufijo < 0) {
            fprintf(stderr, "Formato del bloque '%s' incorrecto\n", buffer);
            exit(EXIT_FAILURE);
        }

        //Creamos la máscara:
        in_addr_t mascara = 0; //Inicializamos a 0 el tipo de dato para IP de 32 bits (IPv4)
        //Desplazar a la izquierda '<<' un 1 sin signo '1U' tanto como el desplazamiento '32 - sufijo' y restamos una unidad para poner todo a 1s
        if (temp_sufijo > 0) //Comprobamos que el sufijo sea > 0
            mascara = htonl(~((1U << (32 - temp_sufijo)) - 1)); //Invertimos bits y pasamos a orden de red para 32 bits

        //Si el 'and' entre el IP pasado como argumento y la máscara equivale al IP de la tabla, y el sufijo es mayor, guardamos
        if (((IPentrada.s_addr & mascara) == IPtabla.s_addr) && (sufijo < temp_sufijo)) {
                sufijo = temp_sufijo; //Asignamos sufijo
                interfaz = (int) strtol(strtok(NULL, "\n"), NULL, 10); //Asignamos interfaz
        }
    }
    fclose(tablaRenvio); //Cerramos archivo

    //Imprimimos bits e interfaz
    printf("Bits prefijo: %d\nInterfaz: %d\n", sufijo, interfaz);

    exit(EXIT_SUCCESS);
}
