#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define BUFFER_SIZE 1024

void proceso_hijo1(int pipe_escritura, char *nombre_archivo) {
    FILE *archivo = fopen(nombre_archivo, "r");
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        exit(1);
    }

    char contenido[BUFFER_SIZE];
    size_t bytes_leidos = fread(contenido, 1, BUFFER_SIZE - 1, archivo);
    contenido[bytes_leidos] = '\0';
    fclose(archivo);

    printf("%s [%d]\n", contenido, getpid());
    
    write(pipe_escritura, contenido, strlen(contenido) + 1);
    
    close(pipe_escritura);
    exit(0);
}

void proceso_nieto1(int pipe_lectura, int pipe_escritura) {
    char buffer[BUFFER_SIZE];
    read(pipe_lectura, buffer, BUFFER_SIZE);
    
    printf("%s [%d]\n", buffer, getpid());
    
    write(pipe_escritura, buffer, strlen(buffer) + 1);
    
    close(pipe_lectura);
    close(pipe_escritura);
    exit(0);
}

void proceso_nieto2(int pipe_lectura, int pipe_escritura) {
    char buffer[BUFFER_SIZE];
    read(pipe_lectura, buffer, BUFFER_SIZE);
    
    printf("%s [%d]\n", buffer, getpid());
    
    write(pipe_escritura, buffer, strlen(buffer) + 1);
    
    close(pipe_lectura);
    close(pipe_escritura);
    exit(0);
}

void proceso_hijo2(int pipe_lectura_padre, int pipe_escritura_padre) {
    // Tuberías para Nieto2
    int pipe_hijo2_nieto2[2];
    int pipe_nieto2_hijo2[2];
    
    // Tuberías para Nieto1
    int pipe_hijo2_nieto1[2];
    int pipe_nieto1_hijo2[2];
    
    if (pipe(pipe_hijo2_nieto2) == -1 || pipe(pipe_nieto2_hijo2) == -1 ||
        pipe(pipe_hijo2_nieto1) == -1 || pipe(pipe_nieto1_hijo2) == -1) {
        perror("Error al crear tuberías de nietos");
        exit(1);
    }
    
    // Crear Nieto2
    pid_t pid_nieto2 = fork();
    if (pid_nieto2 == -1) {
        perror("Error al crear Nieto2");
        exit(1);
    }
    
    if (pid_nieto2 == 0) {
        // Proceso Nieto2
        close(pipe_hijo2_nieto2[1]);
        close(pipe_nieto2_hijo2[0]);
        close(pipe_hijo2_nieto1[0]);
        close(pipe_hijo2_nieto1[1]);
        close(pipe_nieto1_hijo2[0]);
        close(pipe_nieto1_hijo2[1]);
        close(pipe_lectura_padre);
        close(pipe_escritura_padre);
        
        proceso_nieto2(pipe_hijo2_nieto2[0], pipe_nieto2_hijo2[1]);
    }
    
    // Crear Nieto1
    pid_t pid_nieto1 = fork();
    if (pid_nieto1 == -1) {
        perror("Error al crear Nieto1");
        exit(1);
    }
    
    if (pid_nieto1 == 0) {
        // Proceso Nieto1
        close(pipe_hijo2_nieto1[1]);
        close(pipe_nieto1_hijo2[0]);
        close(pipe_hijo2_nieto2[0]);
        close(pipe_hijo2_nieto2[1]);
        close(pipe_nieto2_hijo2[0]);
        close(pipe_nieto2_hijo2[1]);
        close(pipe_lectura_padre);
        close(pipe_escritura_padre);
        
        proceso_nieto1(pipe_hijo2_nieto1[0], pipe_nieto1_hijo2[1]);
    }
    
    // Proceso Hijo2 - Cerrar extremos no usados
    close(pipe_hijo2_nieto2[0]);
    close(pipe_nieto2_hijo2[1]);
    close(pipe_hijo2_nieto1[0]);
    close(pipe_nieto1_hijo2[1]);
    
    // 1. Recibir del Padre
    char buffer[BUFFER_SIZE];
    read(pipe_lectura_padre, buffer, BUFFER_SIZE);
    
    printf("%s [%d]\n", buffer, getpid());
    
    // 2. Enviar a Nieto2
    write(pipe_hijo2_nieto2[1], buffer, strlen(buffer) + 1);
    close(pipe_hijo2_nieto2[1]);
    
    // 3. Recibir de Nieto2
    read(pipe_nieto2_hijo2[0], buffer, BUFFER_SIZE);
    close(pipe_nieto2_hijo2[0]);
    
    printf("%s [%d]\n", buffer, getpid());
    
    // 4. Enviar a Nieto1
    write(pipe_hijo2_nieto1[1], buffer, strlen(buffer) + 1);
    close(pipe_hijo2_nieto1[1]);
    
    // 5. Recibir de Nieto1
    read(pipe_nieto1_hijo2[0], buffer, BUFFER_SIZE);
    close(pipe_nieto1_hijo2[0]);
    
    printf("%s [%d]\n", buffer, getpid());
    
    // 6. Enviar de vuelta al Padre
    write(pipe_escritura_padre, buffer, strlen(buffer) + 1);
    
    // Esperar a los nietos
    waitpid(pid_nieto2, NULL, 0);
    waitpid(pid_nieto1, NULL, 0);
    
    close(pipe_lectura_padre);
    close(pipe_escritura_padre);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <archivo.txt>\n", argv[0]);
        exit(1);
    }
    
    // Tuberías para Hijo1
    int pipe_hijo1_padre[2];
    
    // Tuberías para Hijo2
    int pipe_padre_hijo2[2];
    int pipe_hijo2_padre[2];
    
    if (pipe(pipe_hijo1_padre) == -1 || pipe(pipe_padre_hijo2) == -1 || pipe(pipe_hijo2_padre) == -1) {
        perror("Error al crear las tuberías");
        exit(1);
    }
    
    // Crear Hijo1
    pid_t pid_hijo1 = fork();
    if (pid_hijo1 == -1) {
        perror("Error al crear Hijo1");
        exit(1);
    }
    
    if (pid_hijo1 == 0) {
        // Proceso Hijo1
        close(pipe_hijo1_padre[0]);
        close(pipe_padre_hijo2[0]);
        close(pipe_padre_hijo2[1]);
        close(pipe_hijo2_padre[0]);
        close(pipe_hijo2_padre[1]);
        
        proceso_hijo1(pipe_hijo1_padre[1], argv[1]);
    }
    
    // Crear Hijo2
    pid_t pid_hijo2 = fork();
    if (pid_hijo2 == -1) {
        perror("Error al crear Hijo2");
        exit(1);
    }
    
    if (pid_hijo2 == 0) {
        // Proceso Hijo2
        close(pipe_hijo1_padre[0]);
        close(pipe_hijo1_padre[1]);
        close(pipe_padre_hijo2[1]);
        close(pipe_hijo2_padre[0]);
        
        proceso_hijo2(pipe_padre_hijo2[0], pipe_hijo2_padre[1]);
    }
    
    // Proceso Padre - Cerrar extremos no usados
    close(pipe_hijo1_padre[1]);
    close(pipe_padre_hijo2[0]);
    close(pipe_hijo2_padre[1]);
    
    // 1. Recibir de Hijo1
    char buffer[BUFFER_SIZE];
    read(pipe_hijo1_padre[0], buffer, BUFFER_SIZE);
    close(pipe_hijo1_padre[0]);
    
    printf("%s [%d]\n", buffer, getpid());
    
    // 2. Enviar a Hijo2
    write(pipe_padre_hijo2[1], buffer, strlen(buffer) + 1);
    close(pipe_padre_hijo2[1]);
    
    // 3. Recibir de Hijo2 (después de todo el proceso)
    read(pipe_hijo2_padre[0], buffer, BUFFER_SIZE);
    close(pipe_hijo2_padre[0]);
    
    printf("%s [%d]\n", buffer, getpid());
    
    // Esperar a los hijos
    waitpid(pid_hijo1, NULL, 0);
    waitpid(pid_hijo2, NULL, 0);
    
    return 0;
}
