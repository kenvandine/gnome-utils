#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>


void usage()
{
  puts("whoops!");
  exit(0);
}

gid_t gid;
uid_t uid;

int process_file(char *buf, char *ptr, int not_dir)
{
  puts(buf);

  if (!not_dir) {
    struct stat statbuf;
    
    stat(buf, &statbuf);
    
    if (statbuf.st_mode & S_IFDIR) {
      DIR *dir;
      
      dir=opendir(buf);
      
      if (dir !=NULL) {
	struct dirent *de;
	nlink_t lfound=2;
	int flag = (lfound == statbuf.st_nlink);
	
	ptr[0]='/';
	ptr++;
	ptr[1]=0;
	
	while (NULL != (de = readdir(dir))) {
	  if (de->d_name[0] != '.' &&
	      (de->d_name[1] != '\0' && de->d_name[1] != '.') &&
	      de->d_name[2] != '\0') {
	    /* Skip . and .. */
	    
	    strcpy(ptr,de->d_name);
	    
	    if (flag) {
	      process_file(buf, ptr+strlen(de->d_name), 1);
	    } else if (process_file(buf, ptr+strlen(de->d_name), 0)) {
	      lfound++;
	      if (lfound == statbuf.st_nlink) {
		flag=1;
	      } 
	    }
	  }
	}
	closedir(dir);
      }
      return 1;
    }
  } else {
    puts ("definitely not");
  }

  return 0;
}

int 
main(int argc, char **argv)
{
  int maxpath;
  int startlen;
  char *pathbuf;
  char *pathptr;

  if (argc<2) {
    usage();
  }

  startlen=strlen(argv[1]);
  maxpath=pathconf(argv[1],_PC_PATH_MAX);
  pathbuf=(char *) malloc(maxpath*sizeof(char));
  strcpy(pathbuf,argv[1]);
 
  if ('/' == pathbuf[startlen-1]) {
    pathbuf[startlen--]='0';
  }
  
  pathptr=pathbuf+startlen;

  puts("About to begin.");

  process_file (pathbuf,pathptr,0);
}
