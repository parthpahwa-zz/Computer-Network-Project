/*
===========================================================================================================
                                        Web Browser Engine
                                    Computer Networks Mini Project
-----------------------------------------------------------------------------------------------------------
Author(s): Parth Pahwa, Rohan Gulati
Dependencies: GTK 2.0, Standard Linux C Libraries
-----------------------------------------------------------------------------------------------------------
Usage:
> gcc CN.cpp -w $(pkg-config --cflags --libs gtk+-2.0)
> ./a.out > logs.txt
===========================================================================================================
*/

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <gtk/gtk.h>
#include <stdlib.h>  
char hostname[1024];

int windowCount = 0;

/*================================| PROTOTYPES | ==============================*/

void getHostInfo();
void createConnection();
void sendMessage(char *req, char *hst);
void recieveMessage();
int globalRequestCount = 0;
int globalResponseCount = 0;

/*-----------------------------------------------------------------------------------------------------

Function name: enter_callback
Parameters : GtkWidget *widget
Return type : static void

This function processes the input taken via the GUI. It calls all other major functions in the program, 
and operates on the gtk_main() thread.

------------------------------------------------------------------------------------------------------*/
static void enter_callback( GtkWidget *widget, GtkWidget *entry)
{
  const gchar *entry_text;
  entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
  strcpy(hostname, entry_text);
    getHostInfo();
    createConnection();
    
    char *msg = hostname;
    sendMessage("",msg);
    recieveMessage();

}

void displayWindow(){

}
/*================================| GLOBALS | ==============================*/
int i;
char ip[100];


struct hostent *he;
struct in_addr **addr_list;
int noBreaks = 0;
int socket_desc;
struct sockaddr_in server;
char server_reply[20000];
char header[21024];

/*-----------------------------------------------------------------------------------------------------

Function name: getHostInfo
Parameters : none
Return type : void

getHostInfo resoleves the IP of the enterned web page and copies it into a string

------------------------------------------------------------------------------------------------------*/

void getHostInfo(){
    printf("Fetching %s\n", hostname);
    if( (he = gethostbyname( hostname )) == NULL){
        herror("gethostbyname");
        return;
    }

    addr_list = (struct in_addr **) he->h_addr_list;

    for(i = 0; addr_list[i] != NULL; i++) 
    {
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        //printf("%s resolved to : %s\n" , hostname , ip);
    }



    strcpy(ip , inet_ntoa(*addr_list[0]) );
}



/*-----------------------------------------------------------------------------------------------------

Function name: createConnection
Parameters : none
Return type : void

1)createConnection creates a TCP connection on port 80 of the entered webpage
2)Displays error if socket is not successfully created
3)Displays error if connection is not successfully

------------------------------------------------------------------------------------------------------*/

void createConnection(){

    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){
        perror("Could not create socket");
        exit(0);
    }
    
    
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons( 80 );
    
    //Connect to remote server
    if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect error");
        exit(0);
    }
    // puts("Connected\n"); 
}

/*-----------------------------------------------------------------------------------------------------

Function name: sendMessage
Parameters : 
    1) req : char*
            req contains the requested URI
    2) hst : char*
            hst contains the host name

Return type : void

1)Generates the GET request using the parameters provide
2)Displays error if GET request is not successfully sent

------------------------------------------------------------------------------------------------------*/

void sendMessage(char *req , char *hst){
    char message[1000] = "\0";
    char string1[1024] = "GET /";
    char string2[1024] = "HTTP/1.1\r\nHost:";
    char string3[1024] = "\r\nUser-agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:47.0) Gecko/20100101 Firefox/47.0\r\n\r\n";
    strcat(req, "\0");
    strcat(hst, "\0");

    char req2[100];
    if(strlen(req)!= 1){
        strcpy(req2,req);
        req2[strlen(req)-1]='\0';
        req2[strlen(req)]='\0';
        req2[strlen(req)+1]='\0';
    }

    strcat(message, string1);
    strcat(message, req);
    strcat(message, " ");
    strcat(message, string2);
    strcat(message, " ");
    strcat(message, hst);
    strcat(message, string3);
    
    if( send(socket_desc , message , strlen(message) , 0) < 0)
    {
        perror("Send failed");
        exit(0);
    }
    printf("-------------------Request Sent(#%d)---------------------\n", ++globalRequestCount); 
    printf("%s\n", message);
    printf("---------------------------------------------------------\n");
}



// Prototype for redirection function. Implemented later on.
int redirect(char *header);



/*-----------------------------------------------------------------------------------------------------

Function name: recieveMessage
Parameters : none
Return type : void

1)Recieves the Response message sent by website
3)Writes the Respose message in a .html file for parsing
2)Calls redirect() function

------------------------------------------------------------------------------------------------------*/

void recieveMessage(){
    int flag1 = 0;
    //Receive a reply from the server
    int iter1,iter2 = 0;
    FILE* fp = fopen("index.html", "w+");


    int ctr = 0;
    while( recv(socket_desc, server_reply , 2000 , 0) > 0)    
    {

        if(flag1 < 5){
            strcpy(header, server_reply);
            flag1++;
        }
        //puts(server_reply);
        fputs(server_reply ,fp);
        char signal[5];
        
        for(iter1 = strlen(server_reply)-1; iter1>= strlen(server_reply) -7; iter1--)
            signal[iter2++] = server_reply[iter1];
        if(strcmp(signal, ">lmth") && noBreaks < 2){
            noBreaks++;
            break;
        }
        else{
            ctr++;
        }

    }
    printf("\n***********************Response Received(#%d)*********************\n", ++globalResponseCount);
    printf("%s\n", header);
    printf("\n*****************************************************************\n\n");


    if(ctr > 0 && fork() == 0){
            system("firefox index.html &");
        exit(0);
    }
    else
    {
        wait(0);
    }

    fclose(fp);
    int redir = redirect(header);
    printf("%s successfully fetched from remote server. Launching default web browser.\n", hostname);
    exit(0);
}

/*-----------------------------------------------------------------------------------------------------

Function name: redirect
Parameters : 
    1) header: char*
            header is the Response header recieved from the website for parsing
Return type : int
    returns the POST request code

1)Parses through header
2)Redirects to the new location in case of 301 & 302

------------------------------------------------------------------------------------------------------*/

int redirect(char *header){
    char *headerVals[1024];
    int iter2 = 0;
    char *token = strtok(header, " ");
    while(token){
        headerVals[iter2++] = token;
        token = strtok(NULL, " ");
    }
    int k=0;

    while(k<iter2 && strstr(headerVals[k], "Location") == NULL){

        k++;
    }
    if(k==iter2-1)
        return 0;
    char *newLoc = headerVals[++k];

    newLoc = strtok(newLoc, "\n");

    if(strcmp(headerVals[1], "301")== 0 || strcmp(headerVals[1], "302")== 0){
        //printf("301: Site has permanently moved to %s\n", newLoc);
        token = strtok(newLoc, "/");
        iter2 = 0;
        char *temp[20];
        while(token){
            temp[iter2++] = token;
            token = strtok(NULL, "/");
        }

        if(strlen(temp[2]) == 1){
            // printf("temp2 sklafjklsf: %d\n", int(temp[2][0]));
            sendMessage("",temp[1]);
            recieveMessage();
        }
        else{
            char dst[100] ;
            int j = 0;
            for(int i = 0 ; i < strlen(temp[2]) ; i++){
                if(int(temp[2][i]) != 13){
                    dst[j++] = temp[2][i];

                }
            }
            dst[j] ='\0';
            printf("\n");


            sendMessage(dst,temp[1]);
            recieveMessage();
        }
        return 301;
    }
    else if((strcmp(headerVals[1], "404") == 0)){
        return 404;
    }
    else{
        return 0;
    }
}


int main(int argc , char *argv[])
{
    //Clear stdout
    system("clear");

    /*Variables for GTK window*/
    GtkWidget *window, *win2;
    GtkWidget *vbox, *hbox;
    GtkWidget *entry;
    GtkWidget *text;
    GtkWidget *button;
    gint tmp_pos;

    gtk_init (&argc, &argv);

    /* create a new window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (GTK_WIDGET (window), 400, 350);
    gtk_window_set_title (GTK_WINDOW (window), "Web Browser");
    g_signal_connect (window, "destroy",
      G_CALLBACK (gtk_main_quit), NULL);
    g_signal_connect_swapped (window, "delete-event",
      G_CALLBACK (gtk_widget_destroy), 
      window);

    /* create a new container */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show (vbox);

    /* create a new entry box */
    entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
    g_signal_connect (entry, "activate",
      G_CALLBACK (enter_callback),
      entry);
    gtk_entry_set_text (GTK_ENTRY (entry), "www.google");
    tmp_pos = GTK_ENTRY (entry)->text_length;
    gtk_editable_insert_text (GTK_EDITABLE (entry), ".com", -1, &tmp_pos);
    gtk_editable_select_region (GTK_EDITABLE (entry),
        0, GTK_ENTRY (entry)->text_length);
    gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (entry);

    /* create welcome label */
    gchar *introText = "\t\t\tWelcome!\n Enter the webpage you want to visit.\n\n\n\t\t    Created by:\nRohan Gulati and Parth Pahwa\n"; 
    text = gtk_label_new(introText);
    gtk_box_pack_start (GTK_BOX (vbox), text, TRUE, TRUE, 0);
    gtk_widget_show(text);

    const gchar* buttonLabel  = "Connect";

    button = gtk_button_new_with_label (buttonLabel);
    g_signal_connect_swapped (button, "clicked",
      G_CALLBACK (gtk_widget_destroy),
      window);
    gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
    gtk_widget_set_can_default (button, TRUE);
    gtk_widget_grab_default (button);
    gtk_widget_show (button);
    gtk_widget_show (window);


    /* Run main GTK thread for GUI*/
    gtk_main();



    return 0;
}
