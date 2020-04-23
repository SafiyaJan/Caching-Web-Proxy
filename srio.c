

#include "srio.h"

/****************************************
 * The SafiyaRio package - Robust I/O functions
 ****************************************/

/*
 * srio_readn - Robustly read n bytes (unbuffered)
 */
/* $begin srio_readn */
ssize_t srio_readn(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nread = read(fd, bufp, nleft)) < 0) {
	    if (errno == EINTR) /* Interrupted by sig handler return */
		nread = 0;      /* and call read() again */
	    else if(errno == ECONNRESET)
            return 0;
        else
		return -1;      /* errno set by read() */ 
	} 
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readn */

/*
 * rio_writen - Robustly write n bytes (unbuffered)
 */
/* $begin rio_writen */
ssize_t srio_writen(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nwritten = write(fd, bufp, nleft)) <= 0) {
	    if (errno == EINTR)  /* Interrupted by sig handler return */
		nwritten = 0;    /* and call write() again */
	    else if (errno == EPIPE)
            return n;
        else
		return -1;       /* errno set by write() */
	}
	nleft -= nwritten;
	bufp += nwritten;
    }
    return n;
}
/* $end rio_writen */


/* 
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t srio_read(srio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->srio_cnt <= 0) {  /* Refill if buf is empty */
	rp->srio_cnt = read(rp->srio_fd, rp->srio_buf, 
			   sizeof(rp->srio_buf));
	if (rp->srio_cnt < 0) {
        if (errno == ECONNRESET)
            return 0;
	    if (errno != EINTR) /* Interrupted by sig handler return */
		return -1;
	}
	else if (rp->srio_cnt == 0)  /* EOF */
	    return 0;
	else 
	    rp->srio_bufptr = rp->srio_buf; /* Reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;          
    if (rp->srio_cnt < n)   
	cnt = rp->srio_cnt;
    memcpy(usrbuf, rp->srio_bufptr, cnt);
    rp->srio_bufptr += cnt;
    rp->srio_cnt -= cnt;
    return cnt;
}
/* $end rio_read */

/*
 * rio_readinitb - Associate a descriptor with a read buffer and reset buffer
 */
/* $begin rio_readinitb */
void srio_readinitb(srio_t *rp, int fd) 
{
    rp->srio_fd = fd;  
    rp->srio_cnt = 0;  
    rp->srio_bufptr = rp->srio_buf;
}
/* $end rio_readinitb */

/*
 * rio_readnb - Robustly read n bytes (buffered)
 */
/* $begin rio_readnb */
ssize_t srio_readnb(srio_t *rp, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;
    
    while (nleft > 0) {
	if ((nread = srio_read(rp, bufp, nleft)) < 0) 
    {
        if (errno == EINTR)
            nread = 0;
        else
            return -1;          /* errno set by read() */ 
    }
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readnb */

/* 
 * rio_readlineb - Robustly read a text line (buffered)
 */
/* $begin rio_readlineb */
ssize_t srio_readlineb(srio_t *rp, void *usrbuf, size_t maxlen) 
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) { 
        if ((rc = srio_read(rp, &c, 1)) == 1) {
	    *bufp++ = c;
	    if (c == '\n') {
                n++;
     		break;
            }
	} else if (rc == 0) {
	    if (n == 1)
		return 0; /* EOF, no data read */
	    else
		break;    /* EOF, some data was read */
	} else
	    return -1;	  /* Error */
    }
    *bufp = 0;
    return n-1;
}
/* $end rio_readlineb */

/**********************************
 * Wrappers for robust I/O routines
 **********************************/
ssize_t SRio_readn(int fd, void *ptr, size_t nbytes) 
{
    ssize_t n;
  
    if ((n = srio_readn(fd, ptr, nbytes)) < 0)
	unix_error("Rio_readn error");
    return n;
}

void SRio_writen(int fd, void *usrbuf, size_t n) 
{
    if (srio_writen(fd, usrbuf, n) != n)
	unix_error("Rio_writen error");
}

void SRio_readinitb(srio_t *rp, int fd)
{
    srio_readinitb(rp, fd);
} 

ssize_t SRio_readnb(srio_t *rp, void *usrbuf, size_t n) 
{
    ssize_t rc;

    if ((rc = srio_readnb(rp, usrbuf, n)) < 0)
	unix_error("Rio_readnb error");
    return rc;
}

ssize_t SRio_readlineb(srio_t *rp, void *usrbuf, size_t maxlen) 
{
    ssize_t rc;

    if ((rc = srio_readlineb(rp, usrbuf, maxlen)) < 0)
	unix_error("Rio_readlineb error");
    return rc;
} 