#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

void proceso_impares(int pipe_lectura, int pipe_escritura) {
    int n_datos;
    read(pipe_lectura, &n_datos, sizeof(int));

    int *datos = (int*)malloc(n_datos * sizeof(int));

    read(pipe_lectura, datos, n_datos * sizeof(int));

    int contador = 0;
    for (int i = 0; i < n_datos; i++) {
        if (datos[i] % 2 != 0) {
            contador++;
        }
    }

    write(pipe_escritura, &contador, sizeof(int));

    free(datos);
    close(pipe_lectura);
    close(pipe_escritura);
    exit(0);
}

void proceso_pares(int pipe_lectura, int pipe_escritura) {
    int n_datos;
    read(pipe_lectura, &n_datos, sizeof(int));

    int *datos = (int*)malloc(n_datos * sizeof(int));

    read(pipe_lectura, datos, n_datos * sizeof(int));

    int contador = 0;
    for (int i = 0; i < n_datos; i++) {
        if (datos[i] % 2 == 0) {
            contador++;
        }
    }

    write(pipe_escritura, &contador, sizeof(int));

    free(datos);
    close(pipe_lectura);
    close(pipe_escritura);
    exit(0);
}

void proceso_promedio(int pipe_lectura, int pipe_escritura) {
    int n_datos;
    read(pipe_lectura, &n_datos, sizeof(int));

    int *datos = (int*)malloc(n_datos * sizeof(int));

    read(pipe_lectura, datos, n_datos * sizeof(int));

    long suma = 0;
    for (int i = 0; i < n_datos; i++) {
        suma += datos[i];
    }
    double promedio = (double)suma / n_datos;

    write(pipe_escritura, &promedio, sizeof(double));

    free(datos);
    close(pipe_lectura);
    close(pipe_escritura);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Faltó el archivo\n");
        exit(1);
    }

    int pipe_datos_h1[2];
    int pipe_datos_h2[2];
    int pipe_datos_h3[2];

    int pipe_resultado_h1[2];
    int pipe_resultado_h2[2];
    int pipe_resultado_h3[2];

    if (pipe(pipe_datos_h1) == -1 || pipe(pipe_datos_h2) == -1 || pipe(pipe_datos_h3) == -1 || pipe(pipe_resultado_h1) == -1 || pipe(pipe_resultado_h2) == -1 || pipe(pipe_resultado_h3) == -1) {
        perror("Error al crear las tuberías");
        exit(1);
    }

    pid_t pid_h1 = fork();

    if (pid_h1 == -1) {
        perror("Error al crear proceso hijo 1");
        exit(1);
    }

    if (pid_h1 == 0) {
        close(pipe_datos_h1[1]);
        close(pipe_resultado_h1[0]);

        close(pipe_datos_h2[0]);
        close(pipe_datos_h2[1]);
        close(pipe_datos_h3[0]);
        close(pipe_datos_h3[1]);
        close(pipe_resultado_h2[0]);
        close(pipe_resultado_h2[1]);
        close(pipe_resultado_h3[0]);
        close(pipe_resultado_h3[1]);

        proceso_impares(pipe_datos_h1[0], pipe_resultado_h1[1]);
    }

    pid_t pid_h2 = fork();

    if (pid_h2 == -1) {
        perror("Error al crear proceso hijo 2");
        exit(1);
    }

    if (pid_h2 == 0) {
        close(pipe_datos_h2[1]);
        close(pipe_resultado_h2[0]);

        close(pipe_datos_h1[0]);
        close(pipe_datos_h1[1]);
        close(pipe_datos_h3[0]);
        close(pipe_datos_h3[1]);
        close(pipe_resultado_h1[0]);
        close(pipe_resultado_h1[1]);
        close(pipe_resultado_h3[0]);
        close(pipe_resultado_h3[1]);

        proceso_pares(pipe_datos_h2[0], pipe_resultado_h2[1]);
    }

    pid_t pid_h3 = fork();

    if (pid_h3 == -1) {
        perror("Error al crear proceso hijo 3");
        exit(1);
    }

    if (pid_h3 == 0) {
        close(pipe_datos_h3[1]);
        close(pipe_resultado_h3[0]);

        close(pipe_datos_h1[0]);
        close(pipe_datos_h1[1]);
        close(pipe_datos_h2[0]);
        close(pipe_datos_h2[1]);
        close(pipe_resultado_h1[0]);
        close(pipe_resultado_h1[1]);
        close(pipe_resultado_h2[0]);
        close(pipe_resultado_h2[1]);

        proceso_promedio(pipe_datos_h3[0], pipe_resultado_h3[1]);
    }

    close(pipe_datos_h1[0]);
    close(pipe_datos_h2[0]);
    close(pipe_datos_h3[0]);
    close(pipe_resultado_h1[1]);
    close(pipe_resultado_h2[1]);
    close(pipe_resultado_h3[1]);

    FILE *archivo = fopen(argv[1], "r");
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        exit(1);
    }

    int capacidad = 100;
    int n_datos = 0;
    int *datos = (int*)malloc(capacidad * sizeof(int));

    while (fscanf(archivo, "%d", &datos[n_datos]) == 1) {
        n_datos++;
        if (n_datos >= capacidad) {
            capacidad *= 2;
            datos = (int*)realloc(datos, capacidad * sizeof(int));
        }
    }
    fclose(archivo);

    printf("Archivo leído: %s\n", argv[1]);
    printf("Total de datos: %d\n", n_datos);

    // hijo1
    write(pipe_datos_h1[1], &n_datos, sizeof(int));
    write(pipe_datos_h1[1], datos, n_datos * sizeof(int));
    close(pipe_datos_h1[1]);

    // hijo2
    write(pipe_datos_h2[1], &n_datos, sizeof(int));
    write(pipe_datos_h2[1], datos, n_datos * sizeof(int));
    close(pipe_datos_h2[1]);

    // hijo3
    write(pipe_datos_h3[1], &n_datos, sizeof(int));
    write(pipe_datos_h3[1], datos, n_datos * sizeof(int));
    close(pipe_datos_h3[1]);

    int resultado_impares;
    int resultado_pares;
    double resultado_promedio;

    read(pipe_resultado_h1[0], &resultado_impares, sizeof(int));
    read(pipe_resultado_h2[0], &resultado_pares, sizeof(int));
    read(pipe_resultado_h3[0], &resultado_promedio, sizeof(double));

    close(pipe_resultado_h1[0]);
    close(pipe_resultado_h2[0]);
    close(pipe_resultado_h3[0]);

    waitpid(pid_h1, NULL, 0);
    waitpid(pid_h2, NULL, 0);
    waitpid(pid_h3, NULL, 0);

    printf("Cant. valores impares: %d\n", resultado_impares);
    printf("Cant. valores pares: %d\n", resultado_pares);
    printf("Promedio: %.2f\n", resultado_promedio);

    free(datos);
    return 0;
}