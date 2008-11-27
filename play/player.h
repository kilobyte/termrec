ttyrec tr;

int speed;
vt100 term;

int waiting, loaded;
mutex_t waitm;
cond_t waitc;

void replay();
