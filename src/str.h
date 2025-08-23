#ifndef HTTP_STR_H
#define HTTP_STR_H

#include <stdint.h>

#define SYM_SP ' '
#define SYM_HTAB '\t'
#define SYM_CR '\r'
#define SYM_LF '\n'
#define CRLF "\r\n"

/* return 1 if vchar, 0 otherwise */
int is_vchar(char ch);

/* return 1 if ALPHA, 0 otherwise */
int is_alpha(char ch);

/* return 1 if DIGIT, 0 otherwise */
int is_digit(char ch);

/* return 1 if HEXDIG, 0 otherwise */
int is_hexdig(char ch);

/* return 1 if tchar, 0 otherwise */
int is_tchar(char ch);

/* return 1 if obs-text, 0 otherwise */
int is_obs_text(char ch);

/* return 1 if pchar, 0 otherwise */
int is_pchar(char ch);

/* return 1 if allowed reg-name char, 0 otherwise */
int is_regchar(char ch);

/* if is ALPHA, lowercase */
char lower(char ch);

/* parse DIGIT into uint8_t */
uint8_t to_digit(char ch);

/* return 1 if qdtext, 0 otherwise */
int is_qdtext(char ch);

#endif
