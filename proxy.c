#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n\r\n";

/* Function Prototypes */
void *thread(void *vargp);
void proxy_sim(int fd);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg); 
void read_requestheaders(rio_t *rp);
int parse_uri(char *uri, int *web_port, char *web_host, char *web_path);

/*
 * SigPipe Handler - handles the sigpipe signal by doing nothing, 
                     so that proxy doesnt crash
 */
void sigpipe_hndlr(int signum)
{
	(void)signum;
	return;
}


/*
 * main - Web Proxy Main Routine 
 *      - Adapted from Tiny Web Server Main Routine Code &
 *        lecture notes on "Concurrent Programming"
 */
int main(int argc, char **argv)
{
    // Error checking on number of arguments, must be 2, else exit
    if (argc != 2)
    {
    	fprintf(stderr, "usage: %s <port>\n", argv[0]);
    	exit(1);
    }

    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    
    // Installing Signal Handler
    Signal(SIGPIPE, sigpipe_hndlr);

    // Opening listening socket so proxy can listen to requests from client
    listenfd = Open_listenfd(argv[1]);
    
    // Infinite while loop to keep accepting a connection request
    while(1)
    {
    	clientlen = sizeof(struct sockaddr_storage);
    	connfdp = Malloc(sizeof(int));
    	*connfdp = Accept(listenfd, (SA*)&clientaddr,&clientlen);

        /* Creating thread to handle requests and a thread function is given 
           that the thread will execute */
    	Pthread_create(&tid,NULL,thread,connfdp);
    }
    return 0;
}

/*
 * parse_uri - Parses uri into host, port and path
 *           - Adapted from tiny.c
 */
int parse_uri(char *uri, int *web_port, char *web_host, char *web_path)
{   
	// Uri starts from "http" as 'GET' part already parsed in proxy_sim

	/* Two temp character pointers start and end are used to save the 
	   the seperate values of the uri into local variables */
	char *start = uri;
	char *end = NULL;

	//Variables to save protocol and port values
	char protocol[20];
	char port_temp[20];

	*web_port = 80;
    
    //Checking for correct protocol
    if ((end = strchr(uri,':'))==NULL)
    	return 0;
    
    //Save the protocol and compare it with 'http'
    strncpy(protocol,start,end-start);
    protocol[end-start] = '\0';

    if (strcasecmp(protocol,"http") == 1)
    	return 0;
    
    end++;
    start = end;
    
    //Check if the next two characters are "/" 
    if (*start!='/')
    	return 0;
    start++;
     if (*start!='/')
    	return 0;
    start++;
    
    end = start; 
    
    // Retriving the host from the uri
    int flag = 1;
    while(*end != '\0' && flag)
    { 
    	// If ':' or "/" is seen then either port is specified or a path
    	if (*end == '/' || *end == ':')
    		flag = 0;
    	else
    		end++; 
    }

    // Saving the host 
    strncpy(web_host,start, end-start);
    web_host[end-start] = '\0';
    
    start = end;
    
    //If we reached the end add '/' to the path and return 1
    if (*start == '\0')
    	return 1;

    //Checking if a port was specified, if so save the port 
    if (*start == ':')
    {
    	start++;
    	end = start;
        //Continue until end of address or '/' is seen (that means a path is given)
    	while(*end!='\0' && *end!='/')
    		end++;

    	strncpy(port_temp,start,end-start);
    	port_temp[end-start] = '\0';

    	//Converting port string into port number
    	*web_port = atoi(port_temp);
    	start = end;
    }

    // The rest of the uri is the path, step through it and save in to path
    end = start;
    while(*end != '\0')
    	end++;

    strncpy(web_path, start, end-start);
    web_path[end-start] = '\0';
    return 1;

}


/*
 * Thread - function that the thread executes
 *        - main proxy function called from here 
 *        - closes connfd after request is complete
 *        - code adapted from "Concurrent Programming" lecture notes 
 */
void *thread(void *vargp)
{
	int connfd = *((int*)vargp);

	/* Detach the thread so that it runs independently 
       from other threads and repeaded by kernal when thread terminates
     */
	Pthread_detach(pthread_self());

	/* Free storage allocated to hold connfd */
	Free(vargp);
	Signal(SIGPIPE, sigpipe_hndlr); 

    /* Thread executes the proxy function */
	proxy_sim(connfd);

	Close(connfd);
	return NULL;
}


/*
 * proxy_sim - function handles one HTTP transaction
 *           - Receives the request from the client, passes the request
 *             to the web server. Then proxy recieves the required data back 
 *             from the web server and passes it back to the client
 *           - Code addapted from doit() function in tiny.c
 */
void proxy_sim(int fd)
{
	char buf[MAXLINE];
	char method[MAXLINE];
	char uri[MAXLINE];
	char version[MAXLINE];

	char address_path[MAXLINE];
	char host[MAXLINE];
	int webserver_port;
	char port_temp[MAXLINE];

	char webserver_buf[MAXLINE];
	
	rio_t rio;
	rio_t webserver_rio;
	
	int webserver_fd;

	size_t n;
    
    /* Read request line and headers */
	Rio_readinitb(&rio,fd);
	Rio_readlineb(&rio,buf,MAXLINE);

	printf("Request Headers:\n");
	printf("%s",buf);
    
	sscanf(buf, "%s %s %s",method, uri, version);

	//Checking if the request is a get request
	if(strcasecmp(method,"GET")) 
	{
		clienterror(fd,method,"501","Not Implemented",
			"Web proxy does not implement this method");
		return;
	}
    
	read_requestheaders(&rio);
    
    // Parsing uri into its different components, returns if unsucessful
	if(!parse_uri(uri, &webserver_port, host, address_path))
	{
		clienterror(fd,uri,"401","Not Found",
			"Proxy Web Sever could not handle the request");
		return;
	}
    
    // Converting the port string into port number 
    sprintf(port_temp,"%d", webserver_port);

    //Opening client fd from proxy side (proxy is now a client to the webserver)
	webserver_fd = Open_clientfd(host, port_temp);

    //Send the request that the client sent to proxy, to web server
	Rio_readinitb(&webserver_rio, webserver_fd);

	/* Save get request into buffer and write into webserver_buf, 
       do same for host info.
       - Using sprintf as a specific format is needed to be sent */
	sprintf(webserver_buf, "GET %s HTTP/1.0\r\n", address_path);
	Rio_writen(webserver_fd, (void*)webserver_buf,strlen(webserver_buf));
	sprintf(webserver_buf,"Host: %s\r\n", host);
	Rio_writen(webserver_fd, (void*)webserver_buf, strlen(webserver_buf));

	/* Send required headers to the webservers by writing to webserver_fd */
	Rio_writen(webserver_fd, (void*)user_agent_hdr, strlen(user_agent_hdr));
	Rio_writen(webserver_fd, (void*)connection_hdr, strlen(connection_hdr));
	Rio_writen(webserver_fd, (void*)proxy_connection_hdr, 
		strlen(proxy_connection_hdr));
 
    /* Reading what the server sends back and sending it back to client */
	while((n = Rio_readlineb(&webserver_rio,(void*)webserver_buf,MAXLINE))!=0)
	{
		Rio_writen(fd, (void*)webserver_buf,n);
	}

}

/*
 * read_requesthdrs - read HTTP request headers
 *                  - taken from tiny.c
 */
void read_requestheaders(rio_t *rp) 
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {         
	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
    }
    return;
}

/*
 * clienterror - returns an error message to the client
 *             - taken from tiny.c
 */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
