int codec;
char *command;
int lport,rport;
char *record_name;

void get_parms(int argc, char **argv, int prog);
FILE *fopen_out(char **file_name, int nodetach);
