#include <unistd.h>
int
main(int argc, char ** argv)
{
    char *const args[] = {"http-root-dir/cgi-bin/test-cgi"};
    execvp(args[0], args);
}