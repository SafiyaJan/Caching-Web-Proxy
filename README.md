# Caching-Web-Proxy

Hello!

- This respository contains a caching web proxy that I have written in C
- The proxy is set up to accept incoming accept incoming connections, read and parse requests, forward requests to web servers, read the servers’ responses, and forward those responses to the corresponding clients using HTTP requests and socket programming
- The proxy can deal with multiple concurrent connections by using multithreading
- Caching is added to proxy by using a simple main memory cache of recently accessed web content.

## Files included in repository 

### port-for-user.pl
Generates a random port for a particular user
    ```usage: ./port-for-user.pl <AndrewID>```

### free-port.sh 
Handy script that identifies an unused TCP port that you can use for your proxy or tiny. 
    ```usage: ./free-port.sh```
    
### proxy.c
Main file that houses the caching web proxy
    
