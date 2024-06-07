#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "imprimir.h"
#include <rpc/rpc.h>


pthread_mutex_t mutex;
pthread_cond_t cond;
int mensaje_no_copiado = 1;

pthread_mutex_t file_mutex;

void register_user(int s_local, char* user)
{
    printf("s> OPERATION REGISTER FROM %s\n",user);
    FILE *f;
    pthread_mutex_lock(&file_mutex); 
    f = fopen("users.txt", "r");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        pthread_mutex_unlock(&file_mutex); 
        exit(1);
    }
    // Comprueba si el usuario ya está registrado
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // Elimina el salto de línea al final de la línea
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, user) == 0) {
            // El usuario ya está registrado, envía '1' al cliente
            send(s_local, "1", 1, 0);
            fclose(f);
            pthread_mutex_unlock(&file_mutex); 
            return;
        }
    }
    fclose(f);

    // Registra al nuevo usuario y envía '0' al cliente
    f = fopen("users.txt", "a");
    fprintf(f, "%s\n", user);
    send(s_local, "0", 1, 0);
    fclose(f);
    pthread_mutex_unlock(&file_mutex); 
    close(s_local);
    return;
}

void unregister_user(int s_local, char* user)
{
    printf("s> OPERATION UNREGISTER FROM %s\n",user);
    FILE *f;
    pthread_mutex_lock(&file_mutex); 
    f = fopen("users.txt", "r");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        pthread_mutex_unlock(&file_mutex); 
        exit(1);
    }
    // Comprueba si el usuario ya está registrado, si es así lo borra
    char line[256];
    int isRegistered = 0;  // Variable para rastrear si el usuario está registrado
    while (fgets(line, sizeof(line), f)) {
        // Elimina el salto de línea al final de la línea
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, user) == 0) {
            isRegistered = 1;  // El usuario está registrado
            // El usuario ya está registrado, envía '0' al cliente
            send(s_local, "0", 1, 0);
            fclose(f);
            // Borra el usuario
            FILE *f2;
            f2 = fopen("users.txt", "r");
            FILE *f3;
            f3 = fopen("users2.txt", "w");
            while (fgets(line, sizeof(line), f2)) {
                // Elimina el salto de línea al final de la línea
                line[strcspn(line, "\n")] = 0;
                if (strcmp(line, user) != 0) {
                    fprintf(f3, "%s\n", line);
                }
            }
            fclose(f2);
            fclose(f3);
            remove("users.txt");
            rename("users2.txt", "users.txt");

            // eliminar las entradas de publish_contents.txt que ha publicado el usuario que hace el unregister
            FILE *f4;
            f4 = fopen("published_contents.txt", "r");
            FILE *f5;
            f5 = fopen("published_contents2.txt", "w");
            while (fgets(line, sizeof(line), f4)) {
            // Elimina el salto de línea al final de la línea
            char temp_line[1024];
            strcpy(temp_line, line);
            temp_line[strcspn(temp_line, "\n")] = 0;

            char* published_user = strtok(temp_line, " ");
            if (strcmp(published_user, user) != 0) {
                line[strcspn(line, "\n")] = 0;  // también elimina el salto de línea de 'line'
                fprintf(f5, "%s\n", line);
            }
        
            }
            fclose(f4);
            fclose(f5);
            remove("published_contents.txt");
            rename("published_contents2.txt", "published_contents.txt");

            pthread_mutex_unlock(&file_mutex); 
            close(s_local);
            return;
        }
    }
    fclose(f);
    if (!isRegistered) {  // Si el usuario no está registrado
        send(s_local, "1", 1, 0);  // Envía '1' al cliente
    }
    pthread_mutex_unlock(&file_mutex); 
    close(s_local);
    return;
}

void connect_user(int s_local, char* user, char* ip, char* port)
{
    printf("s> OPERATION CONNECT FROM %s\n",user);
    FILE *f;
    pthread_mutex_lock(&file_mutex); 
    f = fopen("users.txt", "r");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        pthread_mutex_unlock(&file_mutex); 
        exit(1);
    }
    // Comprueba si el usuario ya está registrado
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // Elimina el salto de línea al final de la línea
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, user) == 0) {
            fclose(f);

            // Comprueba si el usuario ya está conectado
            f = fopen("connected_users.txt", "r");
            if (f != NULL) {
                while (fgets(line, sizeof(line), f)) {
                    line[strcspn(line, "\n")] = 0;
                    char* data = strtok(line, " ");
                    if (strcmp(data, user) == 0) {
                        // El usuario ya está conectado, envía '2' al cliente
                        send(s_local, "2", 1, 0);
                        fclose(f);
                        pthread_mutex_unlock(&file_mutex); 
                        return;
                    }
                }
                fclose(f);
            }

            // Conecta al usuario y envía '0' al cliente
            f = fopen("connected_users.txt", "a");
            fprintf(f, "%s %s %s \n", user, ip, port);
            send(s_local, "0", 1, 0);
            fclose(f);
            pthread_mutex_unlock(&file_mutex); 
            return;
        }
    }
    fclose(f);

    // El usuario no está registrado, envía '1' al cliente
    send(s_local, "1", 1, 0);
    pthread_mutex_unlock(&file_mutex); 
    close(s_local);
    return;
}

void publish_content(int s_local, char* user, char* filename, char* description)
{
    printf("s> OPERATION PUBLISH\n");
    FILE *f;
    pthread_mutex_lock(&file_mutex); 
    f = fopen("connected_users.txt", "r");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        pthread_mutex_unlock(&file_mutex); 
        exit(1);
    }

    // Comprueba si el usuario ya está registrado
    f = fopen("users.txt", "r");
    if (f != NULL) {
        char line[256];
        bool user_exists = false;
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            if (strcmp(line, user) == 0) {
                user_exists = true;
                break;
            }
        }
        fclose(f);
        if (!user_exists) {
            // El usuario no está registrado, envía '1' al cliente
            send(s_local, "1", 1, 0);
            pthread_mutex_unlock(&file_mutex); 
            close(s_local);
            return;
        }
    }

    // Comprueba si el usuario ya está conectado
    f = fopen("connected_users.txt", "r");
    if (f != NULL) {
        char line[256];
        bool user_connected = false;
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            char* data = strtok(line, " ");
            if (strcmp(data, user) == 0) {
                user_connected = true;
                break;
            }
        }
        fclose(f);
        if (!user_connected) {
            // El usuario no está conectado, envía '2' al cliente
            send(s_local, "2", 1, 0);
            pthread_mutex_unlock(&file_mutex); 
            close(s_local);
            return;
        }
    }

    // Comprueba si el contenido ya está publicado
    f = fopen("published_contents.txt", "r");
    if (f != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            strtok(line, " ");
            char* published_filename = strtok(NULL, " ");
            if (strcmp(published_filename, filename) == 0) {
                // El contenido ya está publicado, envía '3' al cliente
                send(s_local, "3", 1, 0);
                fclose(f);
                pthread_mutex_unlock(&file_mutex); 
                close(s_local);
                return;
            }
        }
        fclose(f);
    }

    // Publica el contenido y envía '0' al cliente
    f = fopen("published_contents.txt", "a");
    fprintf(f, "%s %s %s \n", user, filename, description);
    send(s_local, "0", 1, 0);
    fclose(f);
    pthread_mutex_unlock(&file_mutex); 
    close(s_local);
    return;
}

void delete_content(int s_local, char* user, char* filename)
{
    printf("s> OPERATION DELETE\n");
    pthread_mutex_lock(&file_mutex); 
    FILE *f, *temp;
    char line[256];
    bool file_exists = false;

    // Comprueba si el usuario ya está registrado
    f = fopen("users.txt", "r");
    if (f != NULL) {
        bool user_exists = false;
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            if (strcmp(line, user) == 0) {
                user_exists = true;
                break;
            }
        }
        fclose(f);
        if (!user_exists) {
            // El usuario no está registrado, envía '1' al cliente
            send(s_local, "1", 1, 0);
            pthread_mutex_unlock(&file_mutex); 
            close(s_local);
            return;
        }
    }

    // Comprueba si el usuario ya está conectado
    f = fopen("connected_users.txt", "r");
    if (f != NULL) {
        bool user_connected = false;
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            char* data = strtok(line, " ");
            if (strcmp(data, user) == 0) {
                user_connected = true;
                break;
            }
        }
        fclose(f);
        if (!user_connected) {
            // El usuario no está conectado, envía '2' al cliente
            send(s_local, "2", 1, 0);
            pthread_mutex_unlock(&file_mutex); 
            close(s_local);
            return;
        }
    }

    // Comprueba si el contenido está publicado
    f = fopen("published_contents.txt", "r");
    temp = fopen("temp.txt", "w");
    if (f != NULL) {
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            char* published_user = strtok(line, " ");
            char* published_filename = strtok(NULL, " ");
            if (strcmp(published_filename, filename) == 0 && strcmp(published_user, user) == 0) {
                // El contenido está publicado por el usuario, no lo escribas en el archivo temporal
                file_exists = true;
            } else {
                // Escribe la línea en el archivo temporal
                fprintf(temp, "%s %s %s\n", published_user, published_filename, strtok(NULL, "\n"));
            }
        }
        fclose(f);
        fclose(temp);
        // Elimina el archivo original y renombra el archivo temporal
        remove("published_contents.txt");
        rename("temp.txt", "published_contents.txt");
        if (!file_exists) {
            // El contenido no está publicado, envía '3' al cliente
            send(s_local, "3", 1, 0);
            pthread_mutex_unlock(&file_mutex); 
            close(s_local);
            return;
        }
    } else {
        // Error al abrir el archivo
        printf("Error opening file!\n");
        pthread_mutex_unlock(&file_mutex); 
        exit(1);
    }

    // Elimina el contenido y envía '0' al cliente
    send(s_local, "0", 1, 0);
    pthread_mutex_unlock(&file_mutex); 
    close(s_local);
    return;
}

void list_users(int s_local, char* user)
{
    printf("s> OPERATION LIST_USERS\n");
    FILE *f;
    pthread_mutex_lock(&file_mutex); 
    f = fopen("users.txt", "r");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        pthread_mutex_unlock(&file_mutex); 
        exit(1);
    }
    // Comprueba si el usuario ya está registrado
    char line[256];
    bool user_exists = false;
    while (fgets(line, sizeof(line), f)) {
        // Elimina el salto de línea al final de la línea
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, user) == 0) {
            user_exists = true;
            break;
        }
    }
    fclose(f);
    if (!user_exists) {
        // El usuario no está registrado, envía '1' al cliente
        send(s_local, "1", 1, 0);
        pthread_mutex_unlock(&file_mutex); 
        close(s_local);
        return;
    }

    // Comprueba si el usuario ya está conectado
    f = fopen("connected_users.txt", "r");
    if (f != NULL) {
        bool user_connected = false;
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            char* connected_user = strtok(line, " ");
            if (strcmp(connected_user, user) == 0) {
                user_connected = true;
                break;
            }
        }
        fclose(f);
        if (!user_connected) {
            // El usuario no está conectado, envía '2' al cliente
            send(s_local, "2", 1, 0);
            pthread_mutex_unlock(&file_mutex); 
            close(s_local);
            return;
        }
    }

    // Lista los usuarios conectados y envía '0' al cliente
    f = fopen("connected_users.txt", "r");
    if (f != NULL) {
        int num_users = 0;
        char message[1024] = "0";
        while (fgets(line, sizeof(line), f)) {
            num_users++;
        }
        rewind(f);
        
        char num_users_str[10];
        sprintf(num_users_str, "%d", num_users);
        strcat(message, num_users_str);
        
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            strcat(message, line);
        }
        fclose(f);
        send(s_local, message, strlen(message) + 1, 0);
    }
    pthread_mutex_unlock(&file_mutex); 
    close(s_local);
    return;

}

void list_content(int s_local, char* user, char* target_user) {
    printf("s> OPERATION LIST_CONTENT\n");
    FILE *f;
    pthread_mutex_lock(&file_mutex); 
    f = fopen("users.txt", "r");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        pthread_mutex_unlock(&file_mutex); 
        exit(1);
    }
        // Comprueba si el usuario ya está registrado
    char line[256];
    bool user_exists = false;
    bool target_user_exists = false;
    while (fgets(line, sizeof(line), f)) {
        // Elimina el salto de línea al final de la línea
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, user) == 0) {
            user_exists = true;
        }
        if (strcmp(line, target_user) == 0) {
            target_user_exists = true;
        }
    }
    fclose(f);
    if (!user_exists) {
        // El usuario no está registrado, envía '1' al cliente
        send(s_local, "1", 1, 0);
        pthread_mutex_unlock(&file_mutex); 
        close(s_local);
        return;
    }
    if (!target_user_exists) {
        // El target_user no existe, envía '3' al cliente
        send(s_local, "3", 1, 0);
        pthread_mutex_unlock(&file_mutex); 
        close(s_local);
        return;
    }

    // Comprueba si el usuario ya está conectado
    f = fopen("connected_users.txt", "r");
    if (f != NULL) {
        bool user_connected = false;
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            char* connected_user = strtok(line, " ");
            if (strcmp(connected_user, user) == 0) {
                user_connected = true;
                break;
            }
        }
        fclose(f);
        if (!user_connected) {
            // El usuario no está conectado, envía '2' al cliente
            send(s_local, "2", 1, 0);
            pthread_mutex_unlock(&file_mutex); 
            close(s_local);
            return;
        }
    }
 
    // Lista los ficheros del usuario y envía '0' al cliente
    f = fopen("published_contents.txt", "r");
    if (f != NULL) {
        int num_files = 0;
        char message[2048] = "0";
        char* token;
        while (fgets(line, sizeof(line), f)) {
            char temp_line[256];
            strcpy(temp_line, line);  // copia la línea a una variable temporal
            temp_line[strcspn(temp_line, "\n")] = 0;

            token = strtok(temp_line, " ");
            if (strcmp(token, target_user) == 0) {
                num_files++;
                strcat(message, "\0");  // añade un carácter de separación
                token = strtok(NULL, "\n");  // obtén el resto de la línea
                if (token != NULL) {
                    strcat(message, token);  // añade el nombre del archivo y la descripción al mensaje
                }
            }
        }    
        fclose(f);
        send(s_local, message, strlen(message) + 1, 0);
    }
    pthread_mutex_unlock(&file_mutex); 
    close(s_local);
    return;

}


void disconnect_user(int s_local, char* user)
{
    printf("s> OPERATION DISCONNECT FROM %s\n",user);
    pthread_mutex_lock(&file_mutex); 
    FILE *f, *temp;
    char line[256];
    bool user_exists = false;
    bool user_connected = false;
    // Comprueba si el usuario está en users.txt
    f = fopen("users.txt", "r");
    if (f != NULL) {
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;  // Elimina el salto de línea al final de la línea
            if (strcmp(line, user) == 0) {
                user_exists = true;
                break;
            }
        }
        fclose(f);
    } else {
        printf("Error opening file!\n");
        pthread_mutex_unlock(&file_mutex); 
        exit(1);
    }

    if (!user_exists) {
        // El usuario no existe, envía '1' al cliente
        send(s_local, "1", 1, 0);
        pthread_mutex_unlock(&file_mutex); 
        close(s_local);
        return;
    }

    // Comprueba si el usuario está conectado
    f = fopen("connected_users.txt", "r");
    temp = fopen("temp.txt", "w");
    if (f != NULL) {
        while (fgets(line, sizeof(line), f)) {
            char temp_line[256];
            strcpy(temp_line, line);  // copia la línea a una variable temporal
            temp_line[strcspn(temp_line, "\n")] = 0;
            
            char* connected_user = strtok(temp_line, " ");
            if (strcmp(connected_user, user) == 0) {
                // El usuario está conectado, no lo escribas en el archivo temporal
                user_connected = true;
            } else {
                // Escribe la línea en el archivo temporal
                fprintf(temp, "%s", line);
            }
        }
        fclose(f);
        fclose(temp);
        // Elimina el archivo original y renombra el archivo temporal
        remove("connected_users.txt");
        rename("temp.txt", "connected_users.txt");
        if (!user_connected) {
            // El usuario no está conectado, envía '1' al cliente
            send(s_local, "2", 1, 0);
            pthread_mutex_unlock(&file_mutex); 
            close(s_local);
            return;
        } else {
            // El usuario está conectado, envía '0' al cliente
            send(s_local, "0", 1, 0);
            pthread_mutex_unlock(&file_mutex); 
            close(s_local);
            return;
        }
    } else {
        // Error al abrir el archivo
        printf("Error opening file!\n");
        pthread_mutex_unlock(&file_mutex); 
        exit(1);
    }
    pthread_mutex_unlock(&file_mutex); 
}

void get_file(int s_local, char* user, char* r_file){
    printf("s> OPERATION GET_FILE FROM %s\n",user);

    FILE *f;
    pthread_mutex_lock(&file_mutex); 
    f = fopen("users.txt", "r");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        pthread_mutex_unlock(&file_mutex); 
        exit(1);
    }
    // Comprueba si el usuario ya está registrado
    char line[256];
    bool user_exists = false;
    while (fgets(line, sizeof(line), f)) {
        // Elimina el salto de línea al final de la línea
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, user) == 0) {
            user_exists = true;
            break;
        }
    }
    fclose(f);
    if (!user_exists) {
        // El usuario no está registrado, envía '1' al cliente
        send(s_local, "1", 1, 0);
        pthread_mutex_unlock(&file_mutex); 
        close(s_local);
        return;
    }

    // Comprobar que el archivo este publidacado por el usuario
    f = fopen("published_contents.txt", "r");
    if (f != NULL) {
        char line[256];
        bool file_exists = false;
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            char* published_user = strtok(line, " ");
            char* published_filename = strtok(NULL, " ");
            if (strcmp(published_filename, r_file) == 0 && strcmp(published_user, user) == 0) {
                file_exists = true;
                break;
            }
        }
        fclose(f);
        if (!file_exists) {
            // El archivo no está publicado por el usuario, envía '2' al cliente
            send(s_local, "2", 1, 0);
            pthread_mutex_unlock(&file_mutex); 
            close(s_local);
            return;
        }
    } else {
        // Error al abrir el archivo
        printf("Error opening file!\n");
        pthread_mutex_unlock(&file_mutex); 
        exit(1);
    }

    // Comprueba si el usuario ya está conectado y envial al cliente el '0' junto con la ip y el puerto de dicho usuario
    f = fopen("connected_users.txt", "r");
    if (f != NULL) {
        bool user_connected = false;
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            char* connected_user = strtok(line, " ");
            if (strcmp(connected_user, user) == 0) {
                user_connected = true;
                char* ip = strtok(NULL, " ");
                char* port = strtok(NULL, " ");
                char message[256];
                strcpy(message, "0 ");
                strcat(message, ip);
                strcat(message, " ");
                strcat(message, port);
                send(s_local, message, strlen(message) + 1, 0);
                fclose(f);
                pthread_mutex_unlock(&file_mutex); 
                close(s_local);
                return;
            }
        }
        fclose(f);
        if (!user_connected) {
            // El usuario no está conectado, envía '2' al cliente
            send(s_local, "2", 1, 0);
            pthread_mutex_unlock(&file_mutex); 
            close(s_local);
            return;
        }
    } else {
        // Error al abrir el archivo
        printf("Error opening file!\n");
        pthread_mutex_unlock(&file_mutex); 
        exit(1);
    }
    pthread_mutex_unlock(&file_mutex); 
    return;
}



void tratar_peticion(void *sockfd)
{
    int *sockfd_int = (int *)sockfd;
    int err;

    int n;
    
    char buffer[1024];

    int s_local;
    pthread_mutex_lock(&mutex);
    s_local = (* (int *)sockfd);
    mensaje_no_copiado = 0;

    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&mutex);
    
    err = recv(s_local, buffer, 1024, 0); 
    if (err == -1)
    {
        printf("Error in reception\n");
        close(s_local);
        return;
    }

    char *op = strtok(buffer, " ");
    char *fecha = strtok(NULL, " ");
    char *hora = strtok(NULL, " ");
    

    char *user = strtok(NULL, " ");
    printf("s> USER %s\n", user);
    
    CLIENT *clnt;
    enum clnt_stat retval;

    char *host = getenv("IMPRIMIR_SERVER");

    clnt = clnt_create(host, IMPRIMIR, IMPRIMIR_V1, "tcp");
    if (clnt == NULL) {
        clnt_pcreateerror(host);
        exit(1);
    }

    if (op && strcmp(op, "REGISTER") == 0){ //REGISTER
        if (user) {
            retval = imprimir_nf_1(op, fecha, hora, user, &n, clnt);
            if (retval != RPC_SUCCESS) {
                clnt_perror(clnt, "call failed");
            }
            register_user(s_local, user);
        } else {
            printf("No user provided\n");
        }
    }
    else if (op && strcmp(op, "UNREGISTER") == 0){ //UNREGISTER
        if (user) {
            retval = imprimir_nf_1(op, fecha, hora, user, &n, clnt);
            if (retval != RPC_SUCCESS) {
                clnt_perror(clnt, "call failed");
            }
            unregister_user(s_local, user);
        } else {
            printf("No user provided\n");
        }
    }
    else if (op && strcmp(op, "CONNECT") == 0){ //CONNECT
        retval = imprimir_nf_1(op, fecha, hora, user, &n, clnt);
        if (retval != RPC_SUCCESS) {
            clnt_perror(clnt, "call failed");
        }
        char *ip = strtok(NULL, " ");
        printf("s> IP %s\n", ip);
        char *port = strtok(NULL, " ");
        printf("s> PORT %s\n", port);
        if (user) {
            connect_user(s_local, user, ip, port);
        } else {
            printf("No user provided\n");
        }
    }
    else if (op && strcmp(op, "DISCONNECT") == 0){ //DISCONNECT
        retval = imprimir_nf_1(op, fecha, hora, user, &n, clnt);
        if (retval != RPC_SUCCESS) {
            clnt_perror(clnt, "call failed");
        }
        if (user) {
            disconnect_user(s_local, user);
        } else {
            printf("No user provided\n");
        }
    }
    else if (op && strcmp(op, "PUBLISH") == 0){ //PUBLISH
        char *filename = strtok(NULL, " ");
        printf("s> FILENAME %s\n", filename);
        char *description = strtok(NULL, "");
        printf("s> DESCRIPTION %s\n", description);
        retval = imprimir_f_1(op, fecha, hora, user, filename, &n, clnt);
        if (retval != RPC_SUCCESS) {
            clnt_perror(clnt, "call failed");
        }
        if (user && filename && description) {
            publish_content(s_local, user, filename, description);
        } else {
            printf("No filename or description provided\n");
        }
    }
    else if (op && strcmp(op, "DELETE") == 0){ //DELETE
        char *filename = strtok(NULL, " ");
        printf("s> FILENAME %s\n", filename);
        retval = imprimir_f_1(op, fecha, hora, user, filename, &n, clnt);
        if (retval != RPC_SUCCESS) {
            clnt_perror(clnt, "call failed");
        }
        if (user && filename) {
            delete_content(s_local, user, filename);
        } else {
            printf("No filename or description provided\n");
        }
    }
    else if (op && strcmp(op, "LIST_USERS") == 0){ //LIST_USERS
        retval = imprimir_nf_1(op, fecha, hora, user, &n, clnt);
        if (retval != RPC_SUCCESS) {
            clnt_perror(clnt, "call failed");
        }
        if (user) {
            list_users(s_local, user);
        } else {
            printf("No user provided\n");
        }
    }
    else if (op && strcmp(op, "LIST_CONTENT") == 0){ //LIST_CONTENT
        retval = imprimir_nf_1(op, fecha, hora, user, &n, clnt);
        if (retval != RPC_SUCCESS) {
            clnt_perror(clnt, "call failed");
        }
        char *user2 = strtok(NULL, " ");
        printf("s> USER connected %s\n", user2);
        if (user) {
            list_content(s_local, user2, user);
        } else {
            printf("No user provided\n");
        }
    }
    else if (op && strcmp(op, "GET_FILE") == 0){ //GET_FILE
        if (retval != RPC_SUCCESS) {
            clnt_perror(clnt, "call failed");
        }
        char *target = strtok(NULL, " ");
        printf("s> TARGET %s\n", target);
        char *r_file = strtok(NULL, " ");
        printf("s> FILE %s\n", r_file);
        retval = imprimir_f_1(op, fecha, hora, user, r_file, &n, clnt);

        if (user) {
            get_file(s_local, target, r_file);
        } else {
            printf("No user provided\n");
        }
    }
    else {
        printf("Invalid operation\n");
    }
    close(s_local);
    pthread_exit(NULL);
}




int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: servidor.c -p P\n");
        return 0;
    }
    struct sockaddr_in server_addr, client_addr;
    socklen_t size;
    int sd, sc;
    int err;
    pthread_t thid;
    pthread_attr_t attr;


    // Inicializar el atributo
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // Crear el socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("SERVER: Error en el socket");
        return (0);
    }

    // Permite que se reuse el socket
    int val = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(int));

    // Inicializar
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind
    err = bind(sd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err == -1)
    {
        printf("Error en bind\n");
        return -1;
    }

    // Listen
    err = listen(sd, SOMAXCONN);
    if (err == -1)
    {
        printf("Error en listen\n");
        return -1;
    }

    size = sizeof(client_addr);

    printf("s> init server <%s>: <%d>\n", inet_ntoa(server_addr.sin_addr), atoi(argv[2]));

    // Aceptar la conexión en sí
    while (1)
    {
        // Nuevo socket
        sc = accept(sd, (struct sockaddr *)&client_addr, (socklen_t *)&size);
        if (sc == -1)
        {
            printf("Error en accept\n");
            return -1;
        }

        printf("s> new connection from %s\n", inet_ntoa(client_addr.sin_addr));

        if(pthread_create(&thid, &attr, (void *)tratar_peticion, (int *) &sc) == 0){
            pthread_mutex_lock(&mutex);
            while (mensaje_no_copiado)
                pthread_cond_wait(&cond, &mutex);
            mensaje_no_copiado = 1;
            pthread_mutex_unlock(&mutex);
        }
        else{
            perror("s > error creación de hilo");
            return -1;
        }
    }
    
    close(sd);
    return 0;
}