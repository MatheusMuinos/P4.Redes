#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_ROTAS 2048
#define MAX_LINEA 256

typedef struct {
    uint32_t destino;
    int prefixo;
    char interfaz[16];
} Ruta;

/* crear mascarilla en orden de red (network byte order) */
static uint32_t crear_mascarilla(int prefixo) {
    if (prefixo <= 0) return 0;
    if (prefixo >= 32) return 0xFFFFFFFF;
    return (~0u) << (32 - prefixo);
}

/* convierte "193.144" -> 193.144.0.0 para aceptar 1..4 octetos */
static uint32_t convertir_a_uint32(const char *direccion) {
    uint32_t resultado = 0;
    const char *ptr = direccion;
    for (int i = 0; i < 4; ++i) {
        char *end;
        long oct = strtol(ptr, &end, 10);
        if (ptr == end) oct = 0;
        if (oct < 0) oct = 0;
        if (oct > 255) oct = oct & 0xFF;
        resultado |= (uint32_t)(oct & 0xFF) << (24 - i * 8);
        if (*end == '.') ptr = end + 1;
        else break;
    }
    return resultado;
}

/* cargar rutas en formato: red/prefijo,interfaz
   admite red con uno a catro octetos (ej. 194.64/16 o 194.64.0.0/16)
*/
static int cargar_rutas(const char *nombre_archivo, Ruta *rutas) {
    FILE *archivo = fopen(nombre_archivo, "r");
    if (!archivo) {
        perror("Error al abrir el archivo de rutas");
        return -1;
    }

    char linea[MAX_LINEA];
    int contador = 0;

    while (fgets(linea, sizeof(linea), archivo) && contador < MAX_ROTAS) {
        char *p = linea;
        /* eliminar retorno de carro y newline (\r\n o \n) */
        char *nl = strchr(p, '\n');
        if (nl) *nl = '\0';
        char *cr = strchr(p, '\r');
        if (cr) *cr = '\0';

        /* omitir líneas vacías */
        while (isspace((unsigned char)*p)) ++p;
        if (*p == '\0') continue;

        /* separar por coma */
        char *coma = strchr(p, ',');
        char parte_red[MAX_LINEA];
        char interfaz[16] = "0"; /* por defecto interfaz 0 */
        if (coma) {
            size_t len = coma - p;
            if (len >= sizeof(parte_red)) len = sizeof(parte_red) - 1;
            strncpy(parte_red, p, len);
            parte_red[len] = '\0';
            /* interfaz posible después de la coma, trim */
            char *iface = coma + 1;
            while (isspace((unsigned char)*iface)) ++iface;
            if (*iface) {
                strncpy(interfaz, iface, sizeof(interfaz) - 1);
                interfaz[sizeof(interfaz) - 1] = '\0';
            }
        } else {
            fprintf(stderr, "Formato de línea inválido (faltó coma): %s\n", linea);
            continue;
        }

        char destino_str[64];
        int prefixo;
        if (sscanf(parte_red, "%63[^/]/%d", destino_str, &prefixo) != 2) {
            fprintf(stderr, "Formato de red inválido: %s\n", parte_red);
            continue;
        }
        if (prefixo < 0 || prefixo > 32) {
            fprintf(stderr, "Prefijo fuera de rango: %d\n", prefixo);
            continue;
        }

        rutas[contador].destino = convertir_a_uint32(destino_str);
        rutas[contador].prefixo = prefixo;
        strncpy(rutas[contador].interfaz, interfaz, sizeof(rutas[contador].interfaz) - 1);
        rutas[contador].interfaz[sizeof(rutas[contador].interfaz) - 1] = '\0';

        /* depuración para mostrar ruta cargada */
        struct in_addr a;
        a.s_addr = htonl(rutas[contador].destino);
        printf("Cargada: %s/%d -> interfaz %s\n", inet_ntoa(a), rutas[contador].prefixo, rutas[contador].interfaz);
        sleep(2);
        contador++;
    }

    fclose(archivo);
    return contador;
}

int main (int argc, char *argv[]) {
    if (argc != 3) {
        fprintf (stderr, "Uso: %s <archivo_de_rutas> <ip_de_origen>\n", argv[0]);
        return EXIT_FAILURE;
    }

    Ruta rutas[MAX_ROTAS];
    int num_rutas = cargar_rutas (argv[1], rutas);
    if (num_rutas < 0) {
        return EXIT_FAILURE;
    }

    /* convertir IP de entrada */
    uint32_t ip_origen = convertir_a_uint32(argv[2]);

    /* búsqueda de mejor coincidencia (longest-prefix match) */
    int mejor_prefijo = -1;
    char mejor_interfaz[16] = "0";
    for (int i = 0; i < num_rutas; i++) {
        uint32_t mask = crear_mascarilla(rutas[i].prefixo);
        if ((rutas[i].destino & mask) == (ip_origen & mask)) {
            if (rutas[i].prefixo > mejor_prefijo) {
                mejor_prefijo = rutas[i].prefixo;
                strncpy(mejor_interfaz, rutas[i].interfaz, sizeof(mejor_interfaz)-1);
                mejor_interfaz[sizeof(mejor_interfaz)-1] = '\0';
            }
        }
    }

    if (mejor_prefijo < 0) {
        printf("\nInterfaz de salida: 0, Prefijo: 0\n"); // no se encontró ruta
    } else {
        printf("\nInterfaz de salida: %s, Prefijo: %d\n", mejor_interfaz, mejor_prefijo);
    }

    return EXIT_SUCCESS;
}