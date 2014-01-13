/*
Program Space Control (psc)

Copyright (C) 2013 Rob van der Hoeven

Version 0.7

Support at: http://freedomboxblog.nl

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//#define _XOPEN_SOURCE 600
#define _GNU_SOURCE

#include <ctype.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include <sys/un.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <termios.h>
#include <pwd.h>

#define pivot_root(new_root,put_old) syscall(SYS_pivot_root,new_root,put_old)
#define LOG_INFO(info_format)      "Info  ps [%s] fn [%s] " info_format "\n", ps_name, __FUNCTION__
#define LOG_ERROR(error_format)    "Error ps [%s] fn [%s] " error_format "\n", ps_name, __FUNCTION__
#define LOG_BUFFER_HEX(description, buffer, count)\
ps_log_hex_dump(__FUNCTION__, __LINE__, description, buffer, count);

#define HOST_NAME_MAX 64

const char *ps_control_options=""
"\n"
"psc version 0.7\n"
"\n"
"With this program you can control a program space.\n"
"\n"
"The first parameter after the programname specifies the name of\n"
"the program space you want to control.\n"
"\n"
"You have the following options:\n"
"\n"
"psc [name] --create [logfilename]\n"
"    Creates a new program space daemon. The PID of this daemon\n"
"    is written to STDOUT, a PID of zero indicates an error.\n"
"    After the daemon is created only the PID and UTS namespaces\n"
"    are unshared. The NET namespace can be unshared with the --net\n"
"    command, a new MOUNT namespace is created with the --chrootfs\n"
"    command. The logfilename is optional, specify a full path if\n"
"    you want the daemon to create a logfile for debugging purposes.\n"
"    In order to run this command you must be root.\n"
"\n"
"psc [name] --kill\n"
"    Kills the program space by sending SIGTERM to its daemon.\n"
"    You must be root to use this command.\n"
"\n"
"psc [name] --net\n"
"    Creates a new network namespace.\n"
"    The namespace starts with no network devices (not even lo).\n"
"    You have to create and configure all network interfaces.\n"
"    This command can only be given one time per program space and\n"
"    only by the root user\n"
"\n"
"psc [name] --ipc\n"
"    Creates a new IPC namespace.\n"
"    This command can only be given one time per program space and\n"
"    only by the root user\n"
"\n"
"psc [name] --chrootfs [path]\n"
"    Creates a new MOUNT namespace and changes the rootfs of the program\n"
"    space daemon. After this command the working directory of the\n"
"    daemon is set to /. This command can only be given one time per\n"
"    program space and only by the root user.\n"
"\n"
"psc [name] --cwd [path]\n"
"    Changes the working directory of the program space daemon\n"
"    to the specified path. All new programs running in program space\n"
"    start with this working directory.\n"
"\n"
"psc [name] --pid\n"
"    Writes the PID of the program space daemon to STDOUT.\n"
"    A PID of zero indicates an error.\n"
"\n"
"psc [name] --run [program path] [program params]\n"
"    Runs the specified program inside the program space. The user\n"
"    account used for running the program is equal to the account\n"
"    that invoked the psc command. The exit code of the psc command\n"
"    is 1 if psc encountered an internal error, or the exit code of\n"
"    the specified program.\n\n";

char PS_MSG_DATA               = 'D';
char PS_MSG_EXIT_STATUS        = 'X';
char PS_MSG_RUN_PROGRAM        = 'R';
char PS_MSG_GET_PID            = 'P';
char PS_MSG_CHANGE_WORKINGDIR  = 'W';
char PS_MSG_CHANGE_ROOTFS      = 'C';
char PS_MSG_UNSHARE_NETWORK    = 'N';
char PS_MSG_UNSHARE_IPC        = 'I';

int  ps_pid =0;
int  ps_fd_log=-1;
int  ps_fd_control=-1;
int  ps_daemon_run=1;
int  ps_safe_to_change_name = 0;
int  ps_log_stderr_output = 1;
int  ps_daemon_rootfs_changed = 0;
int  ps_daemon_network_changed = 0;
int  ps_daemon_ipc_changed = 0;

char *ps_daemon_logfilename=0;
char *ps_program_name;
char ps_log_buffer[256];
char ps_buffer[1024];
char ps_name[HOST_NAME_MAX - 4]; // 64 - (strlen("ps_")+1)
// note: IFNAMSIZ = 16 (incl term 0)
struct sockaddr_un ps_daemon_address;
struct termios     ps_saved_terminal_attributes;

//
// Logging
//

char* ps_get_log_time(void)
{
    static char log_time[128];
    time_t t;
    struct tm *tm;

    t = time(0);
    tm = localtime(&t);

    if (tm)
        if (strftime(log_time, sizeof(log_time), "%F %T", tm))
            return log_time;

    return strcpy(log_time,"get_log_time failed");
}

void ps_log(int exitcode, const char *format, ...)
{
    va_list arguments;

    if (ps_fd_log != -1)
    {
        sprintf(ps_log_buffer,"[%s] ",ps_get_log_time());
        write(ps_fd_log, ps_log_buffer, strlen(ps_log_buffer));
    }

    va_start(arguments, format);
    vsnprintf(ps_log_buffer, sizeof(ps_log_buffer)-1, format, arguments);
    va_end(arguments);

    ps_log_buffer[sizeof(ps_log_buffer)-1]=0;

    if (ps_fd_log != -1)
        write(ps_fd_log, ps_log_buffer, strlen(ps_log_buffer));

    if (ps_log_stderr_output)
        if (strncasecmp(ps_log_buffer, "error", 5) == 0)
        {
            write(STDERR_FILENO, "ERROR :",7);

            char *message_start=strchr(ps_log_buffer,']');

            if (message_start)
                message_start=strchr(message_start+1,']');

            message_start = (message_start) ? message_start+1 : ps_log_buffer;
            write(STDERR_FILENO, message_start, strlen(message_start));
        }

    if (exitcode)
        exit(exitcode);
}

void ps_log_hex_dump(const char* function_name, int line_number, char *description, void* address, int count)
{
    ps_log(0, "Hex   ps [%s] fn [%s] ln [%d] description [%s]\n", ps_name, function_name, line_number, description);

    char hex_buffer[16*3+1];
    char ascii_buffer[16*2+1];
    char *hex_pos=hex_buffer;
    char *hex_pos_max=hex_buffer+15*3;
    char *ascii_pos=ascii_buffer;
    char *c = (char *) address;

    while (count--)
    {
        sprintf(hex_pos,"%.2x ",(unsigned char) *c);

        if (isprint(*c))
            sprintf(ascii_pos,"%c ",*c);
        else
            sprintf(ascii_pos,". ");

        c++;

        if (hex_pos < hex_pos_max)
        {
            hex_pos+=3;
            ascii_pos+=2;
            continue;
        }

        ps_log(0, "Hex   %s- %s\n", hex_buffer, ascii_buffer);
        hex_pos=hex_buffer;
        ascii_pos=ascii_buffer;
    }

    if (hex_pos != hex_buffer)
        ps_log(0, "Hex   %s - %s\n", hex_buffer, ascii_buffer);
}

void ps_log_open(const char* filename)
{
    umask(0);

    if ((ps_fd_log=open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH)) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to open logfile [%s] - error [%s]"), filename, strerror(errno));
}

//
//  shared functions
//

void ps_init(void)
{
    ps_pid=0;
    memset(ps_name, 0, sizeof(ps_name));
    memset(&ps_daemon_address, 0, sizeof(ps_daemon_address));
}

void ps_set_name(const char* name)
{
    if (strlen(name)+1 > sizeof(ps_name))
        ps_log(EXIT_FAILURE, LOG_ERROR("program space name [%s] is too long, max length is [%d]"), name, sizeof(ps_name)-1);

    strcpy(ps_name, name);
    ps_name[sizeof(ps_name)-1]=0;

    memset(&ps_daemon_address, 0, sizeof(ps_daemon_address));
    ps_daemon_address.sun_family=AF_UNIX;

    // name is for an abstract unix socket, ie sun_path[0] == 0 thanks to memset
    sprintf(ps_daemon_address.sun_path+1,"ps_daemon_%s", ps_name);
}

void ps_create_directory(char *path)
{
    char *dir_start, *dir_end, *path_end;

    umask(0);
    path_end=path+strlen(path);
    dir_start=path+1;

    while (dir_start < path_end)
    {
        dir_end=strstr(dir_start,"/");

        if (dir_end)
            *dir_end=0;

        if (mkdir(path, 0755) == -1)
            if (errno != EEXIST)
                ps_log(EXIT_FAILURE, LOG_ERROR("unable to create directory [%s] - error [%s]"), path, strerror(errno));

        if (dir_end == NULL)
            break;

        *dir_end='/';
        dir_start=dir_end+1;
    }
}

//
//  ps_program code
//

void ps_program_exec(int fd_master, char *arguments)
{
    struct winsize windowsize;
    char *argv[64];
    char *p;
    int i, n, fd_slave;

    if (setsid() == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("setsid failed - error [%s]"), strerror(errno));

    fd_slave=open(ptsname(fd_master), O_RDWR);

    if (fd_slave == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("unable to open slave with name [%s] - error [%s]"), ptsname(fd_master), strerror(errno));

    ps_log(0, LOG_INFO("fd_master [%d] fd_slave [%d] ptsname [%s]"), fd_master, fd_slave, ptsname(fd_master));

    if (dup2(fd_slave, STDIN_FILENO) != STDIN_FILENO)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to set STDIN_FILENO - error [%s]"), strerror(errno));

    if (dup2(fd_slave, STDOUT_FILENO) != STDOUT_FILENO)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to set STDOUT_FILENO - error [%s]"), strerror(errno));

    if (dup2(fd_slave, STDERR_FILENO) != STDERR_FILENO)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to set STDERR_FILENO - error [%s]"), strerror(errno));

    if (fd_slave > STDERR_FILENO)
        close(fd_slave);

    close(fd_master);

    // get terminal columns and rows

    memset(&windowsize, 0, sizeof(windowsize));

    p=arguments;
    windowsize.ws_col=atoi(p);
    p+=strlen(p)+1;
    windowsize.ws_row=atoi(p);
    p+=strlen(p)+1;

    if (ioctl(STDOUT_FILENO, TIOCSWINSZ, &windowsize) == -1)
        ps_log(0, LOG_INFO("failed to set window size to w [%d] h [%d] - error [%s]"), windowsize.ws_col, windowsize.ws_row, strerror(errno));

    // program arguments

    n=atoi(p);
    p+=strlen(p)+1;

    for (i=0; i < n; i++)
    {
        argv[i]=p;
        p+=strlen(p)+1;
    }

    argv[i]=0;
    execvp(argv[0], argv);

    // this code only executes if execvp failes
    ps_log(EXIT_FAILURE, LOG_ERROR("failed to run program [%s] - error [%s]"), argv[0], strerror(errno));
}

//
//  ps_terminal code
//

void ps_terminal_io_loop(int fd_control, int fd_master)
{
    fd_set fdset;
    int fd_max, read_count, write_count, count;

    ps_buffer[0]=PS_MSG_DATA;
    fd_max=(fd_control > fd_master) ? fd_control : fd_master;

    while (1)
    {
        FD_ZERO(&fdset);
        FD_SET(fd_control, &fdset);
        FD_SET(fd_master, &fdset);

        if (select(fd_max+1, &fdset, 0, 0, 0) == -1)
            ps_log(EXIT_FAILURE, LOG_ERROR("select failed - error [%s]"), strerror(errno));

        // handle data comming from pseudo terminal

        if (FD_ISSET(fd_master, &fdset))
        {
            ps_buffer[0]=PS_MSG_DATA;
            read_count=read(fd_master, ps_buffer+1, sizeof(ps_buffer)-1);

            if (read_count == 0)
            {
                ps_log(0, LOG_INFO("fd_master closed"));
                break;
            }

            if (read_count == -1)
            {
                if (errno == EINTR)
                    continue;

                if (errno == EIO)
                {   // not an error, program running in terminal has stopped
                    ps_log(0, LOG_INFO("got EIO"));
                    break;
                }

                ps_log(EXIT_FAILURE, LOG_ERROR("failed to read from terminal - error [%s]"), strerror(errno));
            }

            count=read_count+1;

            while (1)
            {
                write_count=write(fd_control, ps_buffer, count);

                if (write_count == 0)
                    continue;

                if (write_count == -1)
                {
                    if (errno == EINTR)
                        continue;

                    ps_log(EXIT_FAILURE, LOG_ERROR("failed to write to control socket - error [%s]"), strerror(errno));
                }

                if (write_count != count) // write must be atomic
                    ps_log(EXIT_FAILURE, LOG_ERROR("partial write to control socket"));

                break;
            }
        }

        // handle data comming from users terminal

        if (FD_ISSET(fd_control, &fdset))
        {
            read_count=read(fd_control, ps_buffer, sizeof(ps_buffer));

            if (read_count == 0)
                break;

            if (read_count == -1)
            {
                if (errno == EINTR)
                    continue;

                ps_log(EXIT_FAILURE, LOG_ERROR("failed to read from control socket - error [%s]"), strerror(errno));
            }

            // TODO check for control messages, eg change terminal attributes
            // for now assume only data messages

            write_count=1;

            while (write_count != read_count)
            {
                count=write(fd_master, ps_buffer+write_count, read_count-write_count);
                //LOG_BUFFER_HEX("write to terminal", ps_buffer+write_count, count);

                if (count == 0)
                    continue;

                if (count == -1)
                {
                    if (errno == EINTR)
                        continue;

                    ps_log(EXIT_FAILURE, LOG_ERROR("failed to write to terminal - error [%s]"), strerror(errno));
                }

                write_count+=count;
            }
        }
    }
}

void ps_terminal_set_environment(const char *name, const char *value)
{
    int name_length, value_length;
    char *entry;

    name_length=strlen(name);
    value_length=strlen(value);

    entry= malloc(name_length+value_length+2);
    strcpy(entry, name);
    entry[name_length]='=';
    strcpy(entry+name_length+1, value);
    putenv(entry);
}

int ps_terminal_set_user(int uid)
{
    if (getuid() == uid)
        return 0;

    struct passwd *password_entry;

    // a program space can have a completely different rootfs, one in which some
    // users are not in the passwd file. do not allow these users to enter

    if ((password_entry= getpwuid(uid)) == 0)
    {
        ps_log(0, LOG_ERROR("getpwuid failed - error [%s]"), strerror(errno));
        return EXIT_FAILURE;
    }

    if (setgid(password_entry->pw_gid))
    {
        ps_log(0, LOG_ERROR("setgid failed - error [%s]"), strerror(errno));
        return EXIT_FAILURE;
    }

    if (setuid(uid))
    {
        ps_log(0, LOG_ERROR("setuid failed - error [%s]"), strerror(errno));
        return EXIT_FAILURE;
    }

    // modify environment

    ps_terminal_set_environment("HOME", password_entry->pw_dir);
    ps_terminal_set_environment("SHELL", password_entry->pw_shell);
    ps_terminal_set_environment("USER", password_entry->pw_name);
    ps_terminal_set_environment("LOGNAME", password_entry->pw_name);
    return EXIT_SUCCESS;
}

void ps_terminal_send_exitcode(int fd_control, int exitcode)
{
    ps_buffer[0]=PS_MSG_EXIT_STATUS;
    sprintf(ps_buffer+1,"%d", exitcode);

    if (write(fd_control, ps_buffer, strlen(ps_buffer)) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to send exitcode to controller - error [%s]"), strerror(errno));
}

void ps_terminal_run_program(int fd_control, char *arguments)
{
    int fd_master, pid_program, exit_status;
    socklen_t sizeof_credentials;
    struct ucred credentials;

    // run the program under the uid of the user that connected to the server

    sizeof_credentials = sizeof(credentials);

    if(getsockopt(fd_control, SOL_SOCKET, SO_PEERCRED, &credentials, &sizeof_credentials) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to get uid of controlling user - error [%s]"), strerror(errno));

    if (ps_terminal_set_user(credentials.uid) == EXIT_FAILURE)
    {
        ps_terminal_send_exitcode(fd_control, EXIT_FAILURE);
        return;
    }

    // create a pseudoterminal

    fd_master = posix_openpt(O_RDWR | O_NOCTTY);

    if (fd_master == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("posix_openpt() failed -error [%s]"), strerror(errno));

    if (grantpt(fd_master) == -1)
        ps_log(0 /*EXIT_FAILURE*/, LOG_ERROR("grantpt() failed - error [%s]"), strerror(errno));

    if (unlockpt(fd_master) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("unlockpt() failed - error [%s]"), strerror(errno));

    // fork a new process to run the program

    pid_program=fork();

    if (pid_program == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("unable to fork - error [%s]"), strerror(errno));

    if (pid_program == 0)
    {
        close(fd_control);
        ps_program_exec(fd_master, arguments);
        ps_log(EXIT_FAILURE, LOG_ERROR("trespassing error!"));
    }

//  TEST for a problem with bash. It seems that bash disables cntl-d (eof) handling
//  in the pseudo terminal if there is input before it outputs its prompt. This requires
//  us to use the exit statement in a script that is inside a here-document. The sh shell
//  does not have this behaviour. Needs further investigation...
//sleep(1);

    ps_terminal_io_loop(fd_control, fd_master);

    if (waitpid(pid_program, &exit_status, 0) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("waitpid failed - error [%s]"), strerror(errno));

    // return status to controller

    ps_log(0, LOG_INFO("after wait - exit_status [%d]"), exit_status);
    ps_terminal_send_exitcode(fd_control, exit_status);
    close(fd_master);
    close(fd_control);
}

//
//  ps_daemon code
//

int ps_daemon_change_rootfs(char *newroot)
{
    ps_log(0, LOG_INFO("change root dir to [%s]"), newroot);

    if (ps_daemon_rootfs_changed)
    {
        ps_log(0, LOG_ERROR("rootfs already changed!"));
        return EXIT_FAILURE;
    }

    if (chdir(newroot) == -1)
    {
        ps_log(0, LOG_ERROR("failed to change rootdir to [%s] - error [%s]"), newroot, strerror(errno));
        return EXIT_FAILURE;
    }

    // create a directory where pivot_root can attach us_root

    if (mkdir("./us_rootfs", 0755) == -1)
    {
        if (errno != EEXIST)
        {
            ps_log(0, LOG_ERROR("failed to create directory /us_rootfs - error [%s]"), strerror(errno));
            return EXIT_FAILURE;
        }
    }

    if (unshare(CLONE_NEWNS) == -1)
    {
        ps_log(0, LOG_ERROR("unable to unshare mount namespace - error [%s]"), strerror(errno));
        return EXIT_FAILURE;
    }

	if (pivot_root(".", "./us_rootfs") == -1)
    {
		ps_log(0, LOG_ERROR("pivot_root failed - error [%s]"), strerror(errno));
        return EXIT_FAILURE;
	}

    // unmount user space rootfs

    if (mount("", "/us_rootfs", "none", MS_REC | MS_PRIVATE, "") == -1)
    {
        ps_log(0, LOG_ERROR("failed to re-mount /us_rootfs as private - error [%s]"), strerror(errno));
        return EXIT_FAILURE;
    }

    if (umount2("/us_rootfs", MNT_DETACH) == -1)
    {
        ps_log(0, LOG_ERROR("failed to unmount /us_rootfs - error [%s]"), strerror(errno));
        return EXIT_FAILURE;
    }

    if (rmdir("/us_rootfs") == -1)
        ps_log(0, LOG_ERROR("failed to remove directory /us_rootfs - error [%s]"), strerror(errno));


	if (chdir("/") == -1)
    {
        ps_log(0, LOG_ERROR("failed to change to / - error [%s]"), strerror(errno));
        return EXIT_FAILURE;
	}

    if (mkdir("/proc", 0755) == -1);
        if (errno != EEXIST)
        {
            ps_log(0, LOG_ERROR("failed to create /proc directory - error [%s]"), strerror(errno));
            return EXIT_FAILURE;
        }

    if (mount("", "/proc","proc", MS_NODEV | MS_NOEXEC | MS_NOSUID,"") == -1)
    {
        ps_log(0, LOG_ERROR("failed to re-mount /proc - error [%s]"), strerror(errno));
        return EXIT_FAILURE;
    }

    if (mkdir("/dev", 0755) == -1);
        if (errno != EEXIST)
        {
            ps_log(0, LOG_ERROR("failed to create /dev directory - error [%s]"), strerror(errno));
            return EXIT_FAILURE;
        }

    if (mkdir("/dev/pts", 0755) == -1);
        if (errno != EEXIST)
        {
            ps_log(0, LOG_ERROR("failed to create /dev/pts directory - error [%s]"), strerror(errno));
            return EXIT_FAILURE;
        }

	if (mount("devpts", "/dev/pts", "devpts", MS_MGC_VAL , "newinstance,ptmxmode=0666") == -1)
    {
		ps_log(0, LOG_ERROR("unable to mount /dev/pts - error [%s]"), strerror(errno));
		return EXIT_FAILURE;
	}

	if (access("/dev/ptmx", F_OK) == -1)
    {
		if (symlink("/dev/pts/ptmx", "/dev/ptmx") == -1)
            ps_log(0, LOG_ERROR("unable to create /dev/ptmx symlink - error [%s]"), strerror(errno));
	}

    ps_daemon_rootfs_changed=1;
    return EXIT_SUCCESS;
}

void ps_daemon_run_program(int fd_listen, int fd_control, char *arguments)
{
    struct sigaction sa_sigchld;

    // fork a terminal program that creates a pseudoterminal and takes care of running the program

    int pid_terminal=fork();

    if (pid_terminal == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to create terminal program - error [%s]"), strerror(errno));

    if (pid_terminal == 0)
    {
        close(fd_listen);

        if (ps_safe_to_change_name)
        {
            ps_program_name[2]='t';
            prctl(PR_SET_NAME, (unsigned long) ps_program_name, 0, 0, 0);
        }

        // remove inherited SIGCHLD handler

        sigemptyset(&sa_sigchld.sa_mask);
        sa_sigchld.sa_handler = SIG_DFL;
        sa_sigchld.sa_flags = 0;

        if (sigaction(SIGCHLD, &sa_sigchld, 0) == -1)
            ps_log(EXIT_FAILURE, LOG_ERROR("failed to set SIGCHLD handler - error [%s]"), strerror(errno));

        ps_terminal_run_program(fd_control, arguments);
        exit(EXIT_SUCCESS);
    }

    // NOTE: the daemon does not wait on pid_terminal
}

void ps_daemon_io_loop(int fd_initdata)
{
    int fd_listen, fd_control, read_count;

    // open listening socket

    if ((fd_listen=socket(AF_UNIX, SOCK_SEQPACKET, 0)) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to create socket - error [%s]"), strerror(errno));

    if (bind(fd_listen, (struct sockaddr *) &ps_daemon_address, sizeof(ps_daemon_address)) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to bind socket - error [%s]"), strerror(errno));

    if (listen(fd_listen, 5) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to listen on socket - error [%s]"), strerror(errno));

    // send ps_pid to process that ceated the daemon

    if (write(fd_initdata, &ps_pid, sizeof(ps_pid)) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to send ps_pid to controler - error [%s]"), strerror(errno));

    close(fd_initdata);

    while (ps_daemon_run)
    {
        fd_control=accept(fd_listen, 0, 0);

        if (fd_control == -1)
        {
            if (errno == EINTR)
                continue;

            ps_log(EXIT_FAILURE, LOG_ERROR("failed to accept controller connection - error [%s]"), strerror(errno));
        }

        while ((read_count = read(fd_control, ps_buffer, sizeof(ps_buffer))))
        {
            if (read_count == -1)
            {
                if (errno == EINTR)
                    continue;

                ps_log(0, LOG_INFO("controller disconnected - error [%s]"), strerror(errno));
                break;
            }

            // process control messages

            if (ps_buffer[0] == PS_MSG_RUN_PROGRAM)
            {
                ps_daemon_run_program(fd_listen, fd_control, ps_buffer+1);
                break;
            }

            if (ps_buffer[0] == PS_MSG_GET_PID)
            {
                sprintf(ps_buffer+1,"%d", ps_pid);

                if (write(fd_control, ps_buffer, strlen(ps_buffer)+1) == -1)
                    ps_log(0, LOG_ERROR("failed to send PID to controller - error [%s]"), strerror(errno));

                break;
            }

            if (ps_buffer[0] == PS_MSG_CHANGE_WORKINGDIR)
            {
                ps_buffer[0]= PS_MSG_EXIT_STATUS;

                if (chdir(ps_buffer+1) == -1)
                {
                    ps_log(0, LOG_ERROR("failed to change working dir to [%s] - error [%s]"), ps_buffer+1, strerror(errno));
                    sprintf(ps_buffer+1,"%d", EXIT_FAILURE);
                }
                else
                    sprintf(ps_buffer+1,"%d", EXIT_SUCCESS);

                if (write(fd_control, ps_buffer, strlen(ps_buffer)+1) == -1)
                    ps_log(0, LOG_ERROR("failed to send resultcode of CWD to controller - error [%s]"), strerror(errno));

                break;
            }

            if (ps_buffer[0] == PS_MSG_UNSHARE_NETWORK)
            {
                ps_buffer[0] = PS_MSG_EXIT_STATUS;

                if (ps_daemon_network_changed)
                {
                    ps_log(0, LOG_ERROR("network already unshared"));
                    sprintf(ps_buffer+1,"%d", EXIT_FAILURE);
                }
                else
                {
                    if (unshare(CLONE_NEWNET) == -1)
                    {
                        ps_log(0, LOG_ERROR("failed to unshare network - error [%s]"), strerror(errno));
                        sprintf(ps_buffer+1,"%d", EXIT_FAILURE);
                    }
                    else
                    {
                        ps_daemon_network_changed=1;
                        sprintf(ps_buffer+1,"%d", EXIT_SUCCESS);
                    }
                }

                if (write(fd_control, ps_buffer, strlen(ps_buffer)+1) == -1)
                    ps_log(0, LOG_ERROR("failed to sent resultcode of UNSHARE NETWORK to controller - error [%s]"), strerror(errno));

                break;
            }

            if (ps_buffer[0] == PS_MSG_UNSHARE_IPC)
            {
                ps_buffer[0] = PS_MSG_EXIT_STATUS;

                if (ps_daemon_ipc_changed)
                {
                    ps_log(0, LOG_ERROR("ipc already unshared"));
                    sprintf(ps_buffer+1,"%d", EXIT_FAILURE);
                }
                else
                {
                    if (unshare(CLONE_NEWIPC) == -1)
                    {
                        ps_log(0, LOG_ERROR("failed to unshare ipc - error [%s]"), strerror(errno));
                        sprintf(ps_buffer+1,"%d", EXIT_FAILURE);
                    }
                    else
                    {
                        ps_daemon_ipc_changed=1;
                        sprintf(ps_buffer+1,"%d", EXIT_SUCCESS);
                    }
                }

                if (write(fd_control, ps_buffer, strlen(ps_buffer)+1) == -1)
                    ps_log(0, LOG_ERROR("failed to sent resultcode of UNSHARE IPC to controller - error [%s]"), strerror(errno));

                break;
            }

            if (ps_buffer[0] == PS_MSG_CHANGE_ROOTFS)
            {
                ps_buffer[0] = PS_MSG_EXIT_STATUS;
                sprintf(ps_buffer+1,"%d", ps_daemon_change_rootfs(ps_buffer+1));

                if (write(fd_control, ps_buffer, strlen(ps_buffer)+1) == -1)
                    ps_log(0, LOG_ERROR("failed to sent resultcode of CHROOTFS to controller - error [%s]"), strerror(errno));

                break;
            }
        }

        close(fd_control);
    }
}

static void ps_daemon_sigchld_handler(int sig)
{
    int errno_process=errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        continue;

    errno=errno_process;
}

static void ps_daemon_sigterm_handler(int sig)
{
    ps_daemon_run=0;
}

void ps_daemon_main(int fd_initdata)
{
    struct sigaction sa_sigchld, sa_sigterm;
    int fd;

    if (ps_safe_to_change_name)
    {
        ps_program_name[2]='d';
        prctl(PR_SET_NAME, (unsigned long) ps_program_name, 0, 0, 0);
    }

    ps_log_stderr_output=0;
    chdir("/");
    umask(0);

    // close open files

    for (fd = 0; fd < 256;  fd++)
        if (fd != fd_initdata)
            close(fd);

    // sleep(5); /* NOTE: test if controller only returns after daemon up and running */

    fd = open("/dev/null", O_RDWR);

    if (fd != STDIN_FILENO)
        return;

    if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
        return;

    if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
        return;

    // NOTE: inherted log already closed..

    ps_fd_log=-1;
    if (strcmp(ps_daemon_logfilename, "none") != 0)
        ps_log_open(ps_daemon_logfilename);

    ps_log(0, LOG_INFO("daemon started"));

    if (read(fd_initdata, &ps_pid, sizeof(ps_pid)) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("did not receive daemon pid - error [%s]"), strerror(errno));

    sigemptyset(&sa_sigchld.sa_mask);
    sa_sigchld.sa_handler = &ps_daemon_sigchld_handler;
    sa_sigchld.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa_sigchld, 0) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to set SIGCHLD handler - error [%s]"), strerror(errno));

    sigemptyset(&sa_sigterm.sa_mask);
    sa_sigterm.sa_handler = &ps_daemon_sigterm_handler;
    sa_sigterm.sa_flags = 0;

    if (sigaction(SIGTERM, &sa_sigterm, 0) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to set SIGTERM handler - error [%s]"), strerror(errno));

    // change hostname

    snprintf(ps_buffer,sizeof(ps_buffer)-1,"ps_%s", ps_name);
    ps_buffer[sizeof(ps_buffer)-1]=0;
    ps_log(0, LOG_INFO("setting hostname to [%s]"), ps_buffer);

    if (sethostname(ps_buffer, strlen(ps_buffer)) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to change hostname - error [%s]"), strerror(errno));

    ps_daemon_io_loop(fd_initdata);

    ps_log(0, LOG_INFO("daemon stopped"));
}

int ps_daemon_main_wrapper(void *data)
{
    int fd_initdata = *((int *) data);

    ps_daemon_main(fd_initdata);
    return 0;
}

//
//  ps_controler code
//

void ps_control_create_daemon(void)
{
    int fd_initdata[2];

    ps_log(0, LOG_INFO("creating daemon"));
    ps_safe_to_change_name=(strcmp(ps_program_name,"psc") == 0) ? 1 : 0;

    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fd_initdata) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to create socketpair for daemon initialization - error [%s]"), strerror(errno));

    // become a child of the init process by forking and closing the parent process.

    switch (fork())
    {
        case -1:
            ps_log(EXIT_FAILURE, LOG_ERROR("unable to create daemon - fork 1"));

        case 0:
            // become the leader of new session

            if (setsid() == -1)
                return;

            // do another fork to prevent the daemon to open a controlling terminal in the future.
            // use the clone() function to create new PID and UTS namespaces for the child.

            long stack_size = 4*sysconf(_SC_PAGESIZE);
            char *stack;
            int  pid_daemon;

            if (!(stack = malloc(stack_size)))
                ps_log(EXIT_FAILURE, LOG_ERROR("failed to allocate stack"));

            pid_daemon=clone(ps_daemon_main_wrapper, stack+stack_size, CLONE_NEWPID | CLONE_NEWUTS | SIGCHLD, (void *) (fd_initdata+1));

            close(fd_initdata[1]);

            if (pid_daemon == -1)
                ps_log(EXIT_FAILURE, LOG_ERROR("unable to create daemon - fork 2"));

            // send a message to the newly created daemon

            if (write(fd_initdata[0], &pid_daemon, sizeof(pid_daemon)) == -1)
                ps_log(EXIT_FAILURE, LOG_ERROR("failed to send daemon pid to daemon - error [%s]"), strerror(errno));

            _exit(EXIT_SUCCESS);

        default:
            // wait for daemon response

            close(fd_initdata[1]);

            int read_count=read(fd_initdata[0], &ps_pid, sizeof(ps_pid));

            if (read_count < sizeof(ps_pid))
                ps_pid=0;

            sprintf(ps_buffer,"%d\n", ps_pid);
            write(STDOUT_FILENO, ps_buffer, strlen(ps_buffer));

            if (read_count < sizeof(ps_pid))
                ps_log(EXIT_FAILURE, LOG_ERROR("did not receive daemon pid"));

            // daemon created and ready to accept connections
            ps_log(0, LOG_INFO("ps_pid [%d]"), ps_pid);
    }
}

void ps_control_connect_daemon()
{
    ps_fd_control=socket(AF_UNIX, SOCK_SEQPACKET, 0);

    if (ps_fd_control == -1)
        ps_log(0, LOG_ERROR("failed to create socket - error [%s]"), strerror(errno));
    else
    {
        if (connect(ps_fd_control, (struct sockaddr *) &ps_daemon_address, sizeof(ps_daemon_address)) == -1)
        {
            ps_log(0, LOG_ERROR("failed to connect to daemon - error [%s]"), strerror(errno));
            close(ps_fd_control);
            ps_fd_control=-1;
        }
    }
}

void ps_control_io_loop()
{
    // read from STDIN -> fd_control
    // read from fd_control -> STDOUT
    // stop if fd_control closed

    fd_set fdset;
    int read_count, write_count, count, stdin_closed;

    stdin_closed=0;

    while (1)
    {
        FD_ZERO(&fdset);
        FD_SET(ps_fd_control, &fdset);

        if (!stdin_closed)
            FD_SET(STDIN_FILENO, &fdset);

        if (select(ps_fd_control+1, &fdset, 0,0,0) == -1)
            ps_log(EXIT_FAILURE, LOG_ERROR("select failed - error [%s]"), strerror(errno));

        memset(ps_buffer,0,sizeof(ps_buffer));

        if (FD_ISSET(ps_fd_control, &fdset))
        {
            read_count=read(ps_fd_control, ps_buffer, sizeof(ps_buffer));

            if (read_count == 0) // EOF
                break;

            if (read_count == -1)
            {
                if (errno == EINTR)
                    continue;

                ps_log(0, LOG_ERROR("failed to read from control socket - error [%s]"), strerror(errno));
                break;
            }

            if (ps_buffer[0] == PS_MSG_EXIT_STATUS)
            {
                ps_buffer[read_count]=0;
                ps_log(0, LOG_INFO("received exit status [%s]"), ps_buffer+1);
                exit(strtol(ps_buffer+1, 0, 10));
            }

            if (ps_buffer[0] == PS_MSG_GET_PID)
            {
                ps_log(0, LOG_INFO("received pid [%s]"), ps_buffer+1);
                write(STDOUT_FILENO, ps_buffer+1, strlen(ps_buffer+1));
                write(STDOUT_FILENO, "\n", 1);
                exit(EXIT_SUCCESS);
            }

            write_count=1;

            while (write_count != read_count)
            {
                count=write(STDOUT_FILENO, ps_buffer+write_count, read_count-write_count);

                if (count == 0)
                    continue;

                if (count == -1)
                {
                    if (errno == EINTR)
                        continue;

                    ps_log(0, LOG_ERROR("failed to write to stdout - error [%s]"), strerror(errno));
                    break;
                }

                write_count+=count;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &fdset))
        {
            ps_buffer[0]=PS_MSG_DATA;
            read_count=read(STDIN_FILENO, ps_buffer+1, sizeof(ps_buffer)-1);

            if (read_count == 0) // EOF
            {
                stdin_closed=1;

                // send cntrl-d to the program space terminal

                ps_buffer[0]=PS_MSG_DATA;
                ps_buffer[1]=4;

                if (write(ps_fd_control, ps_buffer, 2) == -1)
                    ps_log(0, LOG_ERROR("failed to write cntrl-d to pst - error [%s]"), strerror(errno));

                continue;
            }

            if (read_count == -1)
            {
                if (errno == EINTR)
                    continue;

                ps_log(EXIT_FAILURE, LOG_ERROR("failed to read from terminal stdin - error [%s]"), strerror(errno));
            }

            read_count++;
            write_count=0;

            while (write_count != read_count)
            {
                count=write(ps_fd_control, ps_buffer+write_count, read_count-write_count);
                //LOG_BUFFER_HEX("sending data", ps_buffer, count);

                if (count == 0)
                    continue;

                if (count == -1)
                {
                    if (errno == EINTR)
                        continue;

                    ps_log(EXIT_FAILURE, LOG_ERROR("failed to write to control socket - error [%s]"), strerror(errno));
                }

                write_count+=count;
            }
        }
    }
}

int ps_control_send_receive(int size)
{
    int read_count=0;

    if (ps_fd_control == -1)
        return -1;

    if (write(ps_fd_control, ps_buffer, size) == -1)
    {
        ps_log(0, LOG_ERROR("failed to send message of type [%c] - error [%s]"), ps_buffer, strerror(errno));
        return -1;
    }

    while (1)
    {
        read_count=read(ps_fd_control, ps_buffer, sizeof(ps_buffer));

        if (read_count != -1)
            break;

        if (errno == EINTR)
            continue;

        ps_log(0, LOG_ERROR("failed to read response - error [%s]"), strerror(errno));
        break;
    }

    if (ps_buffer[0] == PS_MSG_EXIT_STATUS)
    {
        ps_buffer[read_count]=0;
        ps_log(0, LOG_INFO("received exit status [%s]"), ps_buffer+1);
        exit(strtol(ps_buffer+1, 0, 10));
    }

    return read_count;
}

void ps_control_run_program(int argc, char *argv[])
{
    int i;
    char *p;
    struct winsize windowsize;

    if (argc <= 0)
        ps_log(EXIT_FAILURE, LOG_ERROR("invallid number of arguments [%d]"), argc);

    if (isatty(STDIN_FILENO))
    {
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, &windowsize) == -1)
            ps_log(EXIT_FAILURE, LOG_ERROR("failed to determine window size"));
    }
    else
    {
        windowsize.ws_col=80;
        windowsize.ws_row=25;
    }

    p=ps_buffer;
    *p++=PS_MSG_RUN_PROGRAM;

    sprintf(p,"%d",windowsize.ws_col);
    p+=strlen(p)+1;

    sprintf(p,"%d",windowsize.ws_row);
    p+=strlen(p)+1;

    sprintf(p,"%d",argc);
    p+=strlen(p)+1;

    int arg_size;
    int buffer_size_left=sizeof(ps_buffer)-(p-ps_buffer);

    for (i=0; i < argc; i++)
    {
        arg_size=strlen(argv[i])+1;

        if (arg_size > buffer_size_left)
            ps_log(EXIT_FAILURE, LOG_ERROR("size of --run parameters too large for buffer."));

        sprintf(p,"%s",argv[i]);
        p+=arg_size;
        buffer_size_left-=arg_size;
    }

    if ((write(ps_fd_control, ps_buffer, p-ps_buffer)) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("failed to send PS_MSG_RUN_PROGRAM - error [%s]"), strerror(errno));

    ps_control_io_loop();
}

int ps_control_get_daemon_pid(void)
{
    sprintf(ps_buffer,"%c",PS_MSG_GET_PID);

    if (ps_control_send_receive(strlen(ps_buffer)+1) < 2)
        return -1;

    return atoi(ps_buffer+1);
}

void ps_control_change_workingdir(char *workingdir)
{
    snprintf(ps_buffer, sizeof(ps_buffer)-1, "%c%s", PS_MSG_CHANGE_WORKINGDIR, workingdir);
    ps_buffer[sizeof(ps_buffer)-1]=0;
    ps_control_send_receive(strlen(ps_buffer)+1);
}

void ps_control_unshare_network()
{
    sprintf(ps_buffer, "%c", PS_MSG_UNSHARE_NETWORK);
    ps_control_send_receive(strlen(ps_buffer)+1);
}

void ps_control_unshare_ipc()
{
    sprintf(ps_buffer, "%c", PS_MSG_UNSHARE_IPC);
    ps_control_send_receive(strlen(ps_buffer)+1);
}

void ps_control_change_rootfs(char *rootdir)
{
    snprintf(ps_buffer, sizeof(ps_buffer)-1, "%c%s", PS_MSG_CHANGE_ROOTFS, rootdir);
    ps_buffer[sizeof(ps_buffer)-1]=0;
    ps_control_send_receive(strlen(ps_buffer)+1);
}

void ps_control_save_terminal_attributes(void)
{
    if (tcgetattr(STDIN_FILENO, &ps_saved_terminal_attributes) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("unable to save terminal attributes - error [%s]"), strerror(errno));
}

void ps_control_set_terminal_raw_mode(void)
{
    struct termios terminal_attributes_raw = ps_saved_terminal_attributes;

    terminal_attributes_raw.c_lflag &= ~(ICANON | ISIG | IEXTEN |ECHO);
    terminal_attributes_raw.c_iflag &= ~(BRKINT | ICRNL |IGNBRK | IGNCR | IGNCR | INPCK | ISTRIP | IXON | PARMRK);
    terminal_attributes_raw.c_oflag &= ~OPOST;
    terminal_attributes_raw.c_cc[VMIN] = 1;
    terminal_attributes_raw.c_cc[VTIME] =0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH /*TCSANOW*/ , &terminal_attributes_raw) == -1)
        ps_log(EXIT_FAILURE, LOG_ERROR("unable to put terminal in raw mode - error [%s]"), strerror(errno));
}

void ps_control_restore_terminal_attributes(void)
{
    if (tcsetattr(STDIN_FILENO, TCSANOW, &ps_saved_terminal_attributes) == -1)
        ps_log(0, LOG_ERROR("unable to restore terminal attributes - error [%s]"), strerror(errno));
}

typedef struct
{
    char *name;
    int  needs_root;
    int  argc_min;
    int  argc_max;
} ps_command_info_t;

const ps_command_info_t ps_command_infos[]=
{
    {"--create"  , 1, 0, 1},
    {"--chrootfs", 1, 1, 1},
    {"--net"     , 1, 0, 0},
    {"--ipc"     , 1, 0, 0},
    {"--kill"    , 1, 0, 0},
    {"--pid"     , 0, 0, 0},
    {"--cwd"     , 0, 1, 1},
    {"--run"     , 0, 1, 50},
    {0           , 0, 0, 0}
};

void ps_control_check_input(char *command, int argc)
{
    ps_command_info_t const *command_info = ps_command_infos;

    while (command_info->name)
    {
        if (strcmp(command_info->name, command) != 0)
        {
            command_info++;
            continue;
        }

        if (command_info->needs_root)
        {
            if (getuid() != 0)
                ps_log(EXIT_FAILURE, LOG_ERROR("The command [%s] requires root permission"), command);
        }

        if ((argc < command_info->argc_min) || (argc > command_info->argc_max))
            ps_log(EXIT_FAILURE, LOG_ERROR("Invallid number of parameters for command [%s]"), command);

        return;
    }

    ps_log(EXIT_FAILURE, LOG_ERROR("Unknown command [%s]"), command);
}

void ps_control_open_logfile()
{
    char *filename, *slashpos;

    if ((filename=getenv("PSC_LOGFILENAME")) == NULL)
        return;

    if ((slashpos=strrchr(filename,'/')))
    {
        *slashpos=0;
        ps_create_directory(filename);
        *slashpos='/';
    }

    ps_log_open(filename);
}

void ps_control_prepare_daemon_logfile(char *filename)
{
    char *slashpos;

    ps_daemon_logfilename=strdup(filename);

    if (strcmp(filename,"none") == 0)
        return;

    if ((slashpos=strrchr(filename,'/')))
    {
        *slashpos=0;
        ps_create_directory(filename);
        *slashpos='/';
    }
}

int main(int argc, char *argv[])
{
    ps_program_name=argv[0];
    ps_control_open_logfile();

    if (argc == 2)
        if ( (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
        {
            printf("%s", ps_control_options);
            return EXIT_SUCCESS;
        }

    if (argc < 3)
        ps_log(EXIT_FAILURE, LOG_ERROR("too few parameters"));

    ps_init();
    ps_set_name(argv[1]);
    ps_control_check_input(argv[2], argc-3);
    ps_log(0, LOG_INFO("creating controller"));

    if (strcmp(argv[2],"--create") == 0)
    {
        ps_control_prepare_daemon_logfile((argc == 4) ? argv[3] : "none");
        ps_control_create_daemon();
    }
    else
    {
        ps_control_connect_daemon();

        if (strcmp(argv[2],"--pid") == 0)
        {
            ps_pid=ps_control_get_daemon_pid();
            printf("%d\n",(ps_pid == -1) ? 0 : ps_pid);

            if (ps_pid == -1)
                ps_log(EXIT_FAILURE, LOG_ERROR("--pid: failed to receive daemon PID"));
        }

        if (strcmp(argv[2],"--kill") == 0)
        {
            ps_pid=ps_control_get_daemon_pid();

            if (ps_pid <= 1)
                ps_log(EXIT_FAILURE, LOG_ERROR("--kill : could not determine daemon PID"));

            if (kill(ps_pid, SIGTERM) == -1)
                ps_log(EXIT_FAILURE, LOG_ERROR("--kill : failed to kill daemon - error [%s]"), strerror(errno));
        }

        if (ps_fd_control == -1)
            return EXIT_FAILURE;

        if (strcmp(argv[2],"--net") == 0)
            ps_control_unshare_network();

        if (strcmp(argv[2],"--ipc") == 0)
            ps_control_unshare_ipc();

        if (strcmp(argv[2],"--chrootfs") == 0)
            ps_control_change_rootfs(argv[3]);

        if (strcmp(argv[2],"--cwd") == 0)
            ps_control_change_workingdir(argv[3]);

        if (strcmp(argv[2],"--run") == 0)
        {
            if (isatty(STDIN_FILENO))
            {
                ps_control_save_terminal_attributes();
                atexit(ps_control_restore_terminal_attributes);
                ps_control_set_terminal_raw_mode();
            }

            ps_control_run_program(argc-3, argv+3);
        }
    }

    ps_log(0, LOG_INFO("closing controller"));
    return EXIT_SUCCESS;
}
