#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

volatile int signal_caught = 0;

void signal_handler(int signum) {
	printf("Signal %d caught!\n", signum);
	signal_caught = 1;
	// Note: We're not returning properly with sigreturn here
	// This test just verifies the handler gets called
	exit(42);  // Exit with a known value to indicate success
}

int main() {
	printf("Setting up signal handler %p...\n", signal_handler);

	// Install signal handler for SIGUSR1
	if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
		printf("Failed to install signal handler\n");
		return 1;
	}
	// Retrieve and print the installed handler address
	struct sigaction sa;
	if (sigaction(SIGUSR1, NULL, &sa) == -1) {
		printf("Failed to retrieve signal action\n");
		return 1;
	}
	printf("Installed handler address: %p\n", sa.sa_handler);
	fflush(stdout);

	printf("Raising SIGUSR1...\n");

	// Raise the signal
	raise(SIGUSR1);

	// If we get here, signal handling didn't work properly
	printf("Signal was not caught!\n");
	return 1;
}
