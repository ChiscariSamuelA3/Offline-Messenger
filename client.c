#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>

extern int errno;
int port;
int sd;
struct sockaddr_in server;
char mesajC[100];
char mesajS[100];
char convS[1024];
ssize_t input_bytes, output_bytes;
bool quitCommand = false;
int nr_comanda;
int tip_meniu;

bool logged = false;
char Username[20], Parola[20];

void client_read()
{
    bzero(mesajC, 100);
    scanf("%s", mesajC);
}

void firstMenu()
{
    printf("\n[client] Meniul principal:\n1. Login\n2. Register\n3. Quit\n");
    fflush(stdout);
    printf("\nIntroduceti numarul corespunzator comenzii dorite: ");
    fflush(stdout);
}

void secondMenu()
{
    printf("\n[client] Meniu:\n1. Start a chat\n2. Show message history\n3. Logout\n4. Quit\n");
    fflush(stdout);
    printf("\nIntroduceti numarul corespunzator comenzii dorite: ");
    fflush(stdout);
}

void recvMessagefromServer()
{
    bzero(mesajS, 100);
    input_bytes = read(sd, mesajS, 100);
    if(input_bytes <= 0)
    {
        perror("[client] read() : err\n");
        return;
    }

    printf("[client] : Raspunsul primit de la server... %s\n", mesajS);
}

void sendMessageToServer()
{
    output_bytes = write(sd, mesajC, 100);
    if(output_bytes <= 0)
    {
        perror("[client] write() : err\n");
        return;
    }
}

//trimite sever-ului numarul comenzii alese
void send_command()
{           
    char command[100];
    bzero(command, 100);

    sprintf(command, "comanda:%d", nr_comanda);

    output_bytes = write(sd, command, 100);
    if(output_bytes <= 0)
    {
        perror("[client] write() : err\n");
        return;
    }
}

void recvMessagefromUser(char mess[])
{
    bzero(mesajS, 100);
    input_bytes = read(sd, mesajS, 100);
    if(input_bytes <= 0)
    {
        strcpy(mess, "");
        return;
    }
}

void check_notification()
{
    //trimite username
    bzero(mesajC, 100);
    strcpy(mesajC, Username);
    sendMessageToServer();

    //primeste info de la server daca are sau nu notificari
    char mess[100];
    bzero(mess, 100);
    recvMessagefromUser(mess);

    printf("%s\n", mesajS);
    fflush(stdout);
}

void register_command()
{
    printf("--------    Incepe procesul de creare cont nou...\n");
    fflush(stdout);

    if(logged == true)
    {           
        printf("[client] Esti deja logat!\n");
        return;
    }

    while(logged == false)
    {
        send_command();

        //Username si Parola pentru noul cont creat
        printf("[client] Username: ");
        fflush(stdout);
    
        //trimite username catre server si il salveaza pt. el
        bzero(mesajC, 100);
        scanf("%s", mesajC);

        bzero(Username, 20);
        strcpy(Username, mesajC);
    
        sendMessageToServer();

        printf("[client] Password: ");
        fflush(stdout);
    
        //trimite parola catre server si o salveaza pt. el
        bzero(mesajC, 100);
        scanf("%s", mesajC);

        bzero(Parola, 20);
        strcpy(Parola, mesajC);

        sendMessageToServer();
    
        //se asteapta raspuns de la server
        recvMessagefromServer();

        if(!strcmp("Cont creat cu succes!", mesajS))
        {
            logged = true;
        }
        else 
        {
            logged = false;
            return;
        }
    }
} 

void login_command()
{
    printf("--------    Incepe procesul de logare...\n");
    fflush(stdout);

    if(logged == true)
    {
        printf("[client] Esti deja logat!\n");
        return;
    }

    while(logged == false)
    {
        send_command();

        //Username si Parola pentru noul cont creat
        printf("[client] ---- Username: ");
        fflush(stdout);
    
        //trimite username catre server si il slaveaza pt. el
        bzero(mesajC, 100);
        scanf("%s", mesajC);

        bzero(Username, 20);
        strcpy(Username, mesajC);
    
        sendMessageToServer();

        printf("[client] ---- Password: ");
        fflush(stdout);
    
        //trimite parola catre server si o salveaza pt. el
        bzero(mesajC, 100);
        scanf("%s", mesajC);

        bzero(Parola, 20);
        strcpy(Parola, mesajC);

        sendMessageToServer();
    
        //se asteapta raspuns de la server
        recvMessagefromServer();

        if(!strcmp("Ai fost logat cu succes!", mesajS))
        {
            logged = true;
        }
        else 
        {
            logged = false;
            return;
        }
    }

}

void logout_command()
{
    printf("--------    Incepe procesul de logout...\n");
    fflush(stdout);

    logged = false;

    send_command(nr_comanda);

    //trimite catre server username-ul pt. care se doreste delogarea
    bzero(mesajC, 100);
    strcpy(mesajC, Username);

    sendMessageToServer();

    recvMessagefromServer();
}

void recvConversationfromUser(char mess[])
{
    bzero(convS, 1024);
    input_bytes = read(sd, convS, 1024);
    if(input_bytes <= 0)
    {
        strcpy(mess, "");
        return;
    }
}

void start_chat_command()
{
    char destinatar[100];
    bzero(destinatar, 100);

    printf("--------    Chat-ul a fost activat!\n");
    fflush(stdout);

    send_command();
    check_notification();

    //trimite propriul username catre server
    bzero(mesajC, 100);
    strcpy(mesajC, Username);
    sendMessageToServer();

    // introduce username destinatar
    printf("[client] Introdu numele destinatarului: ");
    fflush(stdout);

    bzero(mesajC, 100);
    scanf("%s", mesajC);

    if(!strcmp(Username, mesajC))
    {
        printf("[client] Nu poti sa iti trimiti mesaje singur!\n");
        fflush(stdout);

        bzero(mesajC, 100);
        strcpy(mesajC, "propriul username");
        sendMessageToServer();

        return;
    }
   
    // trimite username destinatar catre server
    sendMessageToServer(); 
    strcpy(destinatar, mesajC);
    
    //verifica daca este sau nu valid numele destinatarului introdus
    recvMessagefromServer();

    if(!strcmp(mesajS, "destinatar valid"))
    {
        printf("\n***CONVERSATIE:***\nDin acest moment puteti introduce orice mesaj!\n#    +stop -pentru a iesi\n#    [id]+reply [message] -pentru a da reply cu [message] la mesajul [id]\n");
        fflush(stdout);

        bool cond_stop = false;

        int pid;
        pid = fork();
        if(pid == -1)
        {
            perror("fork() : err");
            return;
        }
        else if(pid == 0) // copil
        {
            while(1)
            {
                //primeste mesajul de la server
                char mess[100];
                bzero(mess, 100);

                recvMessagefromUser(mess);

                if(strstr(mesajS, "+stop"))
                {
                    exit(0);
                }

                if(strcmp(mesajS, ""))
                {   
                    printf("%s\n", mesajS);
                    fflush(stdout);
                }
            }
        }
        else // parinte
        {
            while(!cond_stop)
            {
                bzero(mesajC, 100);
                read(0, mesajC, 100);
                mesajC[strlen(mesajC) - 1] = '\0';

                if(!strcmp(mesajC, "+stop"))
                {
                    cond_stop = true;
                }

                sendMessageToServer();
            }
        }
    }
}

void history_command()
{
    printf("--------    Istoric conversatii activat...\n");
    fflush(stdout);

    send_command();

    //trimite propriul username catre server
    bzero(mesajC, 100);
    strcpy(mesajC, Username);
    sendMessageToServer();

    printf("\n---- Conversatiile lui %s: \n", Username);
    fflush(stdout);
    
    //primeste de la server userii cu care exista conversatii
    char mess[100];
    bzero(mess, 100);
    recvMessagefromUser(mess);
    
    printf("1. Mesaje proprii\n");
    fflush(stdout);
    printf("%s\n", mesajS);
    fflush(stdout);

    printf("----Introdu numarul optiunii pentru a fi afisat istoricul: ");
    fflush(stdout);
    int nr_optiune = 1;
    scanf("%d", &nr_optiune);

    printf("Ai ales optiunea %d\n\n", nr_optiune);
    fflush(stdout);

    if(nr_optiune != 1 && !strchr(mesajS, (nr_optiune + '0')))
    {
        printf("Optiune invalida!\n");
        fflush(stdout);

        //trimite optiune invalida catre server
        bzero(mesajC, 100);
        strcpy(mesajC, "optiune invalida");
        sendMessageToServer();

        return;
    }

    //trimite optiune aleasa pentru istoric conversatie
    bzero(mesajC, 100);
    sprintf(mesajC, "%d", nr_optiune);
    sendMessageToServer();

    //primeste istoricul conversatiilor alese
    bzero(mess, 100);

    recvConversationfromUser(mess);

    printf("%s\n", convS);
    fflush(stdout);


}

void quit_command()
{
    printf("--------    Se inchide Offline-Messenger...\n");
    fflush(stdout);

    send_command();

    quitCommand = true;

    //va fi deloagat...
    logged = false;


    //trimitem catre server username-ul pt care se doreste Quit
    bzero(mesajC, 100);
    strcpy(mesajC, Username);

    sendMessageToServer();

    close(sd);
}

void checkCommand()
{
    if(nr_comanda > 0 && nr_comanda < 8)
    {
        if(nr_comanda == 1)
        {
            login_command();
        }
        else if(nr_comanda == 2)
        {
            register_command();
        }
        else if(nr_comanda == 3 || nr_comanda == 7)
        {
            quit_command();
        }
        else if(nr_comanda == 4 && tip_meniu == 1)
        {
            start_chat_command();
        }
        else if (nr_comanda == 5 && tip_meniu == 1)
        {
            history_command();
        }
        else if(nr_comanda == 6 && tip_meniu == 1)
        {
            logout_command();
        }
    }
}

int main(int argc, char *argv[])
{
    
    sd = socket (AF_INET, SOCK_STREAM, 0);
    if(sd == -1)
    {
        perror("[client] socket() : err\n");
        return errno;
    }

    port = 2024;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("0.0.0.0");
    server.sin_port = htons (port);

    if(connect (sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client] connect() : err\n");
        return errno;
    }

    printf("[client] Bun venit!\n");
    fflush(stdout);
    

    while(!quitCommand)
    {
        int key = 0;
        if(logged == false)
        {
            tip_meniu = 0;
            key = 0; // ordine normala
            firstMenu();
        }
        else 
        {
            tip_meniu = 1;
            key = 3; //(nr_comanda += key) 1->4, 2->5 etc
            secondMenu();
        }

        nr_comanda = 0;
        scanf("%d", &nr_comanda);

        nr_comanda += key;

        checkCommand(nr_comanda);
    }
    
    close(sd);

}
