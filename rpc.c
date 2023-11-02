#define _POSIX_C_SOURCE 200112L
#include "rpc.h"
#include "utils.h"
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define NONBLOCKING

void *handle_connection(void *arg);
int valid_check(rpc_data *payload);
typedef struct {
    int sockfd;
    rpc_server *srv;
} thread_data;

struct rpc_server {
    /* Add variable(s) for server state */
    int sockfd, handler_size;
    rpc_handle *handlers;
};

struct rpc_client {
    /* Add variable(s) for client state */
	int sockfd;
};

struct rpc_handle {
    /* Add variable(s) for handle */
    char name[1001]; // Names up to 1000 characters, plus room for '\0'
    rpc_handler handler;
    struct rpc_handle *next;
};


rpc_server *rpc_init_server(int port) {
    rpc_server *srv = malloc(sizeof(rpc_server));
    if (srv == NULL) {
        perror("malloc");
        return NULL;
    }

    int sockfd;

    char port_str[6];  // 5 digits for port number + 1 for '\0'
    sprintf(port_str, "%d", port);
	// Create the listening socket
	sockfd = create_listening_socket(port_str);
	srv->handler_size = 0;
	srv->sockfd = sockfd;
    return srv;
}

int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {
    if (srv == NULL || name == NULL || handler == NULL) {
        return -1;
    }

    int name_len = strlen(name);
    if (name_len == 0 || name_len > 1000) {
        perror("length");
        return -1;
    }

    // search for existing handler with the same name
    rpc_handle *current = srv->handlers;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            //replace it with new one
            current->handler = handler;
            return 0; 
        }
        current = current->next;
    }

    // no existing handler found, create new handler node
    rpc_handle *new_node = malloc(sizeof(rpc_handle));
    if (new_node == NULL) {
        perror("malloc");
        return -1;
    }

    // copy name and handler into new node
    strncpy(new_node->name, name, 1001);
    new_node->handler = handler;
    new_node->next = NULL;

    // if this is the first handler, add it directly to the server
    if (srv->handlers == NULL) {
        srv->handlers = new_node;
    } else {
        // otherwise, add the new node at the end of the list
        current = srv->handlers;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }

    return 0; // success
}

// this function will be executed in a separate thread
void *handle_connection(void *data) {
    thread_data *tdata = (thread_data *)data;
    int commute_sockfd = tdata->sockfd;
    rpc_server *srv = tdata->srv;
	char buffer[100000];
	int len; 
    while (1){

		if (read(commute_sockfd, &len, sizeof(len)) != sizeof(int)){
			break;
		} 
		// Receive a request
		if (read(commute_sockfd, buffer, len) != len){
			perror("read1");
			break;
		}

		if (len > 0) {
			buffer[len - 1] = '\0';  // Null-terminate the string
		} 

		// handle find request
		if (strncmp(buffer, "FIND ", 5) == 0) {
			char *name = buffer + 5;  // Skip past "FIND "

			// search for the handler
			rpc_handle *node = srv->handlers;
			while (node != NULL) {
				if (strcmp(node->name, name) == 0) {
					// found the handler, write a response
					len = snprintf(buffer, sizeof(buffer), "FOUND %s\n", name);
					if (write(commute_sockfd, buffer, len) != len) {
						perror("write");
					}
					break;
				}
				node = node->next;
			}

			if (node == NULL) {
				// handler not found
				len = snprintf(buffer, sizeof(buffer), "NOT FOUND %s\n", name);
				if (write(commute_sockfd, buffer, len) != len) {
					perror("write");
				}
			}
		} 
		// handle CALL request
        else if (strncmp(buffer, "CALL ", 5) == 0) {
            char *name = buffer + 5;  // skip past CALL

            // search handler
            rpc_handle *node = srv->handlers;
            while (node != NULL) {
                if (strncmp(node->name, name, strlen(node->name)) == 0) {
                    // found the handler, receive payload

                    rpc_data *payload = malloc(sizeof(rpc_data));
                    uint64_t read_data1;
                    len = read(commute_sockfd, &(read_data1), sizeof(uint64_t));
                    if (len != sizeof(uint64_t)) {
                        perror("read2.1");
                        break;
                    }
                    payload->data1 = (int)(ntohll(read_data1));


                    // Receive data2_len first
                    uint64_t read_data2_len;
					len = read(commute_sockfd, &(read_data2_len), sizeof(uint64_t));
					if (len != sizeof(uint64_t)) {
						perror("read2.2");
						break;
					}
                    payload->data2_len = (size_t)(ntohll(read_data2_len));
					if (payload->data2_len > 100000){
						fprintf(stderr, "Overlength error\n");
						exit(EXIT_FAILURE);
					}

                    
					if (payload->data2_len > 0){
						payload->data2 = malloc(payload->data2_len);
						len = read(commute_sockfd, payload->data2, payload->data2_len);
						if (len != payload->data2_len) {
							perror("read2.3");
							free(payload->data2);
							break;
						}
					}

                    // call the handler
                    rpc_data *result = node->handler(payload);
                    rpc_data_free(payload);
                        

                    // response writeer
                    int response = 0;
                    
                    if(valid_check(result) == 0){
                        len = snprintf(buffer, sizeof(buffer), "RESULT \n");

                        response = len;
                        write(commute_sockfd, &response, sizeof(int));
                    }
                    else{
                        perror("result-null");
                        write(commute_sockfd, &response, sizeof(int));
                        break;
                    }
                
                    // write back the result
					
					if (write(commute_sockfd, buffer, len) != len) {
						perror("write3.0");
					}
                    int64_t long_value = result->data1;
                    uint64_t write_data1 = htonll(long_value);
					if (write(commute_sockfd, &write_data1, sizeof(uint64_t)) != sizeof(uint64_t)) {
						perror("write3.1");
						break;
					}

					// write data 2 len
                    uint64_t long_size = result->data2_len;
                    uint64_t write_data2_len = htonll(long_size);
					if (write(commute_sockfd, &write_data2_len, sizeof(uint64_t)) != sizeof(uint64_t)) {
                        perror("write3.2");
                        return NULL;
                    }
					if (result->data2_len > 0 )
					{
							// write data 2
						
						if (write(commute_sockfd, result->data2, result->data2_len) != result->data2_len) {
							perror("write3.3");
							break;
						}
					}
                    rpc_data_free(result);

                    break;
                }

                node = node->next;
            }

            if (node == NULL) {
                // Handler not found
                len = snprintf(buffer, sizeof(buffer), "NOT FOUND %s\n", name);
                write(commute_sockfd, &len, sizeof(int));
                if (write(commute_sockfd, buffer, len) != len) {
                    perror("write");
                }
            }
		}
	}

    // close the socket when  done
    close(commute_sockfd);
    return NULL;
}

void rpc_serve_all(rpc_server *srv) {
	struct sockaddr_in6 client_addr;
	socklen_t client_addr_size;
    
	// Listen on socket - means we're ready to accept connections,
	// incoming connection requests will be queued, man 3 listen
	if (listen(srv->sockfd, 11) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	while (1) {
        client_addr_size = sizeof client_addr;
        int new_sockfd = accept(srv->sockfd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (new_sockfd < 0) {
            perror("accept");
            continue;
        }

		thread_data *tdata = malloc(sizeof(thread_data));
		tdata->sockfd = new_sockfd;
		tdata->srv = srv;
        // Create a separate thread for each connection
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_connection, tdata) != 0) {
            perror("pthread_create");
            continue;
        }

        // Detach the thread - this ensures that its resources are automatically
        // reclaimed when it finishes execution
        pthread_detach(thread_id);
    }
	
}



rpc_client *rpc_init_client(char *addr, int port) {
	rpc_client *cl = malloc(sizeof(rpc_client));
    if (cl == NULL) {
        perror("malloc");
        return NULL;
    }

    cl->sockfd = create_commute_socket(addr, port);
    return cl;
}

rpc_handle *rpc_find(rpc_client *cl, char *name) {
    if (cl == NULL || name == NULL) {
		perror("rpc_find");
        return NULL;
    }
    int name_len = strlen(name);
    if (name_len == 0 || name_len > 1000) {
        perror("length");
        return NULL;
    }

    // write the FIND request
    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer), "FIND %s\n", name);
	if (write(cl->sockfd, &len, sizeof(int)) != sizeof(int)) {
        perror("length");
        return NULL;
    }
    if (write(cl->sockfd, buffer, len) != len) {
        perror("write");
        return NULL;
    }

	// read buffer
    len = read(cl->sockfd, buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        perror("read");
        return NULL;
    }
    buffer[len] = '\0'; 
    
    if (strncmp(buffer, "FOUND ", 6) != 0) {
        return NULL;
    }

    // The handler was found, create and return a handle
    rpc_handle *h = malloc(sizeof(rpc_handle));
    if (h == NULL) {
        perror("malloc");
        return NULL;
    }
    strncpy(h->name, name, 1000);
    h->name[1000] = '\0';  // Ensure the string is null-terminated
    
    return h;
}


rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {
    if (cl == NULL || h == NULL || payload == NULL) {
        perror("rpc_call");
        return NULL;
    }
    if(valid_check(payload) < 0){
        perror("valid check");
        return NULL;
    }
    // format and write the "CALL" request
    char buffer[10000];
    int len = snprintf(buffer, sizeof(buffer), "CALL %s\n", h->name);
	// write length
	if (write(cl->sockfd, &len, sizeof(int)) != sizeof(int)) {
        perror("length");
        return NULL;
    }
	// write name

    if (write(cl->sockfd, buffer, len) != len) {
        perror("write2.0");
        return NULL;
    }
	// write data 1
    int64_t long_value = payload->data1;
    uint64_t write_data1 = htonll(long_value);

    if (write(cl->sockfd, &write_data1, sizeof(uint64_t)) != sizeof(uint64_t)) {
        perror("write2.1");
        return NULL;
    }
	// write data 2 len
    uint64_t long_size = payload->data2_len;
    uint64_t write_data2_len = htonll(long_size);
    if (write(cl->sockfd, &write_data2_len, sizeof(uint64_t)) != sizeof(uint64_t)) {
        perror("write2.2");
        return NULL;
    }


   // write data 2

   if (payload->data2_len > 0){
        if (write(cl->sockfd, payload->data2, payload->data2_len) != payload->data2_len) {
            
            perror("write2.3");
            return NULL;
        }
   }

    // wait for the server's response
    //response checker
    int response;
    len = read(cl->sockfd, &response, sizeof(int));

    if (len <= 0) {
        perror("response");
        return NULL;
    }
    if (!response)
    {
        perror("result-null");
        return NULL;
    }
    

	rpc_data *result = malloc(sizeof(rpc_data));
    
	len = read(cl->sockfd, buffer, response);
    if (len <= 0) {
        perror("read3.0");
        return NULL;
    }

    buffer[len] = '\0'; 

	if (strncmp(buffer, "RESULT ", 7) != 0) {
		perror("result");
        return NULL;
    }


    // read data1
    uint64_t read_data1;
    len = read(cl->sockfd, &(read_data1), sizeof(uint64_t));
	if (len != sizeof(uint64_t)) {
		perror("read3.1");
		return NULL;
	}
    result->data1 = (int)(ntohll(read_data1));


	// Receive data2_len first
    uint64_t read_data2_len;
    len = read(cl->sockfd, &(read_data2_len), sizeof(uint64_t));
	if (len != sizeof(uint64_t)) {
		perror("read3.2");
		return NULL;
	}
    result->data2_len = (size_t)(ntohll(read_data2_len));

	if (result->data2_len > 0)
	{
		result->data2 = malloc(result->data2_len);
		len = read(cl->sockfd, result->data2, result->data2_len);
		if (len != result->data2_len) {
			perror("read3.3");

			free(result->data2);
			return NULL;
		}
	}
	else{
		result->data2 = NULL;
	}

	return result;
} 


void rpc_close_client(rpc_client *cl) {
	free(cl);
}

void rpc_data_free(rpc_data *data) {
    if (data == NULL) {
        return;
    }
    if (data->data2 != NULL) {
        free(data->data2);
    }
    free(data);
}

int valid_check(rpc_data *payload){
     if (payload == NULL){
        perror("0");
        return -1;
    }
    if (payload->data2_len == 0 && payload->data2 != NULL)
    {
        perror("2");
        return -1;
    }
    if (payload->data2_len != 0 && payload->data2 == NULL)
    {
        perror("3");
        return -1;
    }
    return 0;
}
