// gcc -g3 -Wall -o saturn saturn.c -lpthread

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <getopt.h>
#include <regex.h>

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int htl, threads, max_threads, collisions, max_collisions, skip, min_bytes;
regex_t *regex;
char *nntpserver, *group;
uint32_t article_count, *articles;
FILE *sock, *log;

typedef struct {
    char id[512], name[512];
    FILE *data;
} article;

void nntp_connect ();
void nntp_xover ();
void * fcp_put_thread (void *args);
article * get_article (uint32_t msg_num);
FILE * read_stduu (FILE *in);
FILE * read_base64 (FILE *in);

void
nntp_connect ()
{
    struct in_addr addr;
    struct sockaddr_in address;
    struct hostent *hent;
    int p = htons(119);
    int connected_socket, connected;

    addr.s_addr = inet_addr(nntpserver);
    if (addr.s_addr == -1) {
        hent = gethostbyname(nntpserver);
        if (hent == NULL) {
	    fprintf(stderr, "Could not resolve host %s.\n", nntpserver);
	    exit(1);
	}
        addr.s_addr = ((struct in_addr *) *hent->h_addr_list)->s_addr;
    }

    memset((char *) &address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = (p);
    address.sin_addr.s_addr = addr.s_addr;

    connected_socket = socket(AF_INET, SOCK_STREAM, 0);
    connected = connect(connected_socket, (struct sockaddr *) &address, sizeof(address));
    if (connected < 0) {
	fprintf(stderr, "Connection to %s:119 failed.\n", nntpserver);
	exit(1);
    }

    sock = fdopen(connected_socket, "w+");
}

void
nntp_xover ()
{
    char line[512];
    int status;
    
    fgets(line, 512, sock);
    if (strncmp(line, "200", 3) != 0) goto badreply;
    fflush(sock);
    
    fprintf(sock, "listgroup %s\r\n", group);
    fflush(sock);
    
    fgets(line, 512, sock); status = 0;
    sscanf(line, "%d", &status);
    if (status != 211) goto badreply;
    fflush(sock);

    printf("Downloading article list... ");
    fflush(NULL);
    article_count = 0;
    articles = calloc(128, sizeof(uint32_t));
    while (fgets(line, 512, sock)) {
	if (strcmp(line, ".\r\n") == 0) break;
	status = sscanf(line, "%d\r\n", &articles[article_count++]);
	if (status != 1) goto badreply;
	if (article_count % 128 == 0)
	    articles = realloc(articles, sizeof(uint32_t) * (article_count + 128));
    }

    if (strcmp(line, ".\r\n") != 0) {
	fprintf(stderr, "listgroup from server terminated unexpectedly!\n");
	exit(1);
    }

    printf("%d articles.\n", article_count);
    return;

badreply:
    fprintf(stderr, "Unexpected reply: %s", line);
    exit(1);
}

FILE *
fcp_connect ()
{
    struct in_addr addr;
    struct sockaddr_in address;
    struct servent *serv;
    int connected_socket, connected;
    serv = getservbyname("fcp", "tcp");
    if (!serv) return NULL;
    addr.s_addr = inet_addr("127.0.0.1");
    memset((char *) &address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = (serv->s_port);
    address.sin_addr.s_addr = addr.s_addr;
    connected_socket = socket(AF_INET, SOCK_STREAM, 0);
    connected = connect(connected_socket, (struct sockaddr *) &address, sizeof(address));
    if (connected < 0) return NULL;
    return fdopen(connected_socket, "w+");
}

void *
fcp_put_thread (void *args)
{
    article *a = (article *) args;
    FILE *fcp = fcp_connect();
    char reply[512];
    int c;
    if (!fcp) {
	fprintf(stderr, "Error connecting to node!\n");
	goto exit;
    }
    fprintf(fcp, "ClientPut\nHopsToLive=%x\nURI=freenet:CHK@\nDataLength=%lx\nData\n", htl, ftell(a->data));
    fflush(fcp);
    rewind(a->data);
    while ((c = fgetc(a->data)) != EOF) fputc(c, fcp);
    fflush(fcp);
    fgets(reply, 512, fcp);
    if (strncmp(reply, "Success", 7) == 0) {
	fgets(reply, 512, fcp);
	if (strncmp(reply, "URI=", 4) != 0) goto free;
	fprintf(log, "%s=%s", a->name, &reply[4]);
    } else { // FCPHandler is stupid, it makes KeyCollisions look like RouteNotFound!
	pthread_mutex_lock(&mutex);
	collisions++;
	pthread_mutex_unlock(&mutex);
    }

free:
    fclose(fcp);
    fclose(a->data);
    free(a);
exit:
    pthread_mutex_lock(&mutex);
    threads--;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

article *
get_article (uint32_t msg_num)
{
    int status = 0, base64 = 0;
    char line[512], msg_id[512], name[512];
    FILE *decoded, *data = tmpfile();
    article *a;

    if (!data) {
	fprintf(stderr, "Error creating tmpfile()!\n");
	return NULL;
    }
    
    if (regex) {
        fprintf(sock, "xhdr subject %d\r\n", msg_num);
        fflush(sock);
        fgets(line, 512, sock);
        sscanf(line, "%d", &status);
        if (status != 221) goto badreply;
        fgets(line, 512, sock);
        if (regexec(regex, line, 0, NULL, 0)) {
	    fprintf(stderr, "Skipping unmatched article %d.\n", msg_num);
	    fgets(line, 512, sock);
	    goto error;
	}
        fgets(line, 512, sock);
        if (strcmp(line, ".\r\n") != 0) goto badreply;
    }
    
    if (min_bytes) {
        fprintf(sock, "xhdr bytes %d\r\n", msg_num);
        fflush(sock);
        fgets(line, 512, sock);
        sscanf(line, "%d", &status);
        if (status != 221) goto badreply;
        fgets(line, 512, sock);
        sscanf(line, "%*d %d\r\n", &status);
        if (status < min_bytes) {
	    fprintf(stderr, "Skipping article %d: less than %d bytes.\n", msg_num, min_bytes);
	    fgets(line, 512, sock);
	    goto error;
        }
        fgets(line, 512, sock);
        if (strcmp(line, ".\r\n") != 0) goto badreply;
    }
    
    fprintf(sock, "body %d\r\n", msg_num);
    fflush(sock);
    fgets(line, 512, sock);
    sscanf(line, "%d %*d %s\r\n", &status, msg_id);
    if (status != 222) goto badreply;
    printf("Downloading article %d... ", msg_num);
    fflush(stdout);
    while (fgets(line, 512, sock)) {
	if (strcmp(line, ".\r\n") == 0) break;
	fputs(line, data);
    }
    if (strcmp(line, ".\r\n") != 0) {
	printf("terminated unexpectedly!\n");
	goto error;
    }
    printf("done.\n");
    
    rewind(data); name[0] = '\0';
    while (fgets(line, 512, data)) {
       if (strncmp(line, "begin ", 6) == 0) {
	   sscanf(line, "begin %*o %s", name);
	   break;
       } else if (strncmp(line, "begin-base64 ", 13) == 0) {
	   sscanf(line, "begin-base64 %*o %s", name);
	   base64 = 1;
	   break;
       }
    }
    
    if (!strlen(name)) {
	fprintf(stderr, "Decoding error on article %d: no data found!\n", msg_num);
	goto error;
    }
    
    if (base64) decoded = read_base64(data);
    else decoded = read_stduu(data);
    if (!decoded) {
	fprintf(stderr, "Decoding error on article %d: bad data!\n", msg_num);
	goto error;
    }
    fclose(data);
    
    a = malloc(sizeof(article));
    a->data = decoded;
    strcpy(a->id, msg_id);
    strcpy(a->name, name);
    return a;

badreply:
    fprintf(stderr, "Unexpected reply: %s", line);
error:
    fclose(data);
    return NULL;
}

void
usage (char *me)
{
    fprintf(stderr, "Usage %s [options] some.news.group\n\n"
                    "  -h --htl           The hops to live for inserts.\n"
		    "  -t --threads       The maximum number of concurrent inserts.\n"
		    "  -r --regex         Only insert articles whose names match regex.\n"
		    "  -c --collisions    Terminate after a number of collisions.\n"
		    "  -s --skip          Skip x newest articles.\n"
		    "  -m --min           Skip articles less than x bytes.\n\n",
		    me);
    exit(2);
}

int
main (int argc, char **argv)
{
    pthread_t thread;
    article *a;
    int c;
    char regex_string[256];
    extern int optind;
    extern char *optarg;

    static struct option long_options[] = {
	{"htl",        1, NULL, 'h'},
        {"threads",    1, NULL, 't'},
	{"regex",      1, NULL, 'r'},
	{"collisions", 1, NULL, 'c'},
	{"skip",       1, NULL, 's'},
	{"min",        1, NULL, 'm'},
	{0, 0, 0, 0}
    };
    
    nntpserver = getenv("NNTPSERVER");
    if (!nntpserver) {
	fprintf(stderr, "The NNTPSERVER environment variable must be set to the hostname of your NNTP server.\n"
		        "Example: export NNTPSERVER=nntp.myisp.com\n");
	exit(2);
    }

    htl = 15;
    max_threads = 1;
    regex_string[0] = '\0';
    max_collisions = 0;
    skip = 0;
    min_bytes = 0;
    
    while ((c = getopt_long(argc, argv, "h:t:r:c:s:m:", long_options, NULL)) != EOF) {
        switch (c) {
        case 'h':
            htl = atoi(optarg);
            break;
        case 't':
            max_threads = atoi(optarg);
            break;
	case 'r':
	    strncpy(regex_string, optarg, 256);
	    break;
        case 'c':
            max_collisions = atoi(optarg);
            break;
        case 's':
            skip = atoi(optarg);
            break;
	case 'm':
	    min_bytes = atoi(optarg);
	    break;
        case '?':
            usage(argv[0]);
            break;
        }
    }

    if (argc != optind+1) {
        usage(argv[0]);
        exit(2);
    }
    group = argv[optind];
   
    if (htl < 1) {
	fprintf(stderr, "Invalid HTL.\n");
	exit(2);
    }
    if (max_threads < 1) {
	fprintf(stderr, "Invalid max threads.\n");
	exit(2);
    }
    if (max_collisions < 0) {
	fprintf(stderr, "Invalid max collisions.\n");
	exit(2);
    }
    if (skip < 0) {
	fprintf(stderr, "Invalid number of articles to skip.\n");
	exit(2);
    }
    if (min_bytes < 0) {
	fprintf(stderr, "Invalid minimum bytes.\n");
	exit(2);
    }

    regex = NULL;
    if (strlen(regex_string)) {
	regex = malloc(sizeof(regex_t));
	c = regcomp(regex, regex_string, REG_ICASE | REG_EXTENDED | REG_NOSUB);
	if (c != 0) {
	    fprintf(stderr, "Invalid regular expression: %s\n", regex_string);
	    exit(2);
	}
    }
    
    nntp_connect();
    nntp_xover();
    
    if (!article_count) exit(0);
    
    log = fopen(group, "a");
    if (!log) {
	fprintf(stderr, "Can't open log file to append to!\n");
	exit(1);
    }
    
    do {
	if (skip-- > 0) {article_count--; continue;}
	if (max_collisions && collisions > max_collisions) {
	    fprintf(stderr, "Max collisions exceeded. Aborting.\n");
	    pthread_exit(NULL);
	}
	if ((a = get_article(articles[--article_count]))) {
	    pthread_create(&thread, NULL, fcp_put_thread, a);
	    if (max_threads && ++threads >= max_threads)
		while (threads >= max_threads) pthread_cond_wait(&cond, &mutex);
	}
    } while (article_count);
    
    printf("End of article list reached. Waiting for inserts to complete...\n");
    pthread_exit(NULL);
}

#define DEC(Char) (((Char) - ' ') & 077)

FILE *
read_stduu (FILE *in)
{
    FILE *out = tmpfile();
    char ch, *p, buf[2 * BUFSIZ];
    int n;

    while (1) {
        if (!fgets(buf, sizeof(buf), in)) return NULL;
        p = buf;
        n = DEC(*p);
        if (n <= 0) break;
        for (++p; n > 0; p += 4, n -= 3) {
            if (n >= 3) {
                ch = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
                fputc(ch, out);
                ch = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
                fputc(ch, out);
                ch = DEC(p[2]) << 6 | DEC(p[3]);
                fputc(ch, out);
            } else {
                if (n >= 1) {
                    ch = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
                    fputc(ch, out);
                }
                if (n >= 2) {
                    ch = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
                    fputc(ch, out);
                }
            }
        }
    }

    if (!fgets(buf, sizeof(buf), in)
	    || strcmp(buf, "end\n") != 0) return out; // be lenient
    return out;
}

FILE *
read_base64 (FILE *in)
{
    static const char b64_tab[256] = {
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\76',  '\177', '\177', '\177', '\77',
        '\64',  '\65',  '\66',  '\67',  '\70',  '\71',  '\72',  '\73',
        '\74',  '\75',  '\177', '\177', '\177', '\100', '\177', '\177',
        '\177', '\0',   '\1',   '\2',   '\3',   '\4',   '\5',   '\6',
        '\7',   '\10',  '\11',  '\12',  '\13',  '\14',  '\15',  '\16',
        '\17',  '\20',  '\21',  '\22',  '\23',  '\24',  '\25',  '\26',
        '\27',  '\30',  '\31',  '\177', '\177', '\177', '\177', '\177',
        '\177', '\32',  '\33',  '\34',  '\35',  '\36',  '\37',  '\40',
        '\41',  '\42',  '\43',  '\44',  '\45',  '\46',  '\47',  '\50',
        '\51',  '\52',  '\53',  '\54',  '\55',  '\56',  '\57',  '\60',
        '\61',  '\62',  '\63',  '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
        '\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',
    };
    
    unsigned char *p, buf[2 * BUFSIZ];
    char c1, c2, c3;
    int last_data = 0;
    FILE *out = tmpfile();
    
    while (1) {
      if (!fgets(buf, sizeof(buf), in)) return out; // be lenient
      p = buf;
      if (memcmp(buf, "====", 4) == 0) break;
      if (last_data != 0) return NULL;
      while (*p != '\n') {
          while ((b64_tab[*p] & '\100') != 0)
              if (*p == '\n' || *p++ == '=') break;
          if (*p == '\n') continue;
          c1 = b64_tab[*p++];
          while ((b64_tab[*p] & '\100') != 0)
              if (*p == '\n' || *p++ == '=') return NULL;
          c2 = b64_tab[*p++];
          while (b64_tab[*p] == '\177')
              if (*p++ == '\n') return NULL;
	  if (*p == '=') {
              fputc(c1 << 2 | c2 >> 4, out);
              last_data = 1;
              break;
          }
          c3 = b64_tab[*p++];
          while (b64_tab[*p] == '\177')
              if (*p++ == '\n') return NULL;
          fputc(c1 << 2 | c2 >> 4, out);
          fputc(c2 << 4 | c3 >> 2, out);
          if (*p == '=') {
              last_data = 1;
              break;
          } else
              fputc(c3 << 6 | b64_tab[*p++], out);
        }
    }

    return out;
}
