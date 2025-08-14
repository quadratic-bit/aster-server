#include <assert.h>
#include "str.h"

int is_vchar(char ch) {
	return ch >= '!' && ch <= '~';
}

int is_alpha(char ch) {
	return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

int is_digit(char ch) {
	return ch >= '0' && ch <= '9';
}

int is_hexdig(char ch) {
	return is_digit(ch) || (
		(ch >= 'a' && ch <= 'f') ||
		(ch >= 'A' && ch <= 'F')
	);
}

int is_tchar(char ch) {
	if (is_alpha(ch) || is_digit(ch)) return 1;
	if (!is_vchar(ch)) return 0;
	return ch == '!' || ch == '#' || ch == '$' || ch == '%' ||
		ch == '&' || ch == '\'' || ch == '*' || ch == '+' ||
		ch == '-' || ch == '.' || ch == '^' || ch == '_' || ch == '`' ||
		ch == '|' || ch == '~';
}

int is_obs_text(char ch) {
	unsigned char uch = (unsigned char)ch;
	return uch >= 0x80;
}

char lower(char ch) {
	if (ch >= 'A' && ch <= 'Z') {
		return ch | 0x20;
	}
	return ch;
}

uint8_t to_digit(char ch) {
	assert(is_digit(ch));
	return (uint8_t)((unsigned char)ch - 0x30);
}

int is_pchar(char ch) {
	if (is_alpha(ch) || is_digit(ch)) {
		return 1;
	}
	if (ch >= '$' && ch <= '.') {
		return 1;
	}
	return ch == '_' || ch == '~' || ch == ':' || ch == '@' || ch == '!' ||
		ch == ';' || ch == '=';
}

int is_regchar(char ch) {
	if (is_alpha(ch) || is_digit(ch)) {
		return 1;
	}
	if (ch >= '$' && ch <= '.') {
		return 1;
	}
	return ch == '_' || ch == '~' || ch == '!' || ch == ';' || ch == '=';
}
