#include <time.h>
#include "datetime.h"

/* IMF-fixdate
   Example:
   Sun, 06 Nov 1994 08:49:37 GMT
 */
void get_current_time(char buf[29]) {
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buf, 30, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
}
