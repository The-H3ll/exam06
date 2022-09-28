
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>


int fd_socket, PORT;

fd_set s_read, s_write, s_master;

char msg[42];

char string[42 * 4096], buffer[42 * 4096], sent[42 * 4096];

int     max_id = 0;


typedef struct client
{
    int id;
    int fd;

    struct client *next;
}               t_client;


t_client *client = NULL;



void    error(char *str)
{
    write(2, str, strlen(str));
    exit(1);
}

void    sendMsgAll(char *msg, int fd)
{
    t_client    *temp = client;

    while (temp != NULL)
    {
        if (temp->fd != fd && FD_ISSET(temp->fd, &s_write))
        {
            if (send(temp->fd, msg, strlen(msg), 0) < 0)
                error("Fatal error\n");
        }
        temp = temp->next;
   }



}


// FILL CLIENT AND MASTER
void   joinClient()
{

    struct sockaddr_in ad;
    int len;
    t_client    *cli, *tmp = client;
    
//   if ((fd_client = accept(fd_socket, (struct sockaddr*)&client_addr, (socklen_t*)&addr_len)) < 0)

    int newSocket = accept(fd_socket,(struct sockaddr *)&ad,  (socklen_t *)&len);

    if (newSocket < 0)
     error ("Fatal error\n");
    if (client == NULL)
    {
        if ((client = malloc(sizeof(t_client))) == NULL)
            error("Fatal error\n");
        client->id = max_id++;
        client->fd = newSocket;
        FD_SET(newSocket, &s_master);
        client->next = NULL;
        return ;
    }
    while (tmp && tmp->next)
        tmp = tmp->next;  
       cli = malloc(sizeof(t_client));
        cli->id = max_id++;
        cli->fd = newSocket;
        cli->next = NULL;
    tmp->next = cli;
    FD_SET(newSocket, &s_master);
    bzero(&msg, sizeof(msg));
    sprintf(msg, "server: client %d just arrived\n", cli->id);
    sendMsgAll(msg, cli->fd);
    bzero(&msg, strlen(msg));
}




void    clear_client(int fd)
{
    t_client    *temp = client;
    t_client *tmp;

    if (temp->fd == fd)
    {
        tmp = temp;
        client = temp->next;
        FD_CLR(temp->fd, &s_master);
        free(tmp);
        return ;
    }
    while (temp && temp->next)
    {
        if (temp->next->fd == fd)
        {
            tmp = temp->next;
            temp->next = temp->next->next;
            FD_CLR(fd, &s_master);
            free(tmp);
            return ;
        }
        temp = temp->next;
    }
}


int get_id(int fd)
{
    t_client* temp = client;
    while (temp != NULL)
    {
        if (temp->fd == fd)
            return temp->id;
        temp = temp->next;
    }
    return (-1);
}

int max_fd()
{
    t_client* temp = client;
    int  i = fd_socket;

    while (temp != NULL)
    {
        if (temp->fd > i)
            i = temp->fd;
        temp = temp->next;
    }
    return i;
}


void    write_msg(int fd)
{
    int i = 0;
    int j = 0;

            bzero(sent, strlen(sent));
            bzero(buffer, strlen(buffer));
    while (string[i])
    {
        buffer[j] = string[i];
        j++;
        if (string[i] == '\n')
        {
            sprintf(sent, "client %d: %s", get_id(fd), buffer);
            sendMsgAll(sent, fd);
            j = 0;
            bzero(sent, strlen(sent));
            bzero(buffer, strlen(buffer));
        }
        i++;
    }
    bzero(string, strlen(string));

}


int main(int ac, char **av)
{
    if (ac != 2)
        error("Wrong number of arguments\n");
    PORT = atoi(av[1]);
    struct sockaddr_in ad;
    // socket Creation;
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket < 0)
        error("Fatal error\n");

    bzero(&ad, sizeof(ad));
    //FIILINF ad
    ad.sin_family = AF_INET; 
	ad.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	ad.sin_port = htons(PORT);  

    
    // BINDING
    if (bind(fd_socket, ( struct sockaddr *)&ad, sizeof(ad)) < 0)
        error("Fatal error\n");

    if (listen(fd_socket, 10) < 0)
        error("Fatal error\n");

    // all set zero
    // FD_ZERO(&s_write);
    // FD_ZERO(&s_read);
    FD_ZERO(&s_master);
    bzero(&msg, sizeof(msg));
    bzero(&string, sizeof(string));
    FD_SET(fd_socket, &s_master);

    while (1)
    {
        s_read = s_master;
        s_write = s_master;
        if (select(max_fd() + 1, &s_read, &s_write, NULL, NULL) < 0)
            continue;
        for (int i =0 ; i <= max_fd(); i++)
        {
            // Check Read is ready
            if (FD_ISSET(i, &s_read))
            {
                if (i == fd_socket)
                {
                    
                    joinClient();
                    break;
                }
                else {
                    int res ;
                    // printf("res ==> %d\n" , res);
                    while ((res = recv(i, string + strlen(string), 1000, 0)))
                    {
                        // printf("res IN===>  %d\n" , res); 
                        if (string[strlen(string) - 1] == '\n' && res != 1000)
                            break;   
                    }
                    if (res <= 0)
                    {
                        bzero(&msg, strlen(msg));
                        sprintf(msg,"server: client %d just left\n", get_id(i));
                        sendMsgAll(msg, i);
                        close(i);
                        clear_client(i);
                    }
                    else{
                        write_msg(i);
                    }
                }
            }
        }
    }   
    return (0);
}