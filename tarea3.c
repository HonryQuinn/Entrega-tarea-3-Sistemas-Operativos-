#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <time.h>
#include <unistd.h> 
#include <stdbool.h>
#include <math.h>

#define MIN_TAMAÑO_PROCESO 5   
#define MAX_TAMAÑO_PROCESO 50
#define MAX_PROCESOS 100

int marcos_ram_totales;
int marcos_swap_totales;
int marcos_ram_usados = 0;
int marcos_swap_usados = 0;

int *memoria_ram = NULL;      
int *memoria_swap = NULL;

FILE *archivo_log = NULL;

const char* colores[] = {
    "\033[1;32m", 
    "\033[1;36m",  
    "\033[1;33m",  
    "\033[1;35m",  
    "\033[1;34m", 
    "\033[1;31m",  
    "\033[0;32m",  
    "\033[0;36m",  
    "\033[0;33m", 
    "\033[0;35m", 
    "\033[0;34m", 
    "\033[0;31m",  
    "\033[1;37m",  
    "\033[0;37m",
    "\033[1;30m", 
    "\033[0;30m",  
};

const int num_colores = 16;
const char* reset_color = "\033[0m";

const char* obtener_color(int proceso_id) {
    return colores[proceso_id % num_colores];
}

typedef struct {
    char nombre[50];
    int id;
    int tamaño;
    int num_paginas;
    int marcos_en_ram;
    int marcos_en_swap;
    bool activo;
    time_t tiempo_creacion;
} Proceso;

Proceso procesos[MAX_PROCESOS];
int num_procesos_activos = 0;

void inicializar_memoria(int mem_fisica_mb, int mem_swap_mb, int tamanio_pagina_kb) {
    marcos_ram_totales = (mem_fisica_mb * 1024) / tamanio_pagina_kb;
    marcos_swap_totales = (mem_swap_mb * 1024) / tamanio_pagina_kb;
    
    memoria_ram = (int*)malloc(marcos_ram_totales * sizeof(int));
    memoria_swap = (int*)malloc(marcos_swap_totales * sizeof(int));
    
    for (int i = 0; i < marcos_ram_totales; i++) {
        memoria_ram[i] = -1;
    }
    for (int i = 0; i < marcos_swap_totales; i++) {
        memoria_swap[i] = -1;
    }
    
    printf("Marcos en RAM: %d\n", marcos_ram_totales);
    printf("Marcos en SWAP: %d\n\n", marcos_swap_totales);

    for (int i = 0; i < MAX_PROCESOS; i++) {
        procesos[i].activo = false;
    }
}

bool asignar_proceso(Proceso *proceso) {
    int marcos_asignados = 0;
    
    for (int i = 0; i < marcos_ram_totales && marcos_asignados < proceso->num_paginas; i++) {
        if (memoria_ram[i] == -1) {
            memoria_ram[i] = proceso->id;
            marcos_asignados++;
        }
    }
    
    proceso->marcos_en_ram = marcos_asignados;
    marcos_ram_usados += marcos_asignados;
    
    if (marcos_asignados < proceso->num_paginas) {
        int marcos_necesarios_swap = proceso->num_paginas - marcos_asignados;
        int marcos_swap_asignados = 0;
        
        for (int i = 0; i < marcos_swap_totales && marcos_swap_asignados < marcos_necesarios_swap; i++) {
            if (memoria_swap[i] == -1) {
                memoria_swap[i] = proceso->id;
                marcos_swap_asignados++;
            }
        }
        
        if (marcos_swap_asignados < marcos_necesarios_swap) {
            for (int i = 0; i < marcos_ram_totales; i++) {
                if (memoria_ram[i] == proceso->id) {
                    memoria_ram[i] = -1;
                }
            }
            for (int i = 0; i < marcos_swap_totales; i++) {
                if (memoria_swap[i] == proceso->id) {
                    memoria_swap[i] = -1;
                }
            }
            marcos_ram_usados -= marcos_asignados;
            printf("  └─ No hay memoria suficiente\n");
            return false;
        }
        
        proceso->marcos_en_swap = marcos_swap_asignados;
        marcos_swap_usados += marcos_swap_asignados;
        printf("  └─ Asignado: RAM: %d marcos, SWAP: %d marcos\n", 
               proceso->marcos_en_ram, proceso->marcos_en_swap);
    } else {
        proceso->marcos_en_swap = 0;
        printf("  └─ Asignado en RAM: %d marcos\n", proceso->num_paginas);
    }
    
    return true;
}

int crear_proceso(int id, int tamanio_pagina_kb) {
    int indice = -1;
    for (int i = 0; i < MAX_PROCESOS; i++) {
        if (!procesos[i].activo) {
            indice = i;
            break;
        }
    }
    
    if (indice == -1) {
        printf("  └─ No hay espacio para más procesos\n");
        return -1;
    }
    
    procesos[indice].id = id;
    procesos[indice].tamaño = MIN_TAMAÑO_PROCESO + rand() % (MAX_TAMAÑO_PROCESO - MIN_TAMAÑO_PROCESO + 1);
    
    int tamanio_proceso_kb = procesos[indice].tamaño * 1024;
    procesos[indice].num_paginas = (tamanio_proceso_kb + tamanio_pagina_kb - 1) / tamanio_pagina_kb;
    procesos[indice].activo = false;
    
    sprintf(procesos[indice].nombre, "Proceso_%d", id);
    
    int marcos_libres_ram = marcos_ram_totales - marcos_ram_usados;
    int marcos_libres_swap = marcos_swap_totales - marcos_swap_usados;
    int marcos_libres_total = marcos_libres_ram + marcos_libres_swap;
    
    printf("\n%s┌─ Intentando crear: %s%s\n", obtener_color(id), procesos[indice].nombre, reset_color);
    printf("%s│  Tamaño proceso: %d MB (%d páginas necesarias)%s\n", 
           obtener_color(id), procesos[indice].tamaño, procesos[indice].num_paginas, reset_color);
    printf("│     Memoria disponible ANTES:\n");
    printf("│     └─ RAM:   %d/%d marcos libres\n", marcos_libres_ram, marcos_ram_totales);
    printf("│     └─ SWAP:  %d/%d marcos libres\n", marcos_libres_swap, marcos_swap_totales);
    printf("│     └─ TOTAL: %d marcos disponibles\n", marcos_libres_total);
    
    if (archivo_log != NULL) {
        fprintf(archivo_log, "\n----------------------------------\n");
        fprintf(archivo_log, "[ANTES DE CREAR] %s\n", procesos[indice].nombre);
        fprintf(archivo_log, "  Tamaño: %d MB (%d páginas)\n", 
                procesos[indice].tamaño, procesos[indice].num_paginas);
        fprintf(archivo_log, "  Memoria disponible:\n");
        fprintf(archivo_log, "    RAM:   %d/%d marcos libres\n", marcos_libres_ram, marcos_ram_totales);
        fprintf(archivo_log, "    SWAP:  %d/%d marcos libres\n", marcos_libres_swap, marcos_swap_totales);
        fprintf(archivo_log, "    TOTAL: %d marcos disponibles\n", marcos_libres_total);
    }
    
    if (!asignar_proceso(&procesos[indice])) {
        printf("%s└─ Fallo: No hay memoria suficiente%s\n", obtener_color(id), reset_color);
        if (archivo_log != NULL) {
            fprintf(archivo_log, "\n[RESULTADO] FALLO - No hay memoria suficiente\n");
        }
        return -1;
    }
    
    marcos_libres_ram = marcos_ram_totales - marcos_ram_usados;
    marcos_libres_swap = marcos_swap_totales - marcos_swap_usados;
    marcos_libres_total = marcos_libres_ram + marcos_libres_swap;
    
    printf("%s└─  Éxito: Proceso creado%s\n", obtener_color(id), reset_color);
    printf("    Memoria disponible después:\n");
    printf("      └─ RAM:   %d/%d marcos libres\n", marcos_libres_ram, marcos_ram_totales);
    printf("      └─ SWAP:  %d/%d marcos libres\n", marcos_libres_swap, marcos_swap_totales);
    printf("      └─ TOTAL: %d marcos disponibles\n", marcos_libres_total);
    
    if (archivo_log != NULL) {
        fprintf(archivo_log, "\n[Resultado] Éxito - Asignado: RAM=%d marcos, SWAP=%d marcos\n",
                procesos[indice].marcos_en_ram, procesos[indice].marcos_en_swap);
        fprintf(archivo_log, "  Memoria disponible después:\n");
        fprintf(archivo_log, "    RAM:   %d/%d marcos libres\n", marcos_libres_ram, marcos_ram_totales);
        fprintf(archivo_log, "    SWAP:  %d/%d marcos libres\n", marcos_libres_swap, marcos_swap_totales);
        fprintf(archivo_log, "    TOTAL: %d marcos disponibles\n", marcos_libres_total);
        fprintf(archivo_log, "----------------------------------------------------\n");
    }
    
    procesos[indice].activo = true;
    procesos[indice].tiempo_creacion = time(NULL);
    num_procesos_activos++;
    return indice;
}

void terminar_proceso(int indice) {
    if (indice < 0 || indice >= MAX_PROCESOS || !procesos[indice].activo) {
        return;
    }
    
    int proceso_id = procesos[indice].id;
    
    for (int i = 0; i < marcos_ram_totales; i++) {
        if (memoria_ram[i] == proceso_id) {
            memoria_ram[i] = -1;
        }
    }
    
    for (int i = 0; i < marcos_swap_totales; i++) {
        if (memoria_swap[i] == proceso_id) {
            memoria_swap[i] = -1;
        }
    }
    
    marcos_ram_usados -= procesos[indice].marcos_en_ram;
    marcos_swap_usados -= procesos[indice].marcos_en_swap;
    
    printf("%s🗑️  Terminado: %s (liberados: RAM=%d, SWAP=%d marcos)%s\n", 
           obtener_color(proceso_id),
           procesos[indice].nombre, 
           procesos[indice].marcos_en_ram, 
           procesos[indice].marcos_en_swap,
           reset_color);
    
    if (archivo_log != NULL) {
        fprintf(archivo_log, "\n[TERMINAR] %s (liberados: RAM=%d, SWAP=%d marcos)\n",
                procesos[indice].nombre,
                procesos[indice].marcos_en_ram,
                procesos[indice].marcos_en_swap);
    }
    
    procesos[indice].activo = false;
    num_procesos_activos--;
}

void terminar_proceso_aleatorio() {
    if (num_procesos_activos == 0) {
        printf("No hay procesos activos para terminar\n");
        return;
    }
    
    int proceso_mas_antiguo = -1;
    time_t tiempo_mas_antiguo = time(NULL) + 1000;
    printf("---------------------------------------\n");
    printf("\n Buscando proceso más antiguo (FIFO):\n");
    printf("---------------------------------------\n");
    
    for (int i = 0; i < MAX_PROCESOS; i++) {
        if (procesos[i].activo) {
            printf("  %sP%d%s: creado hace %ld segundos\n", 
                   obtener_color(procesos[i].id),
                   procesos[i].id,
                   reset_color,
                   time(NULL) - procesos[i].tiempo_creacion);
            
            if (procesos[i].tiempo_creacion < tiempo_mas_antiguo) {
                tiempo_mas_antiguo = procesos[i].tiempo_creacion;
                proceso_mas_antiguo = i;
            }
        }
    }
    
    if (proceso_mas_antiguo != -1) {
        printf("  Seleccionado para terminar: %sP%d%s (el más antiguo)\n", 
               obtener_color(procesos[proceso_mas_antiguo].id),
               procesos[proceso_mas_antiguo].id,
               reset_color);
        printf("-----------------------------------------------\n");
        
        if (archivo_log != NULL) {
            fprintf(archivo_log, "\n--- FIFO: Seleccionando proceso más antiguo ---\n");
            fprintf(archivo_log, "Proceso seleccionado: P%d (creado hace %ld segundos)\n",
                    procesos[proceso_mas_antiguo].id,
                    time(NULL) - procesos[proceso_mas_antiguo].tiempo_creacion);
        }
        
        terminar_proceso(proceso_mas_antiguo);
    }
}

void guardar_matriz_en_log(int *memoria, int total_marcos, const char *tipo) {
    if (archivo_log == NULL) return;
    
    int filas = (int)sqrt(total_marcos);
    int columnas = (total_marcos + filas - 1) / filas;
    
    fprintf(archivo_log, "\n---- %s ----\n", tipo);
    fprintf(archivo_log, "Dimensiones: %d x %d = %d marcos\n", filas, columnas, total_marcos);
    fprintf(archivo_log, "Uso: %d/%d marcos (%.1f%%)\n", 
            strcmp(tipo, "RAM") == 0 ? marcos_ram_usados : marcos_swap_usados,
            total_marcos,
            strcmp(tipo, "RAM") == 0 ? 
                (100.0 * marcos_ram_usados / total_marcos) : 
                (100.0 * marcos_swap_usados / total_marcos));
    
    int marco_idx = 0;
    for (int f = 0; f < filas && f < 20; f++) {
        for (int c = 0; c < columnas && marco_idx < total_marcos && c < 40; c++) {
            int proceso_id = memoria[marco_idx];
            if (proceso_id == -1) {
                fprintf(archivo_log, "[ - ]");
            } else {
                fprintf(archivo_log, "[P%-2d]", proceso_id);
            }
            marco_idx++;
        }
        fprintf(archivo_log, "\n");
    }
    if (filas > 20 || columnas > 40) {
        fprintf(archivo_log, "... (matriz truncada)\n");
    }
}

void mostrar_estado() {
    printf("\n______________________________________\n");
    printf("│ RAM:  %d/%d marcos (%.1f%%)     │\n", 
           marcos_ram_usados, marcos_ram_totales,
           100.0 * marcos_ram_usados / marcos_ram_totales);
    printf("│ SWAP: %d/%d marcos (%.1f%%)     │\n", 
           marcos_swap_usados, marcos_swap_totales,
           100.0 * marcos_swap_usados / marcos_swap_totales);
    printf("\n______________________________________\n");
    
    if (archivo_log != NULL) {
        fprintf(archivo_log, "\n--- Estado del Sistema ---\n");
        fprintf(archivo_log, "RAM:  %d/%d marcos (%.1f%%)\n", 
                marcos_ram_usados, marcos_ram_totales,
                100.0 * marcos_ram_usados / marcos_ram_totales);
        fprintf(archivo_log, "SWAP: %d/%d marcos (%.1f%%)\n", 
                marcos_swap_usados, marcos_swap_totales,
                100.0 * marcos_swap_usados / marcos_swap_totales);
        fprintf(archivo_log, "Procesos activos: %d\n", num_procesos_activos);
    }
}

void mostrar_visualizacion_completa() {
    system("clear");
    
    printf("\n");
    printf("----------SIMULADOR DE GESTIÓN DE MEMORIA----------\n");
    mostrar_estado();
    
    printf("\n Procesos Activos: %d\n", num_procesos_activos);
    printf("─────────────────────────────────────\n");
    printf("ID  | Nombre       | Tamaño | RAM  | SWAP | Edad\n");
    printf("────┼──────────────┼────────┼──────┼──────┼──────\n");
    
    time_t ahora = time(NULL);
    for (int i = 0; i < MAX_PROCESOS; i++) {
        if (procesos[i].activo) {
            int edad = ahora - procesos[i].tiempo_creacion;
            printf("%s%-3d%s | %-12s | %4d MB | %4d | %4d | %3ds\n",
                   obtener_color(procesos[i].id),
                   procesos[i].id,
                   reset_color,
                   procesos[i].nombre,
                   procesos[i].tamaño,
                   procesos[i].marcos_en_ram,
                   procesos[i].marcos_en_swap,
                   edad);
        }
    }
    printf("\n");
}

int main() {
    int memoria_fisica;
    int tamanio_pagina;
    
    archivo_log = fopen("simulacion_log.txt", "w");
    if (archivo_log == NULL) {
        printf("Advertencia: No se pudo crear el archivo de log TXT\n");
    }
    
    printf("Ingrese cantidad de Memoria Fisica (MB): ");
    scanf("%d", &memoria_fisica);
    
    printf("Ingrese tamaño de página (KB): ");
    scanf("%d", &tamanio_pagina);
    
    srand(time(NULL));
    float factor = 1.5 + ((float)rand() / RAND_MAX) * 3.0;  
    int memoria_virtual = memoria_fisica * factor;
    int tamanio_swap = memoria_virtual - memoria_fisica;
    
    printf("\n--- Configuración del Sistema ---\n");
    printf("Memoria Física (RAM): %d MB\n", memoria_fisica);
    printf("Memoria Virtual: %d MB (factor: %.2f)\n", memoria_virtual, factor);
    printf("Memoria Swap: %d MB\n", tamanio_swap);
    printf("Tamaño de página: %d KB\n", tamanio_pagina);
    printf("-------------------------------\n\n");

    if (archivo_log != NULL) {
        fprintf(archivo_log, "--- CONFIGURACIÓN DEL SISTEMA ---\n");
        fprintf(archivo_log, "Memoria Física (RAM): %d MB\n", memoria_fisica);
        fprintf(archivo_log, "Memoria Virtual: %d MB (factor: %.2f)\n", memoria_virtual, factor);
        fprintf(archivo_log, "Memoria Swap: %d MB\n", tamanio_swap);
        fprintf(archivo_log, "Tamaño de página: %d KB\n", tamanio_pagina);
        fprintf(archivo_log, "Tamaño de procesos: %d-%d MB\n", MIN_TAMAÑO_PROCESO, MAX_TAMAÑO_PROCESO);
        fprintf(archivo_log, "----------------------------------\n\n");
    }

    inicializar_memoria(memoria_fisica, tamanio_swap, tamanio_pagina);

    time_t inicio = time(NULL);
    time_t ultimo_creacion = inicio;
    time_t ultimo_terminacion = inicio + 30;
    int contador = 0;
    
    while (1) {
        time_t actual = time(NULL);
        int tiempo_transcurrido = actual - inicio;
        
        if (actual - ultimo_creacion >= 2) {
            if (crear_proceso(contador, tamanio_pagina) == -1) {
                printf("\nNo hay memoria disponible. Terminando simulación.\n");
                break;
            }
            
            mostrar_visualizacion_completa();
            printf("[%d s] Proceso P%d creado\n", tiempo_transcurrido, contador);
            contador++;
            ultimo_creacion = actual;
        }
        
        if (tiempo_transcurrido >= 30 && actual - ultimo_terminacion >= 5) {
            terminar_proceso_aleatorio();
            mostrar_visualizacion_completa();
            printf("[%d s] Proceso terminado\n", tiempo_transcurrido);
            
            ultimo_terminacion = actual;
        }
        
        sleep(1);
    }
    
    printf("\nSimulación terminada\n");
    mostrar_visualizacion_completa();
    
    // Guardar matrices solo en el archivo log al final
    if (archivo_log != NULL) {
        fprintf(archivo_log, "\n\n--- ESTADO FINAL DE LA MEMORIA ---\n");
        fprintf(archivo_log, "Total de procesos creados: %d\n", contador);
        guardar_matriz_en_log(memoria_ram, marcos_ram_totales, "RAM");
        guardar_matriz_en_log(memoria_swap, marcos_swap_totales, "SWAP");
        fprintf(archivo_log, "\n--- SIMULACIÓN TERMINADA ---\n");
        fclose(archivo_log);
        printf("Log guardado en: simulacion_log.txt\n");
    }
    
    if (memoria_ram != NULL) free(memoria_ram);
    if (memoria_swap != NULL) free(memoria_swap);
    
    return 0;
}
