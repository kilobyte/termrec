ttyrec tr;

int speed;
tty term;

int waiting, loaded;
mutex_t waitm;
cond_t waitc;

void replay();
