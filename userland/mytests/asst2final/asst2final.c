// Source: https://courses.cs.washington.edu/courses/cse451/14sp/a2.html

char  *filename = "/bin/cp";
char  *args[4];
pid_t  pid;

args[0] = "cp";
args[1] = "file1";
args[2] = "file2";
args[3] = NULL;

pid = fork();
if (pid == 0) execv(filename, argv);

