extern ttyrec tr;

extern int speed;
extern tty term;

extern int waiting, loaded;
extern mutex_t waitm;
extern cond_t waitc;

void replay(void);
