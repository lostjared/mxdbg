#include <stdlib.h>
#include <string.h>

__attribute__((noinline)) int step_out_target(int should_crash) {
    if (should_crash) {
        volatile int *invalid = NULL;
        *invalid = 1;
    }
    return 42;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "exit7") == 0) {
        return 7;
    }
    return step_out_target(argc > 1) == 42 ? EXIT_SUCCESS : EXIT_FAILURE;
}
