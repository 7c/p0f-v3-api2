#ifndef MUTEX_TCP_H
#define MUTEX_TCP_H

#define MUTEX_TCP_IP "127.0.0.1"

// Starts the TCP mutex on the given port
// Returns 0 if successfully started (no other instance is running)
// Returns -1 if the port is already in use (indicating another instance is running)
int mutex_start(int port);

// Releases the mutex (stops holding the port)
void mutex_stop(void);

#endif // MUTEX_TCP_H