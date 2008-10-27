char *command, *format, *format_ext;
int lport,rport;
char *record_name;
int raw;

void get_parms(int argc, char **argv, int prog);
int fopen_out(char **file_name, int nodetach);
