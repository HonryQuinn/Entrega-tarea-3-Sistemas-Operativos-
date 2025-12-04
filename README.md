


# Simulador de Gestión de Memoria (RAM y SWAP)

Este proyecto implementa un simulador de gestión de memoria a bajo nivel. El sistema emula el comportamiento de un Sistema Operativo al administrar la memoria principal (RAM) y el mecanismo de intercambio (SWAP) cuando la memoria se llena, implementando una política FIFO.

##  Características

- **Simulación de RAM:** Asignación de bloques de memoria para procesos simulados.
- **Mecanismo de SWAP:** Implementación de paginación/intercambio para mover procesos inactivos al disco virtual cuando la RAM está saturada.
- **Gestión de Espacio:** Detección de fragmentación y búsqueda de espacios libres.
- **Visualización:** Muestra el estado actual de la memoria y del archivo de intercambio.

##  Constantes del Sistema
```c
#define MIN_TAMAÑO_PROCESO 5    // Tamaño mínimo de proceso (MB)
#define MAX_TAMAÑO_PROCESO 50   // Tamaño máximo de proceso (MB)
#define MAX_PROCESOS 100        // Máximo número de procesos
```
## Pasos para ejecutar:

### 1. Compilar el programa
```bash
gcc simulador.c -o simulador -lm
```

### 2. Ejecutar el simulador
```bash
./simulador
```

### 3. Ingresar tamaño de RAM
```
Ingrese cantidad de Memoria Fisica (MB): 512
```

### 4. Ingresar tamaño de páginas
```
Ingrese tamaño de página (KB): 4
```

#  Documentación de Funciones

## 1.-) `obtener_color(int proceso_id)`

**Descripción:** Devuelve un código de color ANSI para representar visualmente cada proceso en la terminal.

**Parámetros:**
- `proceso_id`: ID del proceso

**Retorna:** String con código de color ANSI

**Funcionamiento:** Utiliza el operador módulo para asignar colores de forma cíclica entre 16 colores predefinidos, asegurando que cada proceso tenga una representación visual única.

---

## 2.-) `inicializar_memoria(int mem_fisica_mb, int mem_swap_mb, int tamanio_pagina_kb)`

**Descripción:** Inicializa y configura las estructuras de datos que simulan la memoria RAM y SWAP del sistema.

**Parámetros:**
- `mem_fisica_mb`: Cantidad de memoria física en MB
- `mem_swap_mb`: Cantidad de memoria swap en MB
- `tamanio_pagina_kb`: Tamaño de cada página en KB

**Funcionamiento:**
1. Calcula el número total de marcos para RAM y SWAP
2. Reserva memoria dinámica usando `malloc()`
3. Inicializa todos los marcos como libres (valor -1)
4. Configura todos los procesos como inactivos

---

## 3.-) `asignar_proceso(Proceso *proceso)`

**Descripción:** Asigna marcos de memoria a un proceso siguiendo una estrategia de asignación RAM-first.

**Parámetros:**
- `proceso`: Puntero a la estructura del proceso

**Retorna:** 
- `true`: Asignación exitosa
- `false`: No hay memoria suficiente

**Funcionamiento:**
1. Intenta asignar todos los marcos necesarios en RAM
2. Si la RAM se llena, continúa asignando en SWAP
3. Si no hay espacio suficiente en ninguna, libera todo lo asignado y retorna fallo
4. Actualiza los contadores de memoria usada

---

## 4.-) `crear_proceso(int id, int tamanio_pagina_kb)`

**Descripción:** Crea un nuevo proceso con tamaño aleatorio y le asigna memoria.

**Parámetros:**
- `id`: Identificador único del proceso
- `tamanio_pagina_kb`: Tamaño de página del sistema

**Retorna:** 
- Índice del proceso creado (≥0)
- `-1` si falla

**Funcionamiento:**
1. Busca un espacio libre en el array de procesos
2. Genera un tamaño aleatorio entre MIN_TAMAÑO_PROCESO y MAX_TAMAÑO_PROCESO
3. Calcula el número de páginas necesarias
4. Muestra estado de memoria ANTES de asignar
5. Llama a `asignar_proceso()` para obtener memoria
6. Registra el tiempo de creación (para política FIFO)
7. Muestra estado de memoria DESPUÉS de asignar
8. Guarda toda la información en el archivo log

---

## 5.-) `terminar_proceso(int indice)`

**Descripción:** Elimina un proceso del sistema y libera todos sus recursos de memoria.

**Parámetros:**
- `indice`: Índice del proceso en el array

**Funcionamiento:**
1. Recorre todos los marcos de RAM liberando los que pertenecen al proceso
2. Recorre todos los marcos de SWAP liberando los que pertenecen al proceso
3. Actualiza los contadores de memoria usada
4. Marca el proceso como inactivo
5. Registra la operación en el log
6. Decrementa el contador de procesos activos

---

## 6.-) `terminar_proceso_aleatorio()`

**Descripción:** Implementa la política de reemplazo FIFO (First In, First Out), seleccionando y terminando el proceso más antiguo del sistema.

**Funcionamiento:**
1. Verifica que existan procesos activos
2. Recorre todos los procesos activos comparando sus tiempos de creación
3. Selecciona el proceso con el menor timestamp (más antiguo)
4. Muestra información sobre la selección FIFO
5. Llama a `terminar_proceso()` para eliminarlo
6. Registra la decisión en el log

**Nota:** El nombre "aleatorio" es legacy, pero la función implementa FIFO determinístico.

---

## 7.-) `guardar_matriz_en_log(int *memoria, int total_marcos, const char *tipo)`

**Descripción:** Genera y guarda una representación visual matricial de la memoria en el archivo log.

**Parámetros:**
- `memoria`: Array que representa RAM o SWAP
- `total_marcos`: Número total de marcos
- `tipo`: String "RAM" o "SWAP"

**Funcionamiento:**
1. Calcula dimensiones óptimas (aproximadamente cuadradas) usando `sqrt()`
2. Escribe encabezado con información de uso
3. Genera matriz donde cada celda muestra:
   - `[ - ]`: Marco libre
   - `[Pxx]`: Marco ocupado por proceso xx
4. Trunca la matriz si supera 20 filas × 40 columnas

---

## 8.-) `mostrar_estado()`

**Descripción:** Muestra un resumen compacto del estado actual de la memoria.

**Salida:**
- En consola: Recuadro visual con uso de RAM y SWAP
- En log: Misma información en formato texto

**Información mostrada:**
- Marcos usados/totales para RAM
- Marcos usados/totales para SWAP
- Porcentaje de uso de cada tipo de memoria
- Número de procesos activos (solo en log)

---

## 9.-) `mostrar_visualizacion_completa()`

**Descripción:** Actualiza la pantalla completa con toda la información del sistema en tiempo real.

**Funcionamiento:**
1. Limpia la consola (`clear`)
2. Muestra banner del simulador
3. Llama a `mostrar_estado()` para mostrar uso de memoria
4. Genera tabla de procesos activos con columnas:
   - **ID**: Identificador del proceso
   - **Nombre**: Proceso_X
   - **Tamaño**: MB asignados
   - **RAM**: Marcos en memoria física
   - **SWAP**: Marcos en memoria virtual
   - **Edad**: Segundos desde su creación
5. Usa colores ANSI para identificar cada proceso visualmente

---

## 10.-) `main()`

**Descripción:** Función principal que coordina toda la simulación temporal del gestor de memoria.

**Flujo de ejecución:**

### Inicialización:
1. Abre archivo `simulacion_log.txt`
2. Solicita parámetros al usuario:
   - Memoria física (MB)
   - Tamaño de página (KB)
3. Calcula automáticamente:
   - Factor aleatorio entre 1.5 y 4.5
   - Memoria virtual = RAM × factor
   - Memoria SWAP = Virtual - Física
4. Llama a `inicializar_memoria()`

### Loop Principal:
Ejecuta indefinidamente hasta quedarse sin memoria:

**Cada 2 segundos:**
- Crea un nuevo proceso
- Actualiza visualización
- Incrementa contador

**A partir del segundo 30, cada 5 segundos:**
- Ejecuta política FIFO
- Termina proceso más antiguo
- Actualiza visualización

**Condición de salida:**
- Si `crear_proceso()` retorna -1 (sin memoria)

### Finalización:
1. Guarda matrices finales en el log
2. Cierra archivo de log
3. Libera memoria dinámica con `free()`
4. Termina programa

---




```c
