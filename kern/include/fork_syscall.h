

/* Helper for fork(). You write this. */
void enter_forked_process(struct trapframe* tf);

int sys_fork(const_userptr_t tf, pid_t*);
