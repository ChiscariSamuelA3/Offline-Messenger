#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>

#define PORT 2024

extern int errno;

int nrClienti;
struct sockaddr_in server; // structura server-ului
struct sockaddr_in from;
    
char mesajC[100]; // mesaj de la client
char mesajS[100] = " "; // mesaj catre client
char convS[1024] = " "; // conversatie
ssize_t input_bytes, output_bytes;
int nrUseri;
bool finished; // comanda executata cu succes si corect
bool logged = false;

bool quitCommand = false; 

struct infoUser
{
    int id; 
    char username[20], parola[20];
    bool online;

}users[256];

// cale pentru fisierele temporare ce contin mesajele necitite
void createPath(char cale[], char user1[], char user2[])
{
    strcpy(cale, "./usersConversations/");
    strcat(cale, user1);
    strcat(cale, "/");
    strcat(cale, user1);
    strcat(cale, "_TEMP_");
    strcat(cale, user2);
}

// cale pentru fisierele ce contin conversatii
void historyFilePath(char cale[], char user1[], char user2[])
{
    strcpy(cale, "./usersConversations/");
    strcat(cale, user1);
    strcat(cale, "/");
    strcat(cale, user1);
    strcat(cale, "_");
    strcat(cale, user2);
}

void init_file()
{   
    char path[256];

    for(int i = 0; i < nrUseri; i++)
    {
        for(int j = 0; j < nrUseri; j++)
        {
            if(strcmp(users[i].username, users[j].username))
            {
                bzero(path, 256);
                createPath(path, users[i].username, users[j].username);

                if( access(path, F_OK ) == 0 ) 
                {
                    //daca in fisierul temporar e doar mesajul "0= +stop@" - se sterge fisierul
                    FILE* file;
                    file = fopen(path, "r");
                    char line[256];
                    char* p;

                    if(file == NULL)
                    {
                        perror("fopen() error: ");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        int nr_line = 0;
                        int gasit = 0;
                        while(fgets(line, 256, file))
                        {
                            nr_line++;

                            if(!strcmp(line, "0= +stop@"))
                            {
                                gasit = 1;
                            }
                        }
                        if(nr_line == 1 && gasit == 1)
                        {
                            fclose(file);
                            remove(path);
                        }
                        else
                        {
                            fclose(file);
                        }
                    }
                    
                }
                
            }
        }
    } 
}

void initializareUseri(char* nume_fisier)
{
    FILE* file;
    file = fopen(nume_fisier, "r");
    char line[256], *p;

    if(file == NULL)
    {
        perror("fopen() error: ");
        exit(EXIT_FAILURE);
    }
    else
    {
        while(fgets(line, 256, file))
        {
            //linie citita -> user existent => primul user este 0, etc

            p = strtok(line, "\n"); // contine: 'username parola'
            char aux[256];
            bzero(aux, 256);

            strcpy(aux, p);

            // initializare user

            p = strtok(aux, " "); // username
            strcpy(users[nrUseri].username, p); 
            p = strtok(NULL, " "); // parola
            strcpy(users[nrUseri].parola, p);
            users[nrUseri].online = 0; // stare

            nrUseri++;
        }
        printf("%d Useri Offline-Mesenger:\n\n", nrUseri);
        fflush(stdout);
        for(int i = 0; i < nrUseri; i++)
        {
            printf("Username: %s --- Password: %s\n", users[i].username, users[i].parola);
        }
        printf("------------------------------\n");
    }

    fclose(file);

    init_file();
}

bool verificare_username(char* nume_fisier, char utilizator[])
{
    FILE* file;
    file = fopen(nume_fisier, "r");
    char line[256];
    char* p;

    if(file == NULL)
    {
        perror("fopen() error: ");
        exit(EXIT_FAILURE);
    }
    else
    {
        while(fgets(line, 256, file))
        {
            
            p = strtok(line, " "); // username-ul

            if(!strcmp(p, utilizator))
            {
                fclose(file);
                return true;
            }
        }
    }

    fclose(file);
    return false;
}

bool verificare_username_parola(char* nume_fisier, char username[], char parola[])
{
    FILE* file;
    file = fopen(nume_fisier, "r");
    char line[256];
    char* p;

    if(file == NULL)
    {
        perror("fopen() error: ");
        exit(EXIT_FAILURE);
    }
    else
    {
        int index_user = 0;
        while(fgets(line, 256, file))
        {
            bool un = false, ps = false;

            //contine: 'username parola'
            p = strtok(line, "\n"); 

            char aux[256];
            bzero(aux, 256);

            strcpy(aux, p);

            p = strtok(aux, " "); // username
            if(!strcmp(p, username))
            {
                un = true; // am gasit unsername-ul
                printf("A fost gasit username-ul %s!\n", username);
            }
            
            p = strtok(NULL, " "); // parola
            if(un == true) // daca a fost gasit username-ul
            {
                if(!strcmp(p, parola)) // parola corecta 
                {
                    ps = true;
                    printf("Parola %s corecta!\n", parola);
                }
            }

            if(un == true && ps == true)
            {
                //actualizare stare pt. user-ul care s-a logat
                users[index_user].online = 1; 

                fclose(file);
                return true;
            }
            
            index_user++;
        }
    }

    fclose(file);
    return false;
}


void adauga_stats(int stare, int fd)
{
    FILE* file;
    file = fopen("users_stats", "a");
    if(file == NULL)
    {
        perror("[server] fopen() : err");
        return;
    }
    fprintf(file, "%d", stare);
    fprintf(file, " %d\n", fd);

    fclose(file);
}


void adauga_user(char* nume_fisier, char username[], char parola[])
{
   
    FILE* file;
    file = fopen(nume_fisier, "a");
    if(file == NULL)
    {
        perror("[server] fopen() : err");
        return;
    }

    //adaugam in fisierul users_conifg username si parola user
    fprintf(file, "%s", username);
    fprintf(file, " %s\n", parola);


    //fiecare user va avea propriul folder de fisiere ce vor contine conversatiile cu alti useri
    char cale[256]; 
    strcpy(cale, "./usersConversations/");
    strcat(cale, username);
    
    if(mkdir(cale, 0777) == -1)
    {
        perror("[server] mkdir() : err");
        return;
    }

    //fiecare user va avea propriul folder de fisiere ce vor contine numarul de mesaje primite
    bzero(cale, 256);
    strcpy(cale, "./numberOfMessages/");
    strcat(cale, username);

    if(mkdir(cale, 0777) == -1)
    {
        perror("[server] mkdir() : err");
        return;
    }

    //fiecare user va avea propriul folder de fisiere ce vor contine 1 daca poate comunica sau 0 daca nu (cu alti useri specifici)
    bzero(cale, 256);
    strcpy(cale, "./usersStates/");
    strcat(cale, username);

    if(mkdir(cale, 0777) == -1)
    {
        perror("[server] mkdir() : err");
        return;
    }
    
    fclose(file);
}

void recvMessagefromClient(int client)
{
    bzero(mesajC, 100);
    
    input_bytes = read(client, mesajC, 100);
    if(input_bytes <= 0)
    {
        perror("[server] read(): err\n");
        close(client);
        
        exit(0);
    }

    printf("[server] Mesaj primit de la client %d: %s\n", nrClienti, mesajC);
}

void sendMessageToClient(int client)
{       
    printf("[server] Raspuns catre client %d... %s\n",nrClienti, mesajS);

    output_bytes = write(client, mesajS, 100);
    if(output_bytes <= 0)
    {
        perror("[server] write(): err\n");
        close(client);

        exit(0);
    }
}

void sendConversationToClient(int client)
{       
    printf("[server] Raspuns catre client %d... %s\n",nrClienti, convS);

    output_bytes = write(client, convS, 1024);
    if(output_bytes <= 0)
    {
        perror("[server] write(): err\n");
    
        close(client);
        exit(0);
    }
}

void register_command(int client)
{
    printf("[server] Se verifica REGISTER...\n");
    fflush(stdout);

    //Serverul primeste pe rand de la client un username si o parola
    char username[50], parola[50];
    bzero(username, 50);
    bzero(parola, 50);

    //username-ul
    recvMessagefromClient(client);
    strcpy(username, mesajC);

    //parola
    recvMessagefromClient(client);
    strcpy(parola, mesajC);

    //se trimite raspuns catre client
    bzero(mesajS, 100);

    char* myUsersFile = "users_config";

    bool exista_user = verificare_username(myUsersFile, username);

    if(!exista_user)
    {
        strcpy(mesajS, "Cont creat cu succes!");

        //introducem noul user in fisierul users_config cu username si parola
        adauga_user(myUsersFile, username, parola);

        //introducem in fisierul users_stats starea Online si descriptorul userului
        adauga_stats(1, client);

        //introducem noul user in struct
        users[nrUseri].online = 1; 
        strcpy(users[nrUseri].username, username); 
        strcpy(users[nrUseri].parola, parola);
        nrUseri++; // creste numarul de useri

        //comanda executata cu succes si corect
        finished = true;  
    }
    else
    {
        //sprintf(mesajS, "Exista deja un utilizator cu username-ul %s.", username);
        strcpy(mesajS, "Exista deja un utilizator cu username-ul introdus");
        
        //comanda executata cu succes si corect
        finished = true; 
    }
    

    sendMessageToClient(client);
}

void login_command(int client)
{
    printf("[server] Se verifica LOGIN...\n");
    fflush(stdout);

    //Serverul primeste pe rand de la client un username si o parola
    char username[50], parola[50];
    bzero(username, 50);
    bzero(parola, 50);

    //username-ul
    recvMessagefromClient(client);
    strcpy(username, mesajC);

    //parola
    recvMessagefromClient(client);
    strcpy(parola, mesajC);

    //se trimite raspuns catre client
    bzero(mesajS, 100);

    char* myUsersFile = "users_config";

    bool exista_user_parola = verificare_username_parola(myUsersFile, username, parola);

    if(exista_user_parola)
    {
        strcpy(mesajS, "Ai fost logat cu succes!");

        //comanda executata cu succes si corect
        finished = true;  
    }
    else
    {
        strcpy(mesajS, "Username sau Parola gresita! Mai incearca.");
        //comanda executata cu succes si corect
        finished = true; 
    }

    sendMessageToClient(client);
}

void logout_command(int client)
{
    printf("[server] Userul s-a deloagat!\n");
    fflush(stdout);

    char username[50];
    bzero(username, 50);

    //server-ul primeste username-ul pt. care se face delogarea
    recvMessagefromClient(client);
    strcpy(username, mesajC);

    for(int i = 0; i < nrUseri; i++)
    {
        if(!strcmp(username, users[i].username))
        {
            users[i].online = 0; // user-ul va fi offline de acum
            
            break;
        }
    }

    bzero(mesajS, 100);
    strcpy(mesajS, "Ai fost delogat cu succes!");

    //comanda executata cu succes si corect
    finished = true;

    sendMessageToClient(client);
}

void searchMessageById(char expeditor[], char destinatar[], int mess_count, char mess_text[])
{
    char cale[256], line[256];
    char* p;
    bzero(cale, 256);
    bzero(line, 256);

    //int to string ca sa verific cu indexul (string) din fisier
    char str_count[256];
    bzero(str_count, 256);
    sprintf(str_count, "%d", mess_count);
    
    historyFilePath(cale, expeditor, destinatar);
    FILE* file; 
    file = fopen(cale, "r");

    if(file == NULL)
    {
        perror("[server] fopen() : err");
        return;
    }
    else
    {
        while(fgets(line, 256, file))
        {
            //line[strlen(line) - 1] = '\0';
            char cpy_line[256];
            bzero(cpy_line, 256);
            strcpy(cpy_line, line);

            if(strstr(cpy_line, "= ")) // daca e un mesaj primit
            {
                p = strtok(cpy_line, "="); //index-ul mesajului

                if(!strcmp(p, str_count))
                {
                    //se elimina indexul-ul din mesaj 
                    int len = -1;
                    for(int i = 0; i < strlen(line); i++)
                    {
                        len ++;
                        if(line[i] == ' ')
                        {
                            break;
                        }
                    }
                    strcpy(line, line + len + 1);

                    strcpy(mess_text, line);

                    fclose(file);
                    return;
                }
            }
        }
    }

    fclose(file);
}

void sendMessageToUser(char mesaj[], char expeditor[], char destinatar[], int id_mess)
{
    // mesajele sunt salvate in fisiere temporare pana ce vor fi citite de 'dest'

    char cale[256];
    bzero(cale, 256);

    createPath(cale, destinatar, expeditor);

    FILE* file;
    file = fopen(cale, "a+");
    if(file == NULL)
    {
        perror("[server] fopen() : err");
        return;
    }

    fprintf(file, "%d= ", id_mess);


    //tratam cazul in care mesajul primit e un reply pentru un anumit mesaj
    char* ret;
    ret = strstr(mesaj, "+reply"); 

    char mesajFinal[256];
    bzero(mesajFinal, 256);

    if(ret) //contine "+reply [message]"
    {
        //avem id-ul mesajului 
        int mess_count = atoi(mesaj);

        //cautam in functie de id mesajul pentru care se da reply
        char mess_text[100];
        bzero(mess_text, 100);
        searchMessageById(expeditor, destinatar, mess_count, mess_text); 
        strcpy(ret, ret + 6); //elimina +reply si ramane doar [message]
        
        printf("RET: %s\n", ret);
        fflush(stdout);

        mess_text[strlen(mess_text) - 1] = '\0';
        sprintf(mesajFinal, "Reply[%d->%s]%s", mess_count, mess_text, ret);
        
        //salveaza in fisierul expeditor_destinatar mesajul de tip reply
        fprintf(file, "%s@", mesajFinal);

        fclose(file);

        sleep(1);
        
        return;
    }

    //salveaza in fisierul expeditor_destinatar mesajul trimis
    fprintf(file, "%s@", mesaj);

    fclose(file);

    sleep(1);
}


void saveMessageTofile(char expeditor[], char destinatar[], char mess[])
{
    // Se salveaza atat in fisierul 'dest_exp', cat si in 'exp_dest' mesajul trimis de exp

    char path1[256], path2[256], line[256];
    bzero(path1, 256);
    bzero(path2, 256);
    bzero(line, 256);

    historyFilePath(path1, expeditor, destinatar); // retine si index mesaj
    historyFilePath(path2, destinatar, expeditor); 

    FILE* file;
    file = fopen(path1, "a");
    if(file == NULL)
    {
        perror("[server] fopen() : err");
        return;
    }

    //in fisierul destinatarului se salveaza mesajul cu index - necesar pentru 'reply'
    fprintf(file, "%s\n", mess);

    fclose(file);

    file = fopen(path2, "a");
    if(file == NULL)
    {
        perror("[server] fopen() : err");
        return;
    }

    //in fisierul expeditorului se salveaza mesajele trimise fara a retine indexul
    char copie[256];
    bzero(copie, 256);
    strcpy(copie, mess);

    int len = -1;
    for(int i = 0; i < strlen(copie); i++)
    {
        len ++;
        if(copie[i] == ' ')
        {
            break;
        }
    }

    // pentru a elimina 'id_mess=' pentru mesajele trimise
    strcpy(copie, copie + len + 1);

    fprintf(file, "%s\n", copie);

    fclose(file);
}

void recvMessageFromUser(char expeditor[], char destinatar[], char mess[])
{
    // 'dest' citeste din fisierul temporar mesajele necitite, primite de la 'exped'
    char cale[256];
    char line[512];
    bzero(cale, 256);
    bzero(line, 512);

    createPath(cale, expeditor, destinatar);

    FILE* file;
    file = fopen(cale, "r");
    if(file == NULL)
    {
        strcpy(mess, "");
        return;
    }

    sleep(1);

    fgets(line, 512, file);

    fclose(file);

    remove(cale);

    // retine in 'mess' mesajele citite
    strcpy(mess, line);
}

void getNumberOfMessages(char expeditor[], char destinatar[], int *id_mess)
{
    // determina numarul de mesaje primite de 'dest' de la 'exped'
    *id_mess = 0;

    char path[256];
    bzero(path, 256);
    strcpy(path, "./numberOfMessages/");
    strcat(path, destinatar);
    strcat(path, "/");
    strcat(path, destinatar);
    strcat(path, "_");
    strcat(path, expeditor);
    strcat(path, ".txt");

    FILE* file;

    if( access(path, F_OK ) == 0 ) 
    {
        // file exists
        file = fopen(path, "r");
        if(file == NULL)
        {
            perror("[server] fopen() : err");
            return;
        }

        int nr = getw(file);
        nr++;

        fclose(file);

        file = fopen(path, "w");
        if(file == NULL)
        {
            perror("[server] fopen() : err");
            return;
        }
        putw(nr, file);

        *id_mess = nr;

        fclose(file);
    } 
    else 
    {
        // file doesn't exist
        file = fopen(path, "w");
        if(file == NULL)
        {
            perror("[server] fopen() : err");
            return;
        }
        putw(1, file);
        
        *id_mess = 1; //suntem la primul mesaj introdus

        fclose(file);
    }
}

int checkOnline(char user1[], char user2[])
{
    char path[256];
    bzero(path, 256);
    strcpy(path, "./usersStates/");
    strcat(path, user1);
    strcat(path, "/");
    strcat(path, user1);
    strcat(path, "_");
    strcat(path, user2);
    strcat(path, ".txt");
    
    FILE* file;

    if( access(path, F_OK ) == 0 ) 
    {
        // file exists
        file = fopen(path, "r");
        if(file == NULL)
        {
            perror("[server] fopen() : err");
            return errno;
        }

        int stare = getw(file);

        fclose(file);

        return stare;
    }
    return -1;
}

void change_user_status(char user1[], char user2[], int stare)
{
    char path[256];
    bzero(path, 256);

    strcpy(path, "./usersStates/");
    strcat(path, user1);
    strcat(path, "/");
    strcat(path, user1);
    strcat(path, "_");
    strcat(path, user2);
    strcat(path, ".txt");
    
    FILE* file;

    file = fopen(path, "w");
    if(file == NULL)
    {
        perror("[server] fopen() : err");
        return;
    }

    putw(stare, file);

    fclose(file);

}

void start_chat_command(int client)
{
    printf("[server] Procesul de trimitere a mesajului in chat...\n");
    fflush(stdout);

    bool fileUsed = false;
    char expeditor[100], destinatar[100], mesaj[100];
    bzero(expeditor, 100);
    bzero(destinatar, 100);

    //primeste username-ul expeditorului
    recvMessagefromClient(client);
    strcpy(expeditor, mesajC);

    //primite username-ul destinatarului
    recvMessagefromClient(client);

    if(!strcmp(mesajC, "propriul username"))
    {
        // finished = true;
        return;
    }

    strcpy(destinatar, mesajC);

    char* myUsersFile = "users_config";

    // verifica daca username-ul destinatarului este valid
    bool exista_username = verificare_username(myUsersFile, destinatar);

    if(exista_username == false)
    {
        bzero(mesajS, 100);
        strcpy(mesajS, "destinatar invalid");

        sendMessageToClient(client);
        // finished = true; 
        return;
    }
    else 
    {
        bzero(mesajS, 100);
        strcpy(mesajS, "destinatar valid");

        sendMessageToClient(client);
    }

    //expeditor va putea primi mesaje de la destinatar -> se afla in chat-ul lor
    change_user_status(expeditor, destinatar, 1);

    int pid;
    pid = fork();
    if(pid == -1)
    {
        perror("fork() : err");
        return;
    }
    else if(pid == 0) // [copil] - 'dest' asteapta sa primeasca mesaje de la 'exped'
    {
        
        while(1)
        {
            char mess[512];
            bzero(mess, 512);

            // se verifica starea de online sau offline a 'destinatarului'
            int check_online = checkOnline(expeditor, destinatar);
            //int check_online = checkOnline(destinatar, expeditor);

            if(check_online == 0) // offline
            {
                continue;
            }
            else if(check_online == 1) // online
            {

                // 'destinatarul' citeste mesajele de la 'expeditor'
                recvMessageFromUser(expeditor, destinatar, mess);

                // mesajul '+stop' indica faptul ca 'exped' a iesit din conversatie

                if(!strcmp(mess, "0= +stop@"))
                {
                    change_user_status(destinatar, expeditor, 0);

                    continue;
                }

                // se verifica daca mesajul primit este valid
                if(strcmp("", mess))
                {
                    char* p;

                    p = strtok(mess, "@\n");
                    while(p)
                    {
                        if(strstr(p, "+stop"))
                        {
                            change_user_status(destinatar, expeditor, 0);
                            
                            continue;//break;?
                        }

                        //salveaza mesajul in istoricul conversatiilor celor 2 useri
                        saveMessageTofile(expeditor, destinatar, p);

                        bzero(mesajS, 100);
                        strcat(mesajS, destinatar);
                        strcat(mesajS, "> ");
                        strcat(mesajS, p);

                        sendMessageToClient(client);

                        p = strtok(NULL, "@\n");
                    }
                }
            }   
           
        }
    }
    else // [parinte] - 'exped' trimite mesaje catre 'dest'
    {
        while(1)
        {
            //primeste mesajul de la client, pentru a-l trimite catre 'dest'
            recvMessagefromClient(client);

            bzero(mesaj, 100);
            strcpy(mesaj, mesajC);

            // mesajul '+stop' indica faptul ca 'exped' vrea sa iasa din conversatie
            if(!strcmp("+stop", mesaj))
            {
                //trimite si la client +stop
                bzero(mesajS, 100);
                strcat(mesajS, "+stop");

                sendMessageToClient(client);

                sendMessageToUser("+stop", expeditor, destinatar, 0);

                // paraseste 'conversatia'
                kill(pid, SIGKILL);

                break;
            }

            // preia numarul de mesaje primite de 'destinatar' de la 'expeditor'
            int id_mess = 0;
            getNumberOfMessages(expeditor, destinatar, &id_mess);

            // 'expeditor' va trimite catre 'destinatar' mesajul cu indexul 'id_mess'
            sendMessageToUser(mesaj, expeditor, destinatar, id_mess);
        }
    }

    // comanda executata cu succes si corect
    finished = true; 
}

void check_conversations_for(char user1[])
{
    // identifica userii pentru care exista conversatii cu 'user1'
    char path[256];
    char aux[100];
    int conversatii_gasite = 1;

    bzero(mesajS, 100);

    FILE* file;
    file = fopen("users_config", "r");
    char line[256];
    bzero(line, 256);
    char* p;

    if(file == NULL)
    {
        perror("fopen() error: ");
        strcpy(mesajS, "Nu aveti conversatii valide!\n");

        return;
    }
    else
    {
        while(fgets(line, 256, file))
        {
            p = strtok(line, " "); // username-ul
            if(strcmp(user1, p)) // username-ul sa fie diferit de cel al lui 'user1'
            {
                bzero(path, 256);
                historyFilePath(path, user1, p);

                if( access(path, F_OK ) == 0 ) 
                {
                    // file exists -> avem conversatie intre cei doi useri
                    conversatii_gasite++;

                    bzero(aux, 100);
                    sprintf(aux, "%d. %s\n", conversatii_gasite, p);
                    strcat(mesajS, aux);
                }  

            }
        }
    }

    fclose(file);
}

void get_conversation_1(char path[], char username1[], char username2[])
{
    // afiseaza doar mesajele trimise de catre 'username1'
    FILE* file;
    file = fopen(path, "r");

    char line[256];
    bzero(line, 256);

    if(file == NULL)
    {
        perror("fopen() error: ");
        return;
    }
    else
    {
        char aux[512];
        bzero(aux, 512);
                    
        while(fgets(line, 256, file))
        {
            if(!strstr(line, "+stop"))
            {   
                if(!strstr(line, "= ")) // mesaj trimis, neindexat
                {
                    bzero(aux, 512);
                    
                    sprintf(aux, "%s to %s >> %s\n", username1, username2, line);
                    strcat(convS, aux);
                }    
            } 
        }
    }

    fclose(file);

}

void get_conversation_all(char path[], char username1[], char username2[])
{
    // afiseaza atat mesajele trimise, cat si cele primite din fisierul cu istoricul conversatiei lor
    
    FILE* file;
    file = fopen(path, "r");

    char line[256];
    bzero(line, 256);

    if(file == NULL)
    {
        perror("fopen() error: ");
        return;
    }
    else
    {
        char aux[512];
        bzero(aux, 512);
                    
        while(fgets(line, 256, file))
        {
            if(!strstr(line, "+stop"))
            {   
                if(!strstr(line, "= ")) // mesaj trimis, neindexat
                {
                    bzero(aux, 512);
                
                    sprintf(aux, "%s to %s >> %s\n", username1, username2, line);
                    strcat(convS, aux);
                } 
                else // mesaj primit
                {
                    bzero(aux, 512);
                
                    sprintf(aux, "%s to %s >> %s\n", username2, username1, line);
                    strcat(convS, aux);
                }   
            } 
        }
    }

    fclose(file);

}

void handle_conversations(char username1[])
{
    // se afiseaza mesajele trimise de 'username1' in toate conversatiile existente

    FILE* file;
    file = fopen("users_config", "r");
    char line[256];
    bzero(line, 256);
    char* p;
    char username2[100];
    bzero(username2, 100);

    if(file == NULL)
    {
        perror("fopen() error: ");

        strcpy(mesajS, "Nu aveti mesaje trimise valide!\n");

        return;
    }
    else
    {
        while(fgets(line, 256, file))
        {
            p = strtok(line, " "); // username-ul 'destinatarului'
            if(strcmp(p, username1))
            {
                char path[256];
                bzero(path, 256);

                historyFilePath(path, username1, p);

                if( access(path, F_OK ) == 0 ) 
                {   
                    // afiseaza doar mesajele trimise de catre 'username1'
                    get_conversation_1(path, username1, p);
                }
            }   
        }

        if(!strcmp(mesajS, ""))
        {
            strcpy(mesajS, "Nu aveti mesaje trimise!\n");
        }
    }

    fclose(file);
}

void history_command(int client)
{
    printf("[server] Se va afisa istoric conversatii....\n");
    fflush(stdout);

    char username1[100];
    bzero(username1, 100);

    //primeste username1
    recvMessagefromClient(client);
    strcpy(username1, mesajC);

    //pentru username1 - gaseste userii cu care exista conversatii
    check_conversations_for(username1);

    //trimite userii cu care exista conversatii catre client pentru a alege dintre ei
    sendMessageToClient(client);

    //primeste numarul optiune ales
    recvMessagefromClient(client);

    if(!strcmp(mesajC, "optiune invalida"))
    {
        finished = true;
        return;
    }

    int optiune_istoric = atoi(mesajC);
    
    char path[256];
   
    if(optiune_istoric == 1) // afiseaza doar mesajele trimise de solicitant
    {
        bzero(convS, 1024);

        // se afiseaza mesajele trimise de 'username1' in toate conversatiile existente
        handle_conversations(username1);

        sendConversationToClient(client);

        bzero(convS, 1024);
    }
    else // afiseaza toate mesajele din conversatia aleasa
    {
        bzero(convS, 1024);
        
        char* ret;
        ret = strstr(mesajS, mesajC); // primul va fi userul cu numarul din mesajC
        strcpy(ret, ret+3); // transforma "2. username\n'text'" -> "username\n'text'"

        char* p;
        p = strtok(ret, "\n");// user-ul ales de 'username1' pentru a i se afisa conversatia

        char path[256];
        bzero(path, 256);
        historyFilePath(path, username1, p);

        if( access(path, F_OK ) == 0 ) // exista istoric intre cei doi
        {   
            // afiseaza atat mesajele trimise, cat si cele primite
            get_conversation_all(path, username1, p);
        }

        sendConversationToClient(client);

        bzero(convS, 1024);
    }

    //comanda executata cu succes si corect
    finished = true;
}


void quit_command(int client)
{
    printf("[server] Clientul s-a deconectat!\n");
    fflush(stdout);

    char username[100];
    bzero(username, 100);

    //server-ul primeste username-ul pt. care se face Quit
    recvMessagefromClient(client);
    strcpy(username, mesajC);

    for(int i = 0; i < nrUseri; i++)
    {
        if(!strcmp(username, users[i].username))
        {
            users[i].online = 0; // user-ul va fi offline de acum
            
            break;
        }
    }

    // clientul se inchide
    quitCommand = 1;

    //comanda executata cu succes si corect
    finished = true;

    close(client);
}

void check_notifications_for(char user[])
{   
   // pentru 'user' se verifica daca exista notificari

    char path[256];
    char aux[100];
    int notificari_gasite = 0;

    bzero(mesajS, 100);
    strcpy(mesajS, "\n--- Notificari:\nAveti mesaje necitite de la:\n");

    FILE* file;
    file = fopen("users_config", "r");
    char line[256];
    bzero(line, 256);
    char* p;

    if(file == NULL)
    {
        perror("fopen() error: ");

        strcpy(mesajS, "Nu aveti notificari valide!\n");

        return;
    }
    else
    {
        while(fgets(line, 256, file))
        {
            p = strtok(line, " "); //username-ul 'expeditorului' de mesaje necitite
            if(strcmp(user, p))
            {
                bzero(path, 256);
                createPath(path, user, p);

                // file exists ->  are notificari - mesaje necitite de 'user'
                if( access(path, F_OK ) == 0 ) 
                {
                    //daca in fisierul temporar e doar mesajul "0= +stop@" - nu se pune ca notificare
                    bool mesaj_stop = verificare_username(path, "0=");
                    if(mesaj_stop)
                        continue;

                    notificari_gasite++;
                    bzero(aux, 100);

                    sprintf(aux, "%d. %s\n", notificari_gasite, p);
                    strcat(mesajS, aux);
                }  

            }
        }
    }

    fclose(file);
}

void notification(int client, char username[])
{
    //pentru 'username' - afiseaza userii care i-au trimis mesaje si inca sunt necitite
    check_notifications_for(username);

    if(!strcmp(mesajS, "\n--- Notificari:\nAveti mesaje necitite de la:\n"))
    {
        bzero(mesajS, 100);
        strcpy(mesajS, "--- Nu aveti notificari!");
    }
}

void command_handling(int client, int nr_comanda)
{
    if(nr_comanda == 1) // login
    {
        login_command(client);
    }
    else if(nr_comanda == 2) // register
    {
        register_command(client);
    }
    else if(nr_comanda == 3 || nr_comanda == 7) // quit 
    {
        quit_command(client);
    }
    else if(nr_comanda == 4) //start a chat
    {
        start_chat_command(client);
    }
    else if(nr_comanda == 5) //show message history
    {
        history_command(client);
    }
    else if(nr_comanda == 6) //logout
    {
        logout_command(client);
    }

}

int main()
{
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    int sd; // descriptor socket
    //creare socket
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd == -1)
    {
        perror("[server] socket() : err\n");
        return errno;
    }

    //familia de socket-uri
    server.sin_family = AF_INET;
    //accepta orice adresa
    server.sin_addr.s_addr = htonl (INADDR_ANY);
    //port utilizat
    server.sin_port = htons (PORT);

    //atasare socket
    if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
    	perror ("[server] bind() : err\n");
    	return errno;
    }
    
    //server-ul asculta daca vin clienti
    if (listen (sd, 5) == -1)
    {
    	perror ("[server] listen() : err\n");
    	return errno;
    }

    //functie care populeaza utilizatorii la pornirea serverului 
    char* myUsersFile = "users_config";
    initializareUseri(myUsersFile);

    printf("[server] Server pornit...\n");
    fflush(stdout);

    //servire in mod concurent a clientilor
    while(1)
    {
        int client;
        socklen_t length = sizeof(from);

        client = accept(sd, (struct sockaddr *) &from, &length);
        if(client < 0)
        {
            perror("[server] accept() : err\n");
            continue;
        }

        int pid;
        pid = fork();
        if(pid == -1)
        {
            close(client);
            continue;
        }
        else if(pid > 0) // [parinte] - se ocupa de acceptarea altor clienti
        {
            nrClienti++;
        }
        else if(pid == 0)// [copil] - se ocupa de clientul curent
        {
            close(sd); // inchide descriptor vechi, se foloseste cel nou returnat de accept
            
            while(!quitCommand) // comanda diferita de quit
            {
                finished = false;
                while(!finished) // se opreste cand datele vor fi valide
                {
                    recvMessagefromClient(client); // primeste comanda x in formatul "comanda:x"

                    int nr_comanda = mesajC[strlen(mesajC) - 1] - '0';

                    // daca e comanda pentru 'chat', mai intai se va verifica daca exista notificari pt. mesaje necitite
                    if(nr_comanda == 4) 
                    {
                        //primeste username
                        recvMessagefromClient(client);
                        char username[100];
                        bzero(username,100);
                        strcpy(username, mesajC);

                        notification(client, username);

                        //trimite catre client daca are sau nu notificari
                        sendMessageToClient(client);
                    }

                    command_handling(client, nr_comanda); // verifica si executa comanda
                }
            }
            
            close(client); // se inchide conexiunea cu acest client
            exit(0); // procesul copil este ucis
        }
    }
}