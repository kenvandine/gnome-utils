#include <string.h>

static char *must_escape = ".+|()$^";

char *
glob2regexp(char *glob)
{
    char *result;
    int i, j, len;
    int brace_count=0;

    len=strlen(glob);
    result=(char *) malloc(2*len*sizeof(char)+1);
    
    for (i = 0, j = 0; i <= len; i++, j++) {
 
        if (NULL != strchr(must_escape,glob[i])) {

	  /* Quote most regexp meta-characters. */

            result[j++] = '\\';
            result[j] = glob[i];

        } else if (glob[i] == '\\') {

	  /* Allow backslash escapes. */

            if (i+1!=len) {

                result[j++] = '\\';
                result[j] = glob[++i];
                
            } else {

                result[j++] = '\\';
                result[j] = '\\';

            }

        } else if (glob[i]=='*') {

	  /* Replace '*' with '.*' */

            result[j++] = '.';
            result[j] = '*';

        } else if (glob[i] == '?') {

	  /* Replace '?' with '.*' */

            result[j] = '.';            

        } else if (glob[i] == '{') {
            
            result[j]='(';
            brace_count++;

        } else if (brace_count > 0 && glob[i] == ',') {

            result[j]='|';
            
        } else if (brace_count > 0 && glob[i] == '}') {

            result[j]=')';
            brace_count--;

        } else if (glob[i]='[') {

            result[j++] = result[i++];

            if (i<len) {
                result[j++] = result[i++];
            }

            while(i < len && result[i] != ']') {
                result[j++] = result[i++];
            }

            if (i<len) {
                result[j] = result[i];
            }

        } else {
            result[j]=glob[i];
        }
    }

    result[j]=0;

    return result;
}
